#!/usr/bin/env python3
"""Versioned Studio catalogue references and failure-isolated 37E audits.

--cache-modern requires Studio, never runs 37E. --audit-modern requires an
existing archive, never runs Studio. --compare-modern first ensures Studio
references are cached and then audits 37E against that exact archive.
Per-score failures are isolated; successful renders are reused by SHA-256.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
from html import escape
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone

from reference_cache import ReferenceCache, png_dimensions, read_pages, sha256
from report_changes import SNAPSHOT, previous_completed_audit, make_changes_report, digest_pages
from bundle_changes import create_bundle
from review_bundle import export_review
from comparison_cache import ComparisonCache
from prune_unused import prune_unused, require_safe_scope
from score_archive import load_score_xml, write_prepared

SCRIPT_DIR = Path(__file__).resolve().parent


def import_hyphen_module(file, name):
    spec = importlib.util.spec_from_file_location(name, SCRIPT_DIR / file)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


prepare = import_hyphen_module('prepare-inputs.py', 'vtest_prepare')
studio = import_hyphen_module('render-studio-references.py', 'vtest_studio')


def atomic_json(path, doc):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.tmp')
    new_content = json.dumps(doc, sort_keys=True, indent=2) + '\n'
    if path.is_file() and path.read_text(encoding='utf-8') == new_content:
        return
    temporary.write_text(new_content, encoding='utf-8')
    temporary.replace(path)


def git_checkout(path):
    """Describe score *source* revision, not a cryptographic build assertion."""
    try:
        root = subprocess.run(['git', '-C', str(path), 'rev-parse', '--show-toplevel'],
                              capture_output=True, text=True, timeout=10, check=True).stdout.strip()
        revision = subprocess.run(['git', '-C', root, 'rev-parse', 'HEAD'],
                                  capture_output=True, text=True, timeout=10, check=True).stdout.strip()
        description = subprocess.run(['git', '-C', root, 'describe', '--tags', '--always'],
                                     capture_output=True, text=True, timeout=10).stdout.strip()
        dirty = subprocess.run(['git', '-C', root, 'status', '--porcelain', '--untracked-files=no'],
                               capture_output=True, text=True, timeout=15).stdout.strip()
        return {'source_revision': revision, 'source_description': description,
                'modified_tracked_files': bool(dirty)}
    except (OSError, subprocess.SubprocessError):
        return {'source_revision': None, 'source_description': None,
                'modified_tracked_files': None}


def selected_scores(root, recursive):
    if not root.is_dir():
        raise ValueError(f'not a score directory: {root}')
    paths = root.rglob('*') if recursive else root.iterdir()
    scores = {}
    for path in sorted((p for p in paths if p.is_file() and p.suffix.lower() in ('.mscx', '.mscz')),
                       key=lambda p: p.relative_to(root).as_posix()):
        relative = path.relative_to(root).as_posix()
        safe = re.sub(r'[^A-Za-z0-9_-]+', '-', path.stem).strip('-')[:44] or 'score'
        name = f'external-{safe}-{hashlib.sha256(relative.encode("utf-8")).hexdigest()[:12]}'
        if name in scores:
            raise ValueError(f'ID collision: {relative} / {scores[name]}')
        scores[name] = {'path': path, 'relative': relative}
    if not scores:
        raise ValueError(f'no MSCX/MSCZ scores under {root}' +
                         (' (use --recursive to include subdirectories)' if not recursive else ''))
    return scores


def prepare_external(name, path, prepared_dir, raw=False):
    """Attempt minimal XML preparation; retain original on incompatibility."""
    original_sha = sha256(path)
    fallback = {'studio': path, 'current': path, 'origin_sha256': original_sha,
                'studio_sha256': original_sha, 'current_sha256': original_sha,
                'preparation_warning': None}
    try:
        xml, member = load_score_xml(path)
        original_studio = xml if raw else prepare.strip_37e_only_style(xml)
        studio_path = path
        if original_studio != xml:
            studio_path = prepared_dir / f'{name}-studio{path.suffix.lower()}'
            write_prepared(path, studio_path, original_studio, member)
        current_path = studio_path
        if not raw:
            current_xml = prepare.apply_style(original_studio, {'beamLayout4': 1})
            if current_xml != original_studio:
                current_path = prepared_dir / f'{name}-37e{path.suffix.lower()}'
                write_prepared(path, current_path, current_xml, member)
        return {'studio': studio_path, 'current': current_path,
                'origin_sha256': original_sha, 'studio_sha256': sha256(studio_path),
                'current_sha256': sha256(current_path), 'preparation_warning': None}
    except (OSError, ValueError, UnicodeError, KeyError) as error:
        fallback['preparation_warning'] = (f'Input preparation failed ({type(error).__name__}: {error}); '
                                           'renderers were given the original file unchanged')
        return fallback
    except Exception as error:
        # An XML parsing failure is a meaningful compatibility finding. Keep
        # testing it with the original score instead of skipping the fixture.
        import xml.etree.ElementTree as ET
        if isinstance(error, ET.ParseError):
            fallback['preparation_warning'] = (f'XML preparation failed ({error}); '
                                               'renderers were given the original file unchanged')
            return fallback
        raise


def execute(exe, input_path, name, work, dpi, timeout, log_file, extra_args=()):
    """One process per score. A successful exit without pages is a failure."""
    for image in work.glob(name + '-[0-9]*.png'):
        image.unlink()
    job = [{'in': str(input_path), 'out': str(work / f'{name}.png')}]
    job_path = work / 'job.json'
    job_path.write_text(json.dumps(job), encoding='utf-8')
    command = [str(exe), *extra_args, '-j', str(job_path), '-r', str(dpi)]
    try:
        proc = subprocess.run(command, cwd=str(work), stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, errors='replace',
                              timeout=timeout)
        output = proc.stdout
        code = proc.returncode
        reason = None if code == 0 else f'renderer exited with status {code}'
    except subprocess.TimeoutExpired as error:
        code = None
        output = (error.stdout or b'')
        if isinstance(output, bytes):
            output = output.decode('utf-8', errors='replace')
        reason = f'renderer timed out after {timeout} seconds'
    except OSError as error:
        code = None
        output = str(error)
        reason = f'could not start renderer: {error}'
    log_file.parent.mkdir(parents=True, exist_ok=True)
    log_file.write_text(f'Command: {command}\nExit status: {code}\n\n{output}', encoding='utf-8')
    if reason is None:
        try:
            pages = read_pages(work, name)
        except (OSError, ValueError) as error:
            reason = f'no valid PNG export: {error}'
            pages = []
    else:
        pages = []
    status = {'status': 'ok' if reason is None else 'failed', 'exit_code': code,
              'error': reason, 'log': log_file.name, 'pages': pages,
              'output_excerpt': output[-1500:]}
    return status


def meta_pages(directory, name, pages):
    validated = []
    for number, entry in enumerate(pages, 1):
        filename = f'{name}-{number}.png'
        path = directory / filename
        if entry.get('file') != filename or not path.is_file():
            raise ValueError(f'missing reference page: {filename}')
        if sha256(path) != entry['sha256'] or png_dimensions(path) != entry['dimensions']:
            raise ValueError(f'invalid PNG page or hash mismatch: {path}')
        validated.append(path)
    return validated


def reference_for(name, prepared, archive, archive_dir):
    entry = archive.get('scores', {}).get(name)
    if not isinstance(entry, dict):
        return None, 'reference not present in archive'
    if entry.get('status') != 'ok':
        reason = entry.get('error') or 'no valid Studio PNG was produced'
        excerpt = str(entry.get('output_excerpt') or '').strip()[-400:]
        return None, 'Studio reference failed: ' + reason + (('; ' + excerpt) if excerpt else '')
    if entry.get('origin_sha256') != prepared['origin_sha256']:
        return None, 'stale reference: original score bytes changed'
    if entry.get('studio_source_sha256') != prepared['studio_sha256']:
        return None, 'stale reference: prepared Studio input changed'
    try:
        pages = meta_pages(archive_dir / 'scores' / name, name, entry['pages'])
    except (OSError, ValueError, TypeError, KeyError) as error:
        return None, f'corrupt or missing archived reference: {error}'
    return pages, None


def source_info(root):
    return {'score_directory_name': root.name, **git_checkout(root)}


def renderer_info(exe, tag, dpi):
    exe_sha = sha256(exe)
    version, reported = studio.studio_version(exe)
    slug_label = tag or (f'{version}' if version else 'unknown')
    slug = re.sub('[^A-Za-z0-9_.-]+', '-', slug_label).strip('-')[:48] or 'unknown'
    resource_salt = os.environ.get('VTEST_REFERENCE_CACHE_SALT', '')
    salt_suffix = ('-' + hashlib.sha256(resource_salt.encode()).hexdigest()[:8]) if resource_salt else ''
    dirname = f'studio-{slug}-{exe_sha[:12]}{salt_suffix}'
    return dirname, {'label': f'Studio {version}' if version else 'Studio (version unverified)',
                     'version': version, 'reported_version': reported,
                     'source_tag_user_supplied': tag, 'executable': str(exe),
                     'executable_sha256': exe_sha, 'resource_salt': resource_salt,
                     'dpi': dpi}


def archive_progress(path, data):
    records = list(data['scores'].values())
    data['summary'] = {'total': len(records), 'ok': sum(s.get('status') == 'ok' for s in records),
                       'failed': sum(s.get('status') != 'ok' for s in records)}
    atomic_json(path, data)


def studio_archive_one(name, item, args, html, cache, previous):
    """Isolated score process; only main thread publishes archive manifest."""
    prepared_dir = html / 'prepared-inputs'
    prep = prepare_external(name, item['path'], prepared_dir, args.raw)
    record = {'relative_path': item['relative'], 'origin_sha256': prep['origin_sha256'],
              'studio_source_sha256': prep['studio_sha256'],
              'preparation_warning': prep['preparation_warning'], 'status': 'failed',
              'pages': []}
    with tempfile.TemporaryDirectory(prefix='.studio-one-', dir=html) as temp:
        work = Path(temp)
        if cache.restore(name, prep['studio_sha256'], work, args.refresh or args.refresh_modern):
            old = previous.get(name, {})
            if (old.get('status') == 'ok' and old.get('origin_sha256') == prep['origin_sha256']
                    and old.get('studio_source_sha256') == prep['studio_sha256']
                    and old.get('relative_path') == item['relative']):
                status = {key: old[key] for key in ('status', 'error', 'exit_code', 'pages',
                                                   'log', 'output_excerpt') if key in old}
            else:
                status = {'status': 'ok', 'error': None, 'exit_code': 0,
                          'pages': read_pages(work, name), 'log': None, 'output_excerpt': ''}
            hit = True
        else:
            status = execute(args.studio, prep['studio'], name, work, args.dpi,
                             args.timeout, html / 'catalogue-logs' / f'{name}-studio.log')
            if status['status'] == 'ok':
                cache.store(name, prep['studio_sha256'], work, status['pages'])
            hit = False
    record.update(status)
    return name, record, hit


def render_archive(args, scores, root, html, archive_root):
    exe = Path(args.studio).expanduser().resolve()
    if not exe.is_file() or not os.access(exe, os.X_OK):
        raise ValueError(f'Studio executable not found or not executable: {exe}')
    dirname, info = renderer_info(exe, args.studio_source_tag, args.dpi)
    archive_dir = archive_root / dirname
    archive_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = archive_dir / 'manifest.json'
    original = None
    if manifest_path.exists():
        original = json.loads(manifest_path.read_text(encoding='utf-8'))
        if (original.get('schema') != 1 or original.get('renderer', {}).get('executable_sha256') !=
                info['executable_sha256'] or original.get('dpi') != args.dpi or
                original.get('renderer', {}).get('resource_salt', '') != info['resource_salt']):
            raise ValueError(f'archive provenance mismatch: {manifest_path}')
    archive = {'schema': 1, 'renderer': info, 'dpi': args.dpi,
               'score_source': source_info(root),
               'created_utc': original.get('created_utc') if original else
                              datetime.now(timezone.utc).isoformat(timespec='seconds'),
               'scores': dict(original.get('scores', {})) if original else {},
               'note': 'Reference success requires exit 0 and SHA-256-verified PNG pages. '
                       'This archive is intended for explicit Git tracking.'}
    cache = ReferenceCache(archive_dir / 'scores', exe, args.dpi, info['resource_salt'])
    (html / 'prepared-inputs').mkdir(parents=True, exist_ok=True)
    previous = dict(original.get('scores', {})) if original else {}
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {}
        for name, item in scores.items():
            future = pool.submit(studio_archive_one, name, item, args, html, cache, previous)
            futures[future] = (name, item)
        for completed, future in enumerate(as_completed(futures), 1):
            try:
                name, record, hit = future.result()
            except Exception as error:
                name, item = futures[future]
                record = {'relative_path': item['relative'], 'status': 'failed',
                          'error': f'Studio preparation/export exception: {type(error).__name__}: {error}',
                          'pages': [], 'origin_sha256': None,
                          'studio_source_sha256': None, 'preparation_warning': None,
                          'exit_code': None, 'log': None, 'output_excerpt': ''}
                hit = False
            archive['scores'][name] = record
            # Save each completed score, regardless of completion order.
            archive_progress(manifest_path, archive)
            detail = ('verified archive cache hit' if hit else
                      f'{len(record["pages"])} page(s), archived' if record['status'] == 'ok'
                      else f'FAILED: {record["error"]} (see {record.get("log")})')
            print(f'   [{completed}/{len(scores)}] {record["relative_path"]}: {detail}', flush=True)
    make_catalogue_index(html, archive, archive_dir)
    return archive_dir, archive


def make_catalogue_index(html, archive, archive_dir):
    rows = []
    for name, entry in archive['scores'].items():
        page = ''
        if entry['status'] == 'ok':
            link = os.path.relpath(archive_dir / 'scores' / name / (name + '-1.png'), html)
            page = f'<a href="{escape(link, quote=True)}">First page</a>'
        if entry.get('log'):
            page += f' <a href="catalogue-logs/{escape(entry["log"], quote=True)}">Log</a>'
        rows.append(f'<tr><td>{escape(entry["relative_path"])}</td>'
                    f'<td>{escape(entry["status"])}</td>'
                    f'<td>{len(entry.get("pages", []))}</td>'
                    f'<td>{escape(entry.get("error") or entry.get("preparation_warning") or "")}'
                    f' {escape(str(entry.get("output_excerpt") or "")[-300:])}</td>'
                    f'<td>{page}</td></tr>')
    info = archive['renderer']
    archive_rel = os.path.relpath(archive_dir / 'manifest.json', html)
    doc = ('<!doctype html><meta charset="utf-8"><title>Studio reference catalogue</title>'
           '<style>body{font:15px/1.5 system-ui,sans-serif;margin:24px}'
           'table{border-collapse:collapse}td,th{border:1px solid #ccc;padding:6px 10px}'
           'th{background:#eee}</style><h1>Studio reference catalogue</h1>'
           f'<p>Version reported by executable: <code>{escape(info["reported_version"])}</code></p>'
           f'<p>Binary SHA-256: <code>{escape(info["executable_sha256"])}</code></p>'
           f'<p>Source tag (user-supplied): <code>{escape(str(info.get("source_tag_user_supplied") or "not specified"))}</code></p>'
           f'<p>Score-source Git revision: <code>{escape(str(archive["score_source"].get("source_revision") or "unavailable"))}</code></p>'
           f'<p>Manifest: <a href="{escape(archive_rel, quote=True)}">{escape(archive_rel)}</a>. '
           'Archive images are separate from generated HTML and can be committed to Git.</p>'
           f'<p>Processed: {archive["summary"]["total"]}; valid: {archive["summary"]["ok"]}; '
           f'failed: {archive["summary"]["failed"]}.</p>'
           '<table><thead><tr><th>Score</th><th>Status</th><th>Pages</th><th>Issue</th>'
           '<th>Files</th></tr></thead><tbody>' + '\n'.join(rows) + '</tbody></table>')
    (html / 'studio-catalogue.html').write_text(doc, encoding='utf-8')



def choose_archive(archive_path, archive_root):
    """Choose an existing Studio archive, without invoking the Studio binary."""
    if archive_path is not None:
        archive_dir = Path(archive_path).expanduser().resolve()
    else:
        options = sorted(p for p in archive_root.glob('studio-*')
                         if (p / 'manifest.json').is_file())
        if len(options) != 1:
            raise ValueError(f'expected exactly one Studio archive; found {len(options)}. '
                             'Specify --archive /path/to/studio-VERSION-HASH')
        archive_dir = options[0]
    manifest = archive_dir / 'manifest.json'
    if not manifest.is_file():
        raise ValueError(f'no Studio manifest: {manifest}')
    archive = json.loads(manifest.read_text(encoding='utf-8'))
    if archive.get('schema') != 1 or not isinstance(archive.get('scores'), dict):
        raise ValueError(f'invalid reference archive: {manifest}')
    return archive_dir, manifest, archive


def verify_archive(args, scores, root, archive_root):
    """Read-only portability preflight: no MuseScore executable, cache or HTML required.

    Inspect the archived manifest, source score bytes, Studio-prepared score
    bytes, archived PNGs and per-score cache metadata. Renderer paths in the
    manifest are provenance only; they need not exist on this computer.
    """
    archive_dir, manifest_path, archive = choose_archive(args.archive, archive_root)
    renderer = archive.get('renderer', {})
    metadata_errors = []
    for key in ('executable_sha256', 'reported_version'):
        if not renderer.get(key):
            metadata_errors.append(f'archive renderer missing {key}')
    if archive.get('dpi') != args.dpi or renderer.get('dpi') != args.dpi:
        metadata_errors.append(f'archive DPI {archive.get("dpi")!r} differs from requested {args.dpi}')
    source = archive.get('score_source', {})
    checkout = git_checkout(root)
    provenance_warnings = []
    archived_rev = source.get('source_revision')
    current_rev = checkout.get('source_revision')
    if archived_rev and current_rev != archived_rev:
        provenance_warnings.append(f'fixture checkout revision differs: archive={archived_rev}, current={current_rev or "unknown"}')
    if archived_rev and checkout.get('modified_tracked_files'):
        provenance_warnings.append('fixture checkout contains modified tracked files')
    if not archived_rev:
        provenance_warnings.append('archive has no verified fixture Git revision (source tag is user-supplied)')
    if args.strict_source and (not archived_rev or current_rev != archived_rev or checkout.get('modified_tracked_files')):
        metadata_errors.append('strict fixture revision check failed; use a clean checkout at the archived source_revision')
    if not args.strict_source and provenance_warnings:
        for warning in provenance_warnings:
            print('WARNING: ' + warning, flush=True)

    results = {}
    counts = {'verified': 0, 'missing': 0, 'stale': 0, 'corrupt': 0, 'studio_failed': 0}
    # Prepared files are temporary. No report folder or reference archive is modified.
    with tempfile.TemporaryDirectory(prefix='vtest-verify-modern-') as temp:
        prepared_dir = Path(temp)
        for number, (name, item) in enumerate(scores.items(), 1):
            record = archive['scores'].get(name)
            reason = None
            state = 'verified'
            try:
                if record is None:
                    state, reason = 'missing', 'no archived reference entry'
                elif not isinstance(record, dict):
                    state, reason = 'corrupt', 'archive entry is not an object'
                elif record.get('relative_path') != item['relative']:
                    state, reason = 'stale', 'fixture path differs from archive entry'
                elif record.get('status') != 'ok':
                    state, reason = 'studio_failed', record.get('error') or 'archived Studio render failed'
                else:
                    prep = prepare_external(name, item['path'], prepared_dir, args.raw)
                    if (record.get('origin_sha256') != prep['origin_sha256'] or
                            record.get('studio_source_sha256') != prep['studio_sha256']):
                        state, reason = 'stale', 'original or prepared Studio input bytes differ'
                    else:
                        pages = record.get('pages')
                        if not isinstance(pages, list) or not pages:
                            raise ValueError('archive has no page records')
                        meta_pages(archive_dir / 'scores' / name, name, pages)
                        cache_file = archive_dir / 'scores' / name / 'cache.json'
                        cache = json.loads(cache_file.read_text(encoding='utf-8'))
                        if (cache.get('schema') != 1 or
                                cache.get('test') != name or
                                cache.get('executable_sha256') != renderer.get('executable_sha256') or
                                cache.get('dpi') != args.dpi or
                                cache.get('resource_salt', '') != renderer.get('resource_salt', '') or
                                cache.get('source_sha256') != prep['studio_sha256'] or
                                cache.get('pages') != pages):
                            raise ValueError('cache metadata does not match archived renderer/input/pages')
            except (OSError, ValueError, TypeError, KeyError, AttributeError, json.JSONDecodeError) as error:
                state, reason = 'corrupt', f'{type(error).__name__}: {error}'
            counts[state] += 1
            results[name] = {'relative_path': item['relative'], 'status': state, 'error': reason}
            if state != 'verified':
                print(f'  [{number}/{len(scores)}] {item["relative"]}: {state}: {reason}', flush=True)
            elif number == 1 or number % 100 == 0 or number == len(scores):
                print(f'  [verify {number}/{len(scores)}] {counts["verified"]} valid Studio references', flush=True)
    # A valid local archive is not necessarily *portable*: local files (or
    # staged additions) can be absent from a fresh clone. Require HEAD content.
    expected_files = [manifest_path]
    for name in scores:
        entry = archive['scores'].get(name, {})
        if entry.get('status') == 'ok':
            folder = archive_dir / 'scores' / name
            expected_files.append(folder / 'cache.json')
            expected_files.extend(folder / page['file'] for page in entry.get('pages', [])
                                  if isinstance(page, dict) and isinstance(page.get('file'), str))
    tracked_paths = set()
    try:
        git_root = Path(subprocess.run(
            ['git', '-C', str(archive_dir), 'rev-parse', '--show-toplevel'],
            capture_output=True, text=True, check=True, timeout=10).stdout.strip()).resolve()
        relative_archive = archive_dir.relative_to(git_root)
        files_result = subprocess.run(
            ['git', '-C', str(git_root), 'ls-tree', '-r', '--name-only', '-z',
             'HEAD', '--', str(relative_archive)],
            capture_output=True, check=True, timeout=15)
        tracked_paths = {git_root / os.fsdecode(path) for path in
                         files_result.stdout.split(b'\0') if path}
    except (OSError, ValueError, subprocess.SubprocessError):
        pass
    untracked = [str(path) for path in expected_files if path not in tracked_paths]
    # A path can exist in HEAD but have newer local bytes. Verify against the
    # committed tree, not just the presence of an index or object entry.
    modified_archive = False
    if tracked_paths:
        diff = subprocess.run(
            ['git', '-C', str(git_root), 'diff', '--quiet', 'HEAD', '--', str(relative_archive)],
            capture_output=True, timeout=30)
        modified_archive = diff.returncode != 0
    if untracked:
        msg = (f'{len(untracked)} required archive file(s) are not committed in Git HEAD; '
               f'example: {untracked[0]}')
        if args.require_committed:
            metadata_errors.append(msg)
        else:
            print('WARNING: ' + msg, flush=True)
    if modified_archive:
        msg = 'archive has modifications relative to Git HEAD; a fresh clone would have different files'
        if args.require_committed:
            metadata_errors.append(msg)
        else:
            print('WARNING: ' + msg, flush=True)
    if metadata_errors:
        for error in metadata_errors:
            print('ERROR: ' + error, flush=True)
    summary = {'schema': 1, 'archive_manifest': str(manifest_path),
               'studio_reported_version': renderer.get('reported_version'),
               'studio_executable_sha256': renderer.get('executable_sha256'),
               'studio_source_tag': renderer.get('source_tag_user_supplied'),
               'fixture_revision_archived': archived_rev,
               'fixture_revision_current': current_rev,
               'provenance_warnings': provenance_warnings,
               'metadata_errors': metadata_errors,
               'required_archive_files': len(expected_files),
               'uncommitted_archive_files': len(untracked),
               'modified_archive_relative_to_head': modified_archive,
               'untracked_examples': untracked[:5],
               'selected_scores': len(scores),
               'additional_archive_entries': len(set(archive['scores']) - set(scores)),
               'counts': counts, 'scores': results,
               'complete': not metadata_errors and counts['verified'] == len(scores)}
    if args.verification_json:
        atomic_json(args.verification_json.expanduser().resolve(), summary)
        print(f'Verification JSON: {args.verification_json}', flush=True)
    print(f'Portable Studio references: {counts["verified"]}/{len(scores)} verified; '
          f'{counts["missing"]} missing; {counts["stale"]} stale; '
          f'{counts["corrupt"]} corrupt; {counts["studio_failed"]} failed Studio exports.', flush=True)
    print(f'Archive Studio version: {renderer.get("reported_version") or "unavailable"}; '
          f'fixture source Git SHA: {archived_rev or "unavailable"}', flush=True)
    return summary['complete']


def current_audit_one(name, item, args, html, cache):
    """Isolated 37E export with verified, resumable successful PNG caching.

    An audit has different import semantics from normal 37E rendering: -f
    deliberately allows scores written by newer MuseScore versions. Keeping
    this in the cache salt prevents mixing forced and ordinary renders.
    """
    prep = prepare_external(name, item['path'], html / 'prepared-inputs', args.raw)
    log_file = html / 'catalogue-logs' / f'{name}-37e.log'
    with tempfile.TemporaryDirectory(prefix='.37e-one-', dir=html) as temp:
        working = Path(temp)
        cached = cache.restore(name, prep['current_sha256'], working,
                               args.refresh or args.refresh_current)
        if cached:
            result = {'status': 'ok', 'exit_code': 0, 'error': None,
                      'log': None,
                      'pages': read_pages(working, name), 'output_excerpt': ''}
        else:
            result = execute(args.current, prep['current'], name, working,
                             args.dpi, args.timeout, log_file, extra_args=('-f',))
            if result['status'] == 'ok':
                cache.store(name, prep['current_sha256'], working, result['pages'])
        # Comparisons run in the main thread; all worker filenames are unique.
        for path in html.glob(name + '-[0-9]*.png'):
            path.unlink()
        if result['status'] == 'ok':
            for page in result['pages']:
                shutil.copy2(working / page['file'], html / page['file'])
    return name, prep, result, cached


def audit_archive(args, scores, root, html, archive_root):
    archive_dir, manifest, archive = choose_archive(args.archive, archive_root)
    current = Path(args.current).expanduser().resolve()
    if not current.is_file() or not os.access(current, os.X_OK):
        raise ValueError(f'37E executable not found or not executable: {current}')
    previous = previous_completed_audit(html)
    if previous is not None and not (html / SNAPSHOT).exists():
        # Preserve the last complete v0.18 result before incremental audit JSON
        # updates can overwrite it. This also survives an interrupted first run.
        atomic_json(html / SNAPSHOT, {**previous, 'completed': True})
    from PIL import Image  # dependency expected by engraving report
    del Image
    comparison = import_hyphen_module('compare-engraving.py', 'vtest_compare')
    comparison_cache = ComparisonCache(html, comparison, refresh=args.refresh_comparisons)
    results = []
    audit = {'schema': 1, 'reference_manifest': str(manifest),
             'reference_version': archive.get('renderer', {}).get('reported_version'),
             'score_source': source_info(root), 'scores': {}}
    prepared_dir = html / 'prepared-inputs'
    prepared_dir.mkdir(parents=True, exist_ok=True)
    # The forced-open policy is part of the cache identity: never silently
    # reuse PNGs from the ordinary version-checking 37E loader.
    current_salt = 'catalogue-force-v1:' + os.environ.get('VTEST_37E_CACHE_SALT', '')
    cache = ReferenceCache(html / '37e-cache', current, args.dpi, current_salt)
    audit['current_renderer_sha256'] = cache.identity['executable_sha256']
    audit['reference_renderer_sha256'] = archive['renderer'].get('executable_sha256')
    audit['dpi'] = args.dpi
    audit['raw'] = args.raw
    audit['selection'] = {'root': str(root), 'recursive': args.recursive, 'match': args.match}
    audit['force_newer_scores'] = True
    audit['jobs'] = args.jobs
    audit['current_cache_root'] = str(cache.root)
    work_items = list(scores.items())
    # Render in independent processes, with distinct jobs and temporary
    # directories. Main thread alone owns audit JSON and HTML updates.
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {}
        for name, item in work_items:
            future = pool.submit(current_audit_one, name, item, args, html, cache)
            futures[future] = (name, item)
        finished = {}
        for count, future in enumerate(as_completed(futures), 1):
            name, item = futures[future]
            try:
                result_name, prep, result, cached = future.result()
            except Exception as error:
                prep = {'origin_sha256': None, 'studio_sha256': None,
                        'current_sha256': None,
                        'preparation_warning': f'37E preparation/export exception: {type(error).__name__}: {error}'}
                result = {'status': 'failed', 'error': prep['preparation_warning'],
                          'exit_code': None, 'pages': [], 'log': None,
                          'output_excerpt': str(error)}
                cached = False
            finished[name] = (prep, result, cached)
            audit['scores'][name] = {
                'relative_path': item['relative'],
                '37e_status': result['status'], '37e_error': result['error'],
                '37e_exit_code': result['exit_code'], '37e_cached': cached,
                '37e_forced_load': True,
                '37e_prepared_sha256': prep['current_sha256'],
                'studio_prepared_sha256': prep['studio_sha256'],
                'origin_sha256': prep['origin_sha256'],
                '37e_output_sha256': digest_pages(result['pages']),
                'reference_status': 'pending',
                'preparation_warning': prep['preparation_warning'],
                '37e_output_excerpt': result.get('output_excerpt'),
                '37e_log': ('catalogue-logs/' + result['log']) if result.get('log') else None,
                '37e_pages': len(result['pages'])}
            atomic_json(html / 'catalogue-audit.json', audit)
            print(f'   [{count}/{len(scores)}] {item["relative"]}: '
                  f'{"verified 37E cache hit" if cached else result["status"]}', flush=True)
    cache_hits = sum(cached for _, _, cached in finished.values())
    print(f'37E rendering/cache checks complete: {len(work_items)} scores, '
          f'{cache_hits} cache hits. Checking engraving comparison cache '
          '(PNG hashes, analysis version and derived images)...', flush=True)
    # Deterministic report order, independent of process completion order.
    compared_pages = 0
    for report_count, (name, item) in enumerate(work_items, 1):
        prep, result, cached = finished[name]
        refs, reference_error = reference_for(name, prep, archive, archive_dir)
        audit['scores'][name].update({
            'reference_status': 'ok' if refs else 'unavailable',
            'reference_error': reference_error,
            'reference_pages': len(refs or []),
            'reference_pages_sha256': digest_pages(archive.get('scores', {}).get(name, {}).get('pages')
                                                   if refs else None)})
        diagnostics = []
        if result['error']:
            detail = (result.get('output_excerpt') or '').strip()[-400:]
            diagnostics.append('37E: ' + result['error'] + ('; ' + detail if detail else ''))
        if reference_error:
            diagnostics.append('Studio archive: ' + reference_error)
        if prep['preparation_warning']:
            diagnostics.append(prep['preparation_warning'])
        max_pages = max(len(result['pages']), len(refs or []), 1)
        counts = {'37E current': len(result['pages']), 'Studio reference': len(refs or [])}
        for n in range(1, max_pages + 1):
            key = name if n == 1 else f'{name}-p{n}'
            record = {'name': name, 'key': key, 'page': n, 'pages_total': max_pages,
                      'page_counts': counts, 'input': {'origin': str(item['path']),
                      'profile': 'external-original', 'source_sha256': prep['current_sha256'],
                      'origin_sha256': prep['origin_sha256'],
                      'studio_source_sha256': prep['studio_sha256'],
                      'style_overrides': {'beamLayout4': 1} if not args.raw else {}}}
            if n == 1:
                record['diagnostic'] = '; '.join(diagnostics) if diagnostics else None
                if result.get('log'):
                    record['diagnostic_log'] = f'catalogue-logs/{result["log"]}'
            if n <= len(result['pages']) and refs and n <= len(refs):
                analysis, _from_cache = comparison_cache.analyse(name, n, refs[n-1])
                record.update(analysis)
                compared_pages += 1
                # analyse returns matching name and key; retain our metadata.
                record['page_counts'] = counts
                record['pages_total'] = max_pages
                record['input'] = {'origin': str(item['path']), 'profile': 'external-original',
                                   'source_sha256': prep['current_sha256'],
                                   'origin_sha256': prep['origin_sha256'],
                                   'studio_source_sha256': prep['studio_sha256'],
                                   'style_overrides': {'beamLayout4': 1} if not args.raw else {}}
            elif n <= len(result['pages']):
                comparison_cache.normalize_current(name, n)
                record['current_only'] = True
                record['current_only_note'] = (
                    'No corresponding Studio page — 37E has an additional page (page counts differ)'
                    if refs else 'Studio reference unavailable for this score')
                if reference_error:
                    record['diagnostic'] = ((record.get('diagnostic') or '')
                                            + ('; ' if record.get('diagnostic') else '')
                                            + reference_error)
            elif refs and n <= len(refs):
                record['missing'] = (result['error'] or
                                     'No corresponding 37E page — Studio has an additional page '
                                     '(page counts differ)')
                record['missing_image'] = os.path.relpath(refs[n-1], html)
                record['missing_image_label'] = 'Archived Studio reference'
            else:
                record['missing'] = result['error'] or 'No 37E output and no comparable Studio reference'
            results.append(record)
        if report_count == 1 or report_count % 25 == 0 or report_count == len(work_items):
            print(f'   [report {report_count}/{len(work_items)}] {item["relative"]} '
                  f'({compared_pages} comparable pages; '
                  f'{comparison_cache.hits} analysis cache hits)', flush=True)
    print(f'Image comparisons complete: {compared_pages} comparable pages '
          f'({comparison_cache.hits} reused, {comparison_cache.misses} recalculated); '
          f'{comparison_cache.current_only_hits} unmatched-page normalizations reused, '
          f'{comparison_cache.current_only_misses} recalculated; '
          f'{len(results)} page sections. Assembling HTML summary and navigation...', flush=True)
    audit['completed'] = True
    markup = comparison.page(results, 5,
                             archive.get('renderer', {}).get('label', 'Studio archive'),
                             archive={'path': os.path.relpath(manifest, html),
                                      'renderer': archive['renderer'],
                                      'audit': audit})
    print('Writing engraving.html and latest-changes view...', flush=True)
    zip_link = (' · <a href="changes/latest-changes.zip" download>Share changed PNGs (ZIP)</a>'
                if args.bundle_changes else '')
    if args.bundle_review:
        review_href = ('engraving-review-all.zip' if args.bundle_review == 'all' else
                       'changes/engraving-review-changes.zip')
        zip_link += (' · <a href="' + review_href + '" download>Standalone ' +
                     args.bundle_review + ' review ZIP</a>')
    markup = markup.replace('<h1>MuseScore visual comparisons</h1>',
                            '<h1>MuseScore visual comparisons</h1>'
                            '<p><a href="changes/engraving.html">View latest new/changed scores</a>'
                            + zip_link + '</p>', 1)
    (html / 'engraving.html').write_text(markup, encoding='utf-8')
    changed = make_changes_report(html, manifest, archive, audit, results, previous,
                                  comparison, atomic_json, bundle_enabled=args.bundle_changes,
                                  review_mode=args.bundle_review)
    if args.bundle_changes:
        create_bundle(html, archive_dir, archive, audit, results, changed,
                      json.loads((html / 'changes' / 'changes.json').read_text(encoding='utf-8'))['removed'])
    else:
        # A previous latest-changes.zip is no longer current after this audit.
        (html / 'changes' / 'latest-changes.zip').unlink(missing_ok=True)
    atomic_json(html / 'catalogue-audit.json', audit)
    # Do not let an earlier report leave a stale archive masquerading as the
    # current run. User-saved copies outside this report directory are untouched.
    for path in (html / 'engraving-review-all.zip',
                 html / 'changes' / 'engraving-review-changes.zip'):
        path.unlink(missing_ok=True)
    if args.bundle_review:
        # Review export is derived entirely from the just-completed audit and
        # verified comparison products. It cannot change latest-changes state.
        export_review(html, args.bundle_review, archive_override=archive_dir)
    # A cleanup preview must not consume the latest-changes checkpoint: the
    # next real audit should still report additions/removals from that baseline.
    if not args.prune_dry_run:
        atomic_json(html / SNAPSHOT, audit)
    if args.prune_unused:
        # Do not prune until both full and changes reports, optional ZIP,
        # and the completed audit checkpoint have been written successfully.
        prune_unused(html, scores, audit, previous, dry_run=args.prune_dry_run)
    print(f'Audit report: {html / "engraving.html"}', flush=True)
    print(f'Audit statuses: {html / "catalogue-audit.json"}', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('--cache-modern', action='store_true', help='build versioned, Git-trackable Studio PNG archive only')
    modes.add_argument('--audit-modern', action='store_true', help='render 37E per score against existing Studio archive')
    modes.add_argument('--compare-modern', action='store_true', help='cache Studio and audit 37E in a single command')
    modes.add_argument('--verify-modern', action='store_true', help='verify Studio archive and fixture checksums without either MuseScore binary')
    modes.add_argument('--export-review', choices=('all', 'changes'), metavar='{all,changes}',
                       help='package a completed audit without launching renderers or changing checkpoints')
    parser.add_argument('--score-dir', type=Path, help='source score directory (not needed for --export-review)')
    parser.add_argument('--recursive', action='store_true')
    parser.add_argument('--match', help='optional regular expression for score paths; use for smaller audits')
    parser.add_argument('--studio', help='Studio executable (cache mode)')
    parser.add_argument('--current', help='37E executable (audit mode)')
    parser.add_argument('--archive', type=Path, help='select archive for audit when several versions exist')
    parser.add_argument('--output-dir', type=Path,
                        help='isolated output root (html/, references/); keeps normal vtest/html untouched')
    parser.add_argument('--archive-root', type=Path,
                        help='Studio archive root (defaults to output-dir/references if provided)')
    parser.add_argument('--html-dir', type=Path,
                        help='generated HTML/cache directory (defaults to output-dir/html if provided)')
    parser.add_argument('--studio-source-tag', help='user-supplied Studio source tag, not verified from binary')
    parser.add_argument('--require-committed', action='store_true',
                        help='verify mode: require all reference files committed and unchanged in Git HEAD')
    parser.add_argument('--strict-source', action='store_true',
                        help='verify mode: require matching clean fixture Git revision')
    parser.add_argument('--verification-json', type=Path,
                        help='verify mode: write machine-readable verification results')
    parser.add_argument('--dpi', type=int, default=130)
    parser.add_argument('--timeout', type=int, default=180, help='per-score renderer timeout in seconds')
    parser.add_argument('--refresh', action='store_true', help='force selected renderer to rerender')
    parser.add_argument('--refresh-current', action='store_true', help='force 37E audit rerender, bypassing current cache')
    parser.add_argument('--refresh-modern', action='store_true', help='force Studio reference regeneration only')
    parser.add_argument('--refresh-comparisons', action='store_true',
                        help='rebuild derived PNGs/measurements; keep renderer caches')
    parser.add_argument('--jobs', type=int, default=1, help='parallel independent score processes (default: 1)')
    parser.add_argument('--raw', action='store_true', help='do not inject beamLayout4=1 into 37E inputs')
    parser.add_argument('--bundle-changes', action='store_true',
                        help='ZIP changed-score 37E/Studio/difference PNGs for sharing')
    parser.add_argument('--bundle-review', choices=('all', 'changes'), metavar='{all,changes}',
                        help='make a standalone HTML/PNG/metrics review ZIP during the audit')
    parser.add_argument('--prune-unused', action='store_true',
                        help='delete orphaned local 37E and comparison products after a completed audit')
    parser.add_argument('--prune-dry-run', action='store_true',
                        help='report what --prune-unused would delete, without deleting anything')
    args = parser.parse_args()
    if args.export_review:
        if args.score_dir is not None or not args.output_dir or args.bundle_review or args.bundle_changes or args.prune_unused or args.prune_dry_run or args.refresh or args.refresh_current or args.refresh_modern or args.refresh_comparisons or args.verify_modern or args.strict_source or args.require_committed or args.match or args.studio or args.current or args.studio_source_tag or args.recursive:
            parser.error('--export-review requires --output-dir, and takes no rendering/verification options or --score-dir')
        html = args.html_dir.expanduser().resolve() if args.html_dir else (args.output_dir.expanduser().resolve() / 'html')
        try:
            export_review(html, args.export_review, archive_override=args.archive)
        except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
            parser.exit(1, f'ERROR: {error}\n')
        return
    if args.score_dir is None:
        parser.error('--score-dir is required for catalogue rendering and verification')
    if args.bundle_review and not (args.audit_modern or args.compare_modern):
        parser.error('--bundle-review requires --audit-modern or --compare-modern')
    if args.timeout <= 0 or args.dpi <= 0 or args.jobs < 1 or args.jobs > 32:
        parser.error('--timeout and --dpi must be positive; --jobs must be 1..32')
    if (args.strict_source or args.require_committed or args.verification_json) and not args.verify_modern:
        parser.error('--strict-source, --require-committed and --verification-json require --verify-modern')
    if args.verify_modern and (args.refresh or args.refresh_current or args.refresh_modern or
                               args.refresh_comparisons or args.prune_unused or args.bundle_changes):
        parser.error('--verify-modern is read-only and does not support rendering, refresh, cleanup or bundling')
    if args.prune_dry_run and not args.prune_unused:
        parser.error('--prune-dry-run requires --prune-unused')
    if args.prune_unused and args.cache_modern:
        parser.error('--prune-unused requires --audit-modern or --compare-modern')
    if args.prune_unused and args.match is not None:
        parser.error('--prune-unused cannot be used with --match (partial selection)')
    if args.prune_unused and args.output_dir is None:
        parser.error('--prune-unused requires an isolated --output-dir')
    if args.bundle_changes and args.cache_modern:
        parser.error('--bundle-changes requires --audit-modern or --compare-modern')
    if args.refresh_comparisons and args.cache_modern:
        parser.error('--refresh-comparisons requires --audit-modern or --compare-modern')
    if args.audit_modern and args.refresh_modern:
        parser.error('--refresh-modern requires --cache-modern or --compare-modern')
    if args.cache_modern and args.refresh_current:
        parser.error('--refresh-current requires --audit-modern or --compare-modern')
    if args.audit_modern and args.studio_source_tag:
        parser.error('--studio-source-tag requires --cache-modern or --compare-modern')
    if not (args.audit_modern or args.verify_modern) and args.archive:
        parser.error('--archive requires --audit-modern or --verify-modern')
    if (args.cache_modern or args.compare_modern) and not args.studio:
        parser.error('--cache-modern and --compare-modern require --studio (VTEST_REFERENCE_MSCORE)')
    if (args.audit_modern or args.compare_modern) and not args.current:
        parser.error('--audit-modern and --compare-modern require --current (VTEST_MSCORE)')
    try:
        root = args.score_dir.expanduser().resolve()
        scores = selected_scores(root, args.recursive)
        if args.match:
            pattern = re.compile(args.match)
            scores = {name: item for name, item in scores.items()
                      if pattern.search(item['relative'])}
            if not scores:
                raise ValueError(f'no scores match expression: {args.match}')
        output_root = args.output_dir.expanduser().resolve() if args.output_dir else None
        html = (args.html_dir.expanduser().resolve() if args.html_dir else
                (output_root / 'html' if output_root else SCRIPT_DIR / 'html'))
        archive_root = (args.archive_root.expanduser().resolve() if args.archive_root else
                        (output_root / 'references' if output_root else SCRIPT_DIR / 'references'))
        # Never include temporary prepared MSCX/MSCZ copies in a recursive
        # selection. Such feedback would grow the catalogue on every run.
        if args.recursive and (html == root or root in html.parents):
            raise ValueError('report HTML directory must be outside recursive --score-dir')
        if args.prune_unused:
            require_safe_scope(previous_completed_audit(html), root, args.recursive,
                               args.match, html)
        if args.verify_modern:
            return 0 if verify_archive(args, scores, root, archive_root) else 1
        html.mkdir(parents=True, exist_ok=True)
        if args.cache_modern or args.compare_modern:
            print('Checking Studio references and rendering cache misses...', flush=True)
            folder, archive = render_archive(args, scores, root, html, archive_root)
            print(f'Versioned reference archive: {folder}', flush=True)
            print(f'Catalogue index: {html / "studio-catalogue.html"}', flush=True)
            print(f'Completed: {archive["summary"]["ok"]} valid; '
                  f'{archive["summary"]["failed"]} failed', flush=True)
        if args.audit_modern or args.compare_modern:
            if args.compare_modern:
                # Use precisely the archive produced/validated above, even if
                # the root contains other Studio versions.
                args.archive = folder
                print('Studio reference pass complete. Checking 37E cache and rendering misses...', flush=True)
            audit_archive(args, scores, root, html, archive_root)
    except (OSError, ValueError, TypeError, KeyError, json.JSONDecodeError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    sys.exit(main())
