#!/usr/bin/env python3
"""Supplement historical MuseScore vtest/ with background-normalized image comparisons.

Run from vtest/: python3 compare-engraving.py tie-1
Requires Pillow (python3-pil). Does not change source/reference images.
"""
import argparse
import hashlib
import json
import os
import re
from collections import Counter
from html import escape
from pathlib import Path

from input_provenance import load_inputs
import sys

try:
    from PIL import Image, ImageChops
except ImportError:
    sys.exit("Pillow is required: install the python3-pil package (or install Pillow in a venv)")


def normalize_ink(path):
    """Composite transparency, then map the empty-page background to white.

    Per-channel adjustment also normalizes anti-aliased black ink that was
    originally rendered over a colored background, unlike -alpha remove.
    This comparison intentionally uses grayscale for color-independent layout.
    """
    source = Image.open(path).convert('RGBA')
    rgb = Image.new('RGBA', source.size, (255, 255, 255, 255))
    rgb.alpha_composite(source)
    rgb = rgb.convert('RGB')
    w, h = rgb.size
    corners = [(0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)]
    background = Counter(rgb.getpixel(xy) for xy in corners).most_common(1)[0][0]
    # A dark corner could be notation rather than the page background.
    if min(background) < 128:
        raise ValueError(f'{path.name}: corner background {background} is too dark to normalize safely')
    channels = rgb.split()
    corrected = []
    for channel, base in zip(channels, background):
        lut = [min(255, round(i * 255 / base)) for i in range(256)]
        corrected.append(channel.point(lut))
    return Image.merge('RGB', corrected).convert('L'), background


def staff_anchor(image):
    """Detect the first long, continuous staff line; return (x0, y, x1).

    Does not attempt scaling, distortion, or per-measure registration. If no
    convincing continuous staff line exists, skip registration entirely.
    """
    width, height = image.size
    cutoff = max(55, int(width * 0.36))
    threshold = 150
    pixels = image.load()
    candidates = []
    for y in range(height):
        start = None
        longest = (0, 0)
        for x in range(width + 1):
            dark = x < width and pixels[x, y] < threshold
            if dark and start is None:
                start = x
            elif not dark and start is not None:
                if x - start > longest[1] - longest[0]:
                    longest = (start, x)
                start = None
        if longest[1] - longest[0] >= cutoff:
            candidates.append((longest[0], y, longest[1] - 1))
    return candidates[0] if candidates else None



def staff_line_spacing(image, anchor):
    """Estimate first-staff spatium in pixels from five full-width staff lines.

    The result is diagnostic only. Return None if we cannot identify a
    consistent set of five approximately equally spaced long lines.
    """
    if anchor is None:
        return None
    x0, y0, x1 = anchor
    span = x1 - x0 + 1
    if span < 30:
        return None
    pixels = image.load()
    lines = [y0]
    # Only scan the first 100 pixels to avoid counting a distant staff.
    for y in range(y0 + 1, min(y0 + 100, image.height)):
        dark = sum(pixels[x, y] < 150 for x in range(x0, x1 + 1))
        if dark >= span * .8 and y > lines[-1] + 1:
            lines.append(y)
            if len(lines) == 5:
                break
    if len(lines) != 5:
        return None
    gaps = [b - a for a, b in zip(lines, lines[1:])]
    avg = sum(gaps) / len(gaps)
    if avg < 2 or max(abs(g - avg) for g in gaps) > max(1.5, .2 * avg):
        return None
    return avg


def difference(current, reference, tolerance):
    # Grayscale, equal-sized canvases. Allow a small anti-aliasing tolerance.
    d = ImageChops.difference(current, reference)
    mask = d.point([0 if x <= tolerance else 255 for x in range(256)])
    changed = mask.histogram()[255]
    # Light gray music underneath; differences in red for visual context.
    overlay = Image.blend(current, reference, .5)
    overlay = Image.blend(Image.new('L', current.size, 255), overlay, .32).convert('RGB')
    overlay.paste((221, 38, 58), (0, 0, *current.size), mask)
    return overlay, changed


# Keep the original 5/255 pixel metric separate from this intentionally
# higher-contrast, binary ink view. Anti-aliased edge variations can dominate
# per-pixel differences without meaningfully changing a notehead or tie.
DEFAULT_INK_CUTOFF = 190
CURRENT_COLOR = (190, 35, 47)    # red: ink found only in the current build
REFERENCE_COLOR = (30, 83, 188)  # blue: ink found only in the reference
SHARED_COLOR = (75, 75, 75)      # dark gray: ink in both


def ink_difference(current, reference, cutoff):
    """Binary engraving shapes with red current-only and blue reference-only ink.

    A pixel is classified as ink when its normalized gray value is below
    cutoff. The classification is deliberately independent of the fine-pixel
    tolerance and cannot be used as a replacement for the original metric.
    """
    threshold = [255 if intensity < cutoff else 0 for intensity in range(256)]
    current_ink = current.point(threshold)
    reference_ink = reference.point(threshold)
    shared = ImageChops.darker(current_ink, reference_ink)
    current_only = ImageChops.subtract(current_ink, reference_ink)
    reference_only = ImageChops.subtract(reference_ink, current_ink)
    canvas = Image.new('RGB', current.size, (255, 255, 255))
    canvas.paste(SHARED_COLOR, (0, 0), shared)
    canvas.paste(CURRENT_COLOR, (0, 0), current_only)
    canvas.paste(REFERENCE_COLOR, (0, 0), reference_only)
    current_count = current_only.histogram()[255]
    reference_count = reference_only.histogram()[255]
    return canvas, (current_count, reference_count)


def directional_pixel_difference(current, reference, tolerance):
    """Fine intensity changes, color-coded by which rendering is darker."""
    cutoff = [0 if delta <= tolerance else 255 for delta in range(256)]
    # subtract uses saturated arithmetic: positive when first is darker
    # on the page (has a *lower* 0..255 grayscale intensity).
    current_darker = ImageChops.subtract(reference, current).point(cutoff)
    reference_darker = ImageChops.subtract(current, reference).point(cutoff)
    context = Image.blend(current, reference, .5)
    context = Image.blend(Image.new('L', current.size, 255), context, .32).convert('RGB')
    context.paste(CURRENT_COLOR, (0, 0), current_darker)
    context.paste(REFERENCE_COLOR, (0, 0), reference_darker)
    return context


def analyse(html_dir, name, tolerance, ink_cutoff, page_number=1, reference_path=None):
    key = name if page_number == 1 else f'{name}-p{page_number}'
    src = html_dir / f'{name}-{page_number}.png'
    ref = reference_path or (html_dir / f'{name}-ref.png')
    if not src.is_file() or not ref.is_file():
        raise FileNotFoundError(f'{name}: expected {src.name} and {ref.name} in {html_dir}')
    curr, bg_curr = normalize_ink(src)
    prev, bg_prev = normalize_ink(ref)
    original_sizes = (curr.size, prev.size)
    if curr.size != prev.size:
        # Different page dimensions are themselves a layout difference.
        # Extend the canvases with white; never rescale or crop engraving.
        size = (max(curr.width, prev.width), max(curr.height, prev.height))
        def pad(image):
            canvas = Image.new('L', size, 255)
            canvas.paste(image, (0, 0))
            return canvas
        curr, prev = pad(curr), pad(prev)
    curr.save(html_dir / f'{key}-normalized-current.png')
    prev.save(html_dir / f'{key}-normalized-reference.png')
    raw_diff, raw_count = difference(curr, prev, tolerance)
    raw_diff.save(html_dir / f'{key}-normalized-diff.png')
    shape_diff, shape_counts = ink_difference(curr, prev, ink_cutoff)
    shape_diff.save(html_dir / f'{key}-ink-diff.png')
    pixel_diff = directional_pixel_difference(curr, prev, tolerance)
    pixel_diff.save(html_dir / f'{key}-directional-diff.png')

    anchor_current = staff_anchor(curr)
    anchor_ref = staff_anchor(prev)
    spacings = (staff_line_spacing(curr, anchor_current), staff_line_spacing(prev, anchor_ref))
    shift = None
    aligned_count = None
    aligned_shape_counts = None
    if anchor_current and anchor_ref:
        dx = anchor_ref[0] - anchor_current[0]
        dy = anchor_ref[1] - anchor_current[1]
        # Sanity limit prevents aligning unrelated lines or distant systems.
        if abs(dx) <= curr.width // 3 and abs(dy) <= curr.height // 5:
            shift = (dx, dy)
            aligned = Image.new('L', curr.size, 255)
            aligned.paste(curr, shift)
            aligned.save(html_dir / f'{key}-aligned-current.png')
            aligned_diff, aligned_count = difference(aligned, prev, tolerance)
            aligned_diff.save(html_dir / f'{key}-aligned-diff.png')
            aligned_shape, aligned_shape_counts = ink_difference(aligned, prev, ink_cutoff)
            aligned_shape.save(html_dir / f'{key}-aligned-ink-diff.png')
            aligned_pixels = directional_pixel_difference(aligned, prev, tolerance)
            aligned_pixels.save(html_dir / f'{key}-aligned-directional-diff.png')

    return {
        'name': name, 'page': page_number, 'key': key, 'backgrounds': (bg_curr, bg_prev), 'size': curr.size, 'original_sizes': original_sizes,
        'raw_count': raw_count, 'aligned_count': aligned_count,
        'shape_counts': shape_counts, 'aligned_shape_counts': aligned_shape_counts,
        'shift': shift, 'anchors': (anchor_current, anchor_ref),
        'spacings': spacings,
    }


def load_reference_manifest(path, names, html_dir):
    """Verify the recorded Studio pages are the pages used by this report."""
    with path.open(encoding='utf-8') as stream:
        manifest = json.load(stream)
    if manifest.get('schema') != 1 or not isinstance(manifest.get('scores'), dict):
        raise ValueError(f'{path}: unsupported reference manifest')
    for name in names:
        pages = manifest.get('scores', {}).get(name, {}).get('pages')
        if not isinstance(pages, list) or not pages:
            raise ValueError(f'{path}: missing Studio pages for {name}')
        for index, info in enumerate(pages, 1):
            expected_file = f'{name}-{index}.png'
            if info.get('file') != expected_file:
                raise ValueError(f'{path}: unexpected Studio page filename for {name} page {index}')
            reference = path.parent / expected_file
            if sha256(reference) != info['sha256']:
                raise ValueError(f'{name}: Studio page {index} differs from manifest; rerun ./gen')
        if sha256(html_dir / f'{name}-ref.png') != pages[0]['sha256']:
            raise ValueError(f'{name}: page-1 report reference differs from Studio manifest')
    return manifest



def load_historical_manifest(path, names, inputs, dpi):
    """Verify all historical pages and their original (unprofiled) score hashes."""
    with path.open(encoding='utf-8') as stream:
        manifest = json.load(stream)
    if manifest.get('schema') != 1 or not isinstance(manifest.get('scores'), dict):
        raise ValueError(f'{path}: unsupported historical renderer manifest')
    if manifest.get('dpi') != dpi:
        raise ValueError('historical renderer DPI differs from Studio reference DPI')
    for name in names:
        item = manifest['scores'].get(name)
        if not isinstance(item, dict) or not item.get('pages'):
            raise ValueError(f'{name}: missing historical renderer pages')
        if item.get('source_sha256') != inputs[name]['origin_sha256']:
            raise ValueError(f'{name}: historical renderer did not use this run\'s original score')
        for page_index, info in enumerate(item['pages'], 1):
            filename = f'{name}-{page_index}.png'
            if info.get('file') != filename:
                raise ValueError(f'{name}: unexpected historical page filename')
            if sha256(path.parent / filename) != info.get('sha256'):
                raise ValueError(f'{name}: historical page {page_index} hash mismatch')
    return manifest


def historical_comparison(html_dir, key, current_path, studio_path, historical_path,
                          tolerance, ink_cutoff):
    """Three-renderer diagnostic, on one unaligned white canvas."""
    curr, _ = normalize_ink(current_path)
    studio, _ = normalize_ink(studio_path)
    old, _ = normalize_ink(historical_path)
    size = (max(image.width for image in (curr, studio, old)),
            max(image.height for image in (curr, studio, old)))
    def pad(img):
        canvas = Image.new('L', size, 255)
        canvas.paste(img, (0, 0))
        return canvas
    curr, studio, old = [pad(img) for img in (curr, studio, old)]
    old.save(html_dir / f'{key}-historical-normalized.png')
    current_mask, current_count = ink_difference(curr, old, ink_cutoff)
    current_mask.save(html_dir / f'{key}-current-vs-historical-ink.png')
    studio_mask, studio_count = ink_difference(old, studio, ink_cutoff)
    studio_mask.save(html_dir / f'{key}-historical-vs-studio-ink.png')
    _, current_pixels = difference(curr, old, tolerance)
    _, studio_pixels = difference(old, studio, tolerance)
    return {'status': 'ok', 'current_count': current_pixels,
            'studio_count': studio_pixels, 'total': size[0] * size[1],
            'current_ink': current_count, 'studio_ink': studio_count}


def sha256(path):
    hasher = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            hasher.update(chunk)
    return hasher.hexdigest()


def numbered_pages(directory, name):
    """Read only actual NAME-N.png pages, not report derivatives."""
    pattern = re.compile(re.escape(name) + r'-(\d+)\.png\Z')
    found = {int(match.group(1)): path for path in directory.iterdir()
             if (match := pattern.fullmatch(path.name)) and int(match.group(1)) > 0}
    if not found:
        return []
    if sorted(found) != list(range(1, len(found) + 1)):
        raise ValueError(f'{name}: non-contiguous PNG pages in {directory}')
    return [found[index] for index in sorted(found)]


def load_beam_manifest(path, names, inputs, dpi):
    """Verify every legacy beam page and ensure it belongs to this exact run."""
    data = json.loads(path.read_text(encoding='utf-8'))
    if data.get('schema') != 1 or not isinstance(data.get('scores'), dict):
        raise ValueError(f'{path}: unsupported beam-layout A/B manifest')
    if data.get('dpi') != dpi:
        raise ValueError('beam-layout A/B DPI differs from Studio reference DPI')
    for name in names:
        expected = inputs[name].get('beam_legacy_source_sha256') if inputs else None
        record = data['scores'].get(name)
        if bool(expected) != bool(record):
            raise ValueError(f'{name}: missing or unexpected beam-layout A/B data')
        if not record:
            continue
        if (record.get('source_sha256') != expected or
                record.get('main_source_sha256') != inputs[name]['source_sha256'] or
                record.get('beamLayout4') != 0):
            raise ValueError(f'{name}: beam-layout A/B rendered the wrong prepared input')
        pages = record.get('pages')
        if not isinstance(pages, list) or not pages:
            raise ValueError(f'{name}: beam-layout A/B generated no pages')
        for index, info in enumerate(pages, 1):
            if info.get('file') != f'{name}-{index}.png':
                raise ValueError(f'{name}: incorrect legacy beam page filename')
            if sha256(path.parent / info['file']) != info.get('sha256'):
                raise ValueError(f'{name}: legacy beam page hash mismatch')
    return data


def beam_ab_comparison(html_dir, name, key, current_path,
                       studio_path, legacy_path, tolerance, ink_cutoff):
    """Use one common absolute canvas for the three meaningful A/B metrics."""
    current, _ = normalize_ink(current_path)
    studio, _ = normalize_ink(studio_path)
    legacy, _ = normalize_ink(legacy_path)
    size = (max(current.width, studio.width, legacy.width),
            max(current.height, studio.height, legacy.height))
    def pad(image):
        if image.size == size:
            return image
        canvas = Image.new('L', size, 255)
        canvas.paste(image, (0, 0))
        return canvas
    modern, reference, old = map(pad, (current, studio, legacy))
    old.save(html_dir / f'{key}-beam-legacy-normalized.png')
    diff, switch_count = difference(modern, old, tolerance)
    diff.save(html_dir / f'{key}-beam-modern-vs-legacy-diff.png')
    modern_vs_old, _ = ink_difference(modern, old, ink_cutoff)
    modern_vs_old.save(html_dir / f'{key}-beam-modern-vs-legacy-ink.png')
    _, modern_count = difference(modern, reference, tolerance)
    _, legacy_count = difference(old, reference, tolerance)
    old_vs_ref, _ = ink_difference(old, reference, ink_cutoff)
    old_vs_ref.save(html_dir / f'{key}-beam-legacy-vs-studio-ink.png')
    return {'status': 'ok', 'switch_count': switch_count,
            'modern_count': modern_count, 'legacy_count': legacy_count,
            'total': size[0] * size[1], 'dimensions': size}


def load_baseline_manifest(path):
    with path.open(encoding='utf-8') as stream:
        manifest = json.load(stream)
    if manifest.get('schema') != 1 or not isinstance(manifest.get('scores'), dict):
        raise ValueError(f'{path}: unsupported 37E baseline manifest')
    return manifest


def pad_pair(a, b):
    size = (max(a.width, b.width), max(a.height, b.height))
    def pad(image):
        if image.size == size:
            return image
        canvas = Image.new('L', size, 255)
        canvas.paste(image, (0, 0))
        return canvas
    return pad(a), pad(b)


def baseline_comparison(html_dir, baseline_dir, name, page_number, key, baseline_info,
                        current_path, reference_path, tolerance, ink_cutoff):
    """Compare a captured 37E page with current 37E and (if present) Studio."""
    pages = baseline_info['pages']
    if page_number > len(pages):
        return {'status': 'missing recorded baseline page'}
    entry = pages[page_number - 1]
    expected_file = f'{name}-{page_number}.png'
    if entry.get('file') != expected_file:
        raise ValueError(f'{name}: inconsistent baseline page name in manifest')
    before_path = baseline_dir / expected_file
    if sha256(before_path) != entry['sha256']:
        raise ValueError(f'{name}: baseline PNG hash mismatch; recapture deliberately')
    before, _ = normalize_ink(before_path)
    now, _ = normalize_ink(current_path)
    before, now = pad_pair(before, now)
    before.save(html_dir / f'{key}-baseline-normalized.png')
    regression, regression_count = difference(now, before, tolerance)
    regression.save(html_dir / f'{key}-baseline-diff.png')
    shapes, shape_counts = ink_difference(now, before, ink_cutoff)
    shapes.save(html_dir / f'{key}-baseline-ink-diff.png')
    result = {'status': 'ok', 'before_path': before_path, 'current_count': regression_count,
              'current_total': before.width * before.height, 'shape_counts': shape_counts,
              'captured_at': baseline_info.get('captured_at', ''),
              'renderer': baseline_info.get('renderer_executable', '')}
    if reference_path is not None:
        studio, _ = normalize_ink(reference_path)
        prior, studio = pad_pair(normalize_ink(before_path)[0], studio)
        # Use the SAME canvas dimensions for both progress percentages.
        common_size = (max(prior.width, now.width), max(prior.height, now.height))
        def same_canvas(image):
            canvas = Image.new('L', common_size, 255)
            canvas.paste(image, (0, 0))
            return canvas
        prior = same_canvas(prior)
        studio = same_canvas(studio)
        current = same_canvas(normalize_ink(current_path)[0])
        prior_diff, prior_count = difference(prior, studio, tolerance)
        prior_diff.save(html_dir / f'{key}-baseline-to-studio-diff.png')
        prior_shape, _ = ink_difference(prior, studio, ink_cutoff)
        prior_shape.save(html_dir / f'{key}-baseline-to-studio-ink-diff.png')
        _, current_reference_count = difference(current, studio, tolerance)
        result['prior_reference_count'] = prior_count
        result['current_reference_count'] = current_reference_count
        result['prior_reference_total'] = common_size[0] * common_size[1]
    return result

def panel(label, filename):
    """A scrollable image preview that links to the original-resolution PNG."""
    label = escape(label)
    filename = escape(filename, quote=True)
    return (f'<div class="panel"><div class="label">{label}</div>'
            f'<div class="preview"><a href="{filename}" title="Open full-size PNG">'
            f'<img loading="lazy" src="{filename}" alt="{label}"></a></div></div>')


def reveal_source_link(result, label='Reveal file'):
    """Browser-dispatchable source selection (requires installed local handler).

    Keep report navigation links separate from file-manager links: clicking a
    score in the table must still jump to its comparison section.
    """
    origin = (result.get('input') or {}).get('origin')
    if not origin:
        return ''
    path = Path(origin)
    if not path.is_absolute() or path.suffix.lower() not in ('.mscx', '.mscz'):
        return ''
    url = 'vtest-reveal:' + path.as_uri()[len('file:'):]
    return (f' <a class="reveal-link" href="{escape(url, quote=True)}" '
            f'title="Select source score in file manager: {escape(str(path), quote=True)}">'
            f'{escape(label)}</a>')


def page(results, tolerance, reference_label, provenance=None, ink_cutoff=DEFAULT_INK_CUTOFF,
         historical=None, primary_historical=False, archive=None):
    out = ["<!doctype html><html><head><meta charset='utf-8'>",
           '<meta name="viewport" content="width=device-width, initial-scale=1">',
           '<title>MuseScore engraving comparisons</title>',
           '<style>',
           'body{font:15px/1.5 system-ui,sans-serif;margin:18px;max-width:1900px}',
           'h1{margin:0 0 12px}h2{margin:36px 0 8px}h3{margin:24px 0 8px}',
           '.panels{display:flex;flex-wrap:wrap;gap:14px;align-items:start}',
           '.panel{flex:1 1 320px;min-width:265px;max-width:570px}',
           '.preview{border:1px solid #bbb;overflow:auto;background:white;max-height:500px}',
           '.preview img{display:block;width:100%;max-width:none;height:auto}',
           '.zoomed .preview img{width:200%}',
           '.label{font-weight:650;margin-bottom:7px}.meta{color:#444;margin:8px 0}',
           '.legend{display:flex;flex-wrap:wrap;gap:16px;margin:12px 0}',
           '.key{display:inline-block;width:14px;height:14px;vertical-align:middle;'
           'border:1px solid #888;margin-right:5px}',
           '.only-current{background:rgb(190,35,47)}',
           '.only-reference{background:rgb(30,83,188)}',
           '.shared{background:rgb(75,75,75)}',
           'code{background:#eee;padding:2px 4px;overflow-wrap:anywhere}',
           '.tools{display:flex;gap:12px;flex-wrap:wrap;align-items:center;margin:12px 0}',
           'button{padding:6px 12px;cursor:pointer}',
           'details{margin:15px 0;padding:8px 12px;border:1px solid #ddd}',
           'summary{cursor:pointer;font-weight:600}',
           '.score-group{margin:18px 0;border:1px solid #ccd3dc;border-radius:5px}',
           '.score-group>summary{padding:10px 14px;background:#f4f6f9;font-size:18px}',
           '.score-group>section{padding:0 14px 12px}',
           '.summary{border-collapse:collapse;margin:12px 0;font-size:14px}',
           '.summary th,.summary td{padding:5px 10px;border:1px solid #ddd;text-align:left}',
           '.summary th{background:#f4f4f4}',
           '.notice{border-left:3px solid #888;padding:8px 12px;background:#f6f6f6}',
           '.reveal-link{margin-left:.6em;font-size:.86em;white-space:nowrap}',
           '</style></head><body>',
           '<h1>MuseScore visual comparisons</h1>',
           f'<p>Reference: <strong>{escape(reference_label)}</strong>.</p>',
           f'<p>Images normalized to white and grayscale. Fine-pixel difference tolerance: '
           f'{tolerance}/255. Shape comparison treats pixels darker than {ink_cutoff}/255 as '
           'ink; this suppresses many antialiasing variations, not all genuine engraving differences.</p>',
           '<div class="legend" role="group" aria-label="Shape comparison legend">',
           '<span><span class="key only-current"></span>37E-only ink</span>',
           f'<span><span class="key only-reference"></span>{escape(reference_label)}-only ink</span>',
           '<span><span class="key shared"></span>Shared ink</span></div>',
           '<div class="tools"><button id="zoom" type="button" '
           'onclick="toggleZoom(this)" aria-pressed="false">Magnify previews 2×</button>',
           '<span>Scroll inside a magnified panel; click its image for the original PNG.</span></div>',
           '<script>function toggleZoom(button){var active=document.body.classList.toggle("zoomed");'
           'button.textContent=active?"Fit previews":"Magnify previews 2×";'
           'button.setAttribute("aria-pressed",String(active));}</script>']
    if archive is not None:
        renderer = archive['renderer']
        statuses = archive['audit']['scores'].values()
        current_fail = sum(entry['37e_status'] != 'ok' for entry in statuses)
        missing_refs = sum(entry['reference_status'] != 'ok' for entry in statuses)
        cache_hits = sum(bool(entry.get('37e_cached')) for entry in statuses)
        # Count scores, not HTML mentions or page rows. Only valid exports from
        # both renderers count as a genuine difference in page counts.
        comparable = [entry for entry in statuses
                      if entry['37e_status'] == 'ok' and entry['reference_status'] == 'ok']
        current_extra = sum(entry['37e_pages'] > entry['reference_pages'] for entry in comparable)
        reference_extra = sum(entry['37e_pages'] < entry['reference_pages'] for entry in comparable)
        out.append('<h2>Failure-isolated catalogue audit</h2>')
        out.append(f'<p class="meta">Studio reference archive: '
                   f'<a href="{escape(archive["path"], quote=True)}">'
                   f'{escape(archive["path"])}</a>; '
                   f'reported version: <code>{escape(str(renderer.get("reported_version")))}</code>; '
                   f'binary SHA-256: <code>{escape(renderer.get("executable_sha256", "unknown"))}</code>.</p>')
        out.append(f'<p class="meta">Scores audited: {len(archive["audit"]["scores"])}; '
                   f'37E rendering failures: {current_fail}; '
                   f'37E cache hits: {cache_hits}; '
                   f'parallel jobs: {archive["audit"].get("jobs", 1)}; '
                   f'missing/stale Studio references: {missing_refs}. '
                   f'Page-count differences: {current_extra + reference_extra} scores '
                   f'(37E has extra pages: {current_extra}; '
                   f'Studio has extra pages: {reference_extra}). '
                   'A successful export does not prove that 37E retained every musical element. '
                   'Failures and render logs are listed in '
                   '<a href="catalogue-audit.json">catalogue-audit.json</a>.</p>')
    if provenance is not None:
        renderer = provenance['renderer']
        reported = renderer.get('reported_version', 'Unavailable')
        out.append(f'<p class="meta">Reference reported version: <code>{escape(reported)}</code>; '
                   f'export DPI: <strong>{provenance["dpi"]}</strong>; '
                   f'executable: <code>{escape(renderer["executable"])}</code>.</p>')
        provenance_dir = 'reference-historical' if primary_historical else 'reference-studio'
        out.append(f'<p class="meta">Full reference provenance: '
                   f'<a href="{provenance_dir}/manifest.json">'
                   f'{provenance_dir}/manifest.json</a>.</p>')
    if historical is not None:
        renderer = historical['renderer']
        out.append(f'<p class="meta">Additional historical renderer: '
                   f'<strong>{escape(renderer["label"])}</strong>; '
                   f'executable: <code>{escape(renderer["executable"])}</code>; '
                   f'reported version: <code>{escape(renderer.get("reported_version", "Unavailable"))}</code>. '
                   'It renders the original MSCX/MSCZ, without modern engraving profiles. '
                   'Differences may therefore reflect both score migration and engraving.</p>')
        out.append('<p class="meta">Historical provenance: '
                   '<a href="reference-historical/manifest.json">'
                   'reference-historical/manifest.json</a>.</p>')
    score_order = list(dict.fromkeys(r['name'] for r in results))
    # Keep stable machine IDs for cache files, anchors and manifests, but show
    # recognizable filenames for external scores in a directory report.
    labels = {name: name for name in score_order}
    for r in results:
        inp = r.get('input')
        if inp and r['name'].startswith('external-'):
            source = Path(inp['origin'])
            labels[r['name']] = f'{source.name} ({source.parent.name})'

    group_ids = {name: f'score-group-{index}' for index, name in enumerate(score_order, 1)}
    group_reports = len(score_order) > 1
    groups_start_open = len(score_order) <= 10
    if len(results) > 1:
        out.append('<nav aria-label="Test overview"><h2>Tests</h2>')
        if group_reports:
            out.append('<div class="tools" role="group" aria-label="Score section controls">'
                       '<button type="button" onclick="setScoreGroupsOpen(true)">'
                       'Expand all scores</button>'
                       '<button type="button" onclick="setScoreGroupsOpen(false)">'
                       'Collapse all scores</button></div>')
        out.append('<table class="summary"><thead><tr><th>Score / page</th>'
                   '<th>Absolute pixel differences</th><th>Aligned pixel differences</th>'
                   '<th>Registration</th><th>First-staff start Δ</th></tr></thead><tbody>')
        for r in results:
            name = escape(r['key'], quote=True)
            group_attr = f' data-score-group="{group_ids[r["name"]]}"' if group_reports else ''
            reveal = reveal_source_link(r)
            # Every summary link names its score, even when the page counts
            # differ. Avoid separate gray "group" rows that resemble categories.
            display = labels[r['name']]
            if r.get('pages_total', 1) > 1:
                display += f' — Page {r["page"]}'
            if r.get('missing') or r.get('current_only'):
                note = (r.get('missing') or r.get('current_only_note')
                        or 'Current 37E page; no reference PNG for this page')
                out.append(f'<tr><td><a href="#{name}"{group_attr}>{escape(display)}</a>{reveal}</td>'
                           f'<td colspan="4">{escape(note)}</td></tr>')
                continue
            total = r['size'][0] * r['size'][1]
            aligned = (f'{100*r["aligned_count"]/total:.2f}%'
                       if r['aligned_count'] is not None else 'N/A')
            shift = (f'{r["shift"][0]:+d}, {r["shift"][1]:+d} px'
                     if r['shift'] is not None else 'N/A')
            starts = r['anchors']
            start_delta = (f'{starts[0][0] - starts[1][0]:+d} px'
                           if all(starts) else 'N/A')
            out.append(f'<tr><td><a href="#{name}"{group_attr}>{escape(display)}</a>{reveal}</td>'
                       f'<td>{100*r["raw_count"]/total:.2f}%</td>'
                       f'<td>{aligned}</td><td>{shift}</td>'
                       f'<td>{start_delta}</td></tr>')
        out.append('</tbody></table></nav>')
    previous_group = None
    for r in results:
        if group_reports and r['name'] != previous_group:
            if previous_group is not None:
                out.append('</details>')
            previous_group = r['name']
            group_id = group_ids[r['name']]
            open_attr = ' open' if groups_start_open else ''
            page_count = r.get('pages_total', 1)
            page_label = f' · {page_count} pages' if page_count > 1 else ''
            out.append(f'<details class="score-group" id="{group_id}"{open_attr}>'
                       f'<summary>{escape(labels[r["name"]])}{page_label}</summary>')
        name = escape(r['key'], quote=True)
        page_index = r['page']
        title = escape(labels[r['name']]) + (f' — page {page_index}' if r.get('pages_total', 1) > 1 else '')
        title += reveal_source_link(r, 'Reveal source score')
        count_note = ''
        counts = r.get('page_counts')
        if page_index == 1 and counts is not None and len(set(counts.values())) > 1:
            detail = ', '.join(f'{label}: {count}' for label, count in counts.items())
            count_note = (f'<p class="notice">Page counts differ ({escape(detail)}). '
                          'Pages are matched by number only; reflow can move content '
                          'between pages and make per-page percentages misleading.</p>')
        diagnostic = ''
        if r.get('diagnostic'):
            diagnostic = (f'<p class="notice">{escape(r["diagnostic"])}</p>'
                          + (f'<p><a href="{escape(r["diagnostic_log"], quote=True)}">'
                             '37E renderer log</a></p>' if r.get('diagnostic_log') else ''))
        if r.get('missing'):
            out.append(f'<section><h2 id="{name}">{title}</h2>' + count_note + diagnostic
                       + f'<p class="notice">{escape(r["missing"])}</p>')
            if 'missing_image' in r:
                out.append('<div class="panels">')
                out.append(panel(r['missing_image_label'], r['missing_image']))
                out.append('</div>')
            out.append('</section>')
            continue
        if r.get('current_only'):
            out.append(f'<section><h2 id="{name}">{title}</h2>' + count_note + diagnostic)
            reason = r.get('current_only_note') or 'Current 37E page; no reference PNG is available for this page'
            out.append(f'<p class="notice">{escape(reason)}. '
                       'No reference-difference percentage was calculated.</p>')
            if r.get('input'):
                inp = r['input']
                out.append(f'<p class="meta">37E input: <code>{escape(inp["origin"])}</code>; '
                           f'profile: <strong>{escape(inp["profile"])}</strong>; '
                           f'SHA-256: <code>{escape(inp["source_sha256"])}</code>.</p>')
            out.append('<div class="panels">')
            out.append(panel('Current 37E · normalized', f'{name}-normalized-current.png'))
            out.append('</div>')
            if r.get('baseline'):
                baseline_info = r['baseline']
                if baseline_info['status'] == 'ok':
                    diff_pct = 100 * baseline_info['current_count'] / baseline_info['current_total']
                    out.append('<h3>Recorded 37E baseline → current 37E</h3>')
                    out.append(f'<p>Before/current differing pixels: '
                               f'<strong>{baseline_info["current_count"]:,}</strong> '
                               f'({diff_pct:.2f}%).</p>')
                    out.append('<div class="panels">')
                    out.append(panel('Before · recorded 37E', f'{name}-baseline-normalized.png'))
                    out.append(panel('Before ↔ current · directional ink',
                                     f'{name}-baseline-ink-diff.png'))
                    out.append('</div>')
                else:
                    out.append(f'<p class="notice">Baseline not comparable: '
                               f'{escape(baseline_info["status"])}</p>')
            out.append('</section>')
            continue
        w, h = r['size']
        total = w * h
        pct = 100 * r['raw_count'] / total
        only_current, only_reference = r['shape_counts']
        out.append(f'<section><h2 id="{name}">{title}</h2>' + count_note + diagnostic)
        if provenance is not None:
            item = provenance['scores'][r['name']]
            page1 = item['pages'][page_index - 1]
            out.append(f'<p class="meta">Reference input SHA-256: '
                       f'<code>{escape(item["source_sha256"])}</code>; '
                       f'Reference pages: {len(item["pages"])}; '
                       f'page {page_index}: {page1["dimensions"][0]} × {page1["dimensions"][1]} px.</p>')
        if r.get('input'):
            inp = r['input']
            out.append(f'<p class="meta">Input: <code>{escape(inp["origin"])}</code>; '
                       f'profile: <strong>{escape(inp["profile"])}</strong>; '
                       f'original SHA-256: <code>{escape(inp["origin_sha256"])}</code>; '
                       f'37E input SHA-256: <code>{escape(inp["source_sha256"])}</code>; '
                       f'Reference input SHA-256: <code>{escape(inp["origin_sha256"] if primary_historical else inp["studio_source_sha256"])}</code>. '
                       + ('Historical renderer uses the original unprofiled score. ' if primary_historical else
                          'Studio omits 37E-specific backport settings. ' if provenance and
                          inp['source_sha256'] != inp['studio_source_sha256'] else
                          '37E and Studio use the same input. ' if provenance else
                          'The historical reference was recorded separately. ') +
                       'Profile settings do not change the original score.</p>')
            if inp.get('style_overrides'):
                settings = ', '.join(f'{key}={value}' for key, value in
                                     sorted(inp['style_overrides'].items()))
                out.append(f'<p class="meta">37E style overrides: '
                           f'<code>{escape(settings)}</code></p>')
                if inp.get('37e_only_overrides') and not primary_historical:
                    out.append('<p class="meta">Studio input excludes 37E-only overrides: '
                               f'<code>{escape(str(inp["37e_only_overrides"]))}</code>.</p>')
        if 'historical' in r:
            h = r['historical']
            out.append('<h3>Additional historical MuseScore comparison</h3>')
            if h['status'] != 'ok':
                out.append(f'<p class="notice">{escape(h["status"])}</p>')
                if h.get('image'):
                    out.append('<div class="panels">')
                    out.append(panel('Historical page (unmodified score)', h['image']))
                    out.append('</div>')
            else:
                out.append(f'<p class="meta">37E vs historical: '
                           f'<strong>{100*h["current_count"]/h["total"]:.2f}%</strong>; '
                           f'historical vs Studio: '
                           f'<strong>{100*h["studio_count"]/h["total"]:.2f}%</strong>. '
                           'These pixel comparisons use a shared absolute canvas, '
                           'not measure matching. Red = first renderer only; '
                           'blue = second renderer only. Scores with different '
                           'style inputs or pagination are not controlled A/B tests.</p>')
                out.append('<div class="panels">')
                out.append(panel('Historical MuseScore · normalized',
                                 f'{name}-historical-normalized.png'))
                out.append(panel('37E ↔ historical · red 37E, blue historical',
                                 f'{name}-current-vs-historical-ink.png'))
                out.append(panel('Historical ↔ Studio · red historical, blue Studio',
                                 f'{name}-historical-vs-studio-ink.png'))
                out.append('</div>')
        if 'beam_ab' in r:
            ab = r['beam_ab']
            out.append('<h3>37E beam-layout A/B — selected automatically for this fixture</h3>')
            if ab['status'] != 'ok':
                out.append(f'<p class="notice">{escape(ab["status"])}</p>')
            else:
                total_ab = ab['total']
                legacy_pct = 100 * ab['legacy_count'] / total_ab
                modern_pct = 100 * ab['modern_count'] / total_ab
                delta = modern_pct - legacy_pct
                out.append(f'<p class="meta">37E legacy <code>beamLayout4=0</code> '
                           f'vs Studio: <strong>{legacy_pct:.2f}%</strong>; '
                           f'37E modern <code>beamLayout4=1</code> vs Studio: '
                           f'<strong>{modern_pct:.2f}%</strong>; '
                           f'modern minus legacy: <strong>{delta:+.2f} percentage points</strong>. '
                           f'Legacy vs modern: {ab["switch_count"]:,} changed pixels '
                           f'({100*ab["switch_count"]/total_ab:.2f}%). '
                           'All comparisons use the same unaligned white canvas; '
                           'lower disagreement does not prove musical correctness.</p>')
                out.append('<div class="panels">')
                out.append(panel('37E legacy · beamLayout4=0',
                                 f'{name}-beam-legacy-normalized.png'))
                out.append(panel('Modern 37E ↔ legacy 37E · red modern, blue legacy',
                                 f'{name}-beam-modern-vs-legacy-ink.png'))
                out.append(panel('Legacy 37E ↔ Studio · red legacy, blue Studio',
                                 f'{name}-beam-legacy-vs-studio-ink.png'))
                out.append('</div>')
        out.append(f'<p class="meta">Backgrounds: current {r["backgrounds"][0]}, '
                   f'reference {r["backgrounds"][1]}; '
                   f'absolute changed pixels: <strong>{r["raw_count"]:,}</strong> / '
                   f'{total:,} ({pct:.2f}%).</p>')
        if r['original_sizes'][0] != r['original_sizes'][1]:
            out.append(f'<p class="meta">Different PNG dimensions: current {r["original_sizes"][0]}, '
                       f'reference {r["original_sizes"][1]}; white canvas padding only, no scaling.</p>')
        current_anchor, reference_anchor = r['anchors']
        if current_anchor and reference_anchor:
            delta = current_anchor[0] - reference_anchor[0]
            detail = (f'First-staff extent: 37E x={current_anchor[0]}..{current_anchor[2]} px; '
                      f'{escape(reference_label)} x={reference_anchor[0]}..{reference_anchor[2]} px. '
                      f'37E staff-start offset relative to reference: <strong>{delta:+d} px</strong>.')
            current_spacing, reference_spacing = r['spacings']
            if current_spacing is not None and reference_spacing is not None:
                mean_spacing = (current_spacing + reference_spacing) / 2
                detail += (f' Staff line spacing: {current_spacing:g} px (37E), '
                           f'{reference_spacing:g} px (reference); '
                           f'offset about <strong>{delta / mean_spacing:+.2f} sp</strong>.')
            out.append(f'<p class="meta">{detail} Pixel-derived measurements only; '
                       'do not imply a particular score-style setting.</p>')
        if r.get('baseline'):
            baseline = r['baseline']
            if baseline['status'] == 'ok':
                change_pct = 100 * baseline['current_count'] / baseline['current_total']
                out.append('<h3>Recorded 37E baseline → current 37E</h3>')
                out.append(f'<p class="meta">Before/current differing pixels: '
                           f'<strong>{baseline["current_count"]:,}</strong> '
                           f'({change_pct:.2f}%). Baseline captured: '
                           f'{escape(baseline["captured_at"])}. '
                           'The baseline is not updated by normal test runs.</p>')
                if 'prior_reference_count' in baseline:
                    prior_pct = 100 * baseline['prior_reference_count'] / baseline['prior_reference_total']
                    current_pct = 100 * baseline['current_reference_count'] / baseline['prior_reference_total']
                    out.append(f'<p class="meta">Comparison with {escape(reference_label)}: '
                               f'before <strong>{prior_pct:.2f}%</strong> vs '
                               f'current <strong>{current_pct:.2f}%</strong>; '
                               f'change <strong>{current_pct-prior_pct:+.2f} percentage points</strong>. '
                               'These page-wide pixel percentages measure disagreement, not musical correctness.</p>')
                out.append('<p class="meta">Directional baseline colors: '
                           'before/current uses red = current-only and blue = baseline-only; '
                           'before/Studio uses red = baseline-only and blue = Studio-only.</p>')
                out.append('<div class="panels">')
                out.append(panel('Before · recorded 37E', f'{name}-baseline-normalized.png'))
                out.append(panel('Before ↔ current · directional ink', f'{name}-baseline-ink-diff.png'))
                if 'prior_reference_count' in baseline:
                    out.append(panel('Before ↔ reference · directional ink',
                                     f'{name}-baseline-to-studio-ink-diff.png'))
                out.append('</div>')
            else:
                out.append(f'<p class="notice">37E baseline not comparable: '
                           f'{escape(baseline["status"])}. No before/current metrics shown.</p>')
        out.append('<h3>Absolute layout — no alignment</h3>')
        out.append(f'<p class="meta">Strong-ink pixels: {only_current:,} current-only; '
                   f'{only_reference:,} reference-only. Pixel differences and ink differences '
                   'are separate measurements.</p>')
        out.append('<div class="panels">')
        out.append(panel('37E · normalized', f'{name}-normalized-current.png'))
        out.append(panel(reference_label + ' · normalized', f'{name}-normalized-reference.png'))
        out.append(panel('Shape differences (red / blue)', f'{name}-ink-diff.png'))
        out.append('</div>')
        if r['shift'] is not None:
            dx, dy = r['shift']
            ac, ar = r['aligned_shape_counts']
            out.append('<h3>First-staff aligned — diagnostic only</h3>')
            out.append(f'<p class="meta">Current translated <strong>({dx:+d}, {dy:+d}) px</strong> '
                       f'using first-staff anchors {r["anchors"][0]} vs {r["anchors"][1]}. '
                       f'Fine-pixel differences: {r["aligned_count"]:,} ({100*r["aligned_count"]/total:.2f}%). '
                       f'Strong ink: {ac:,} current-only; {ar:,} reference-only. '
                       'Registration never scales or stretches, and can hide genuine global '
                       'offsets; see the absolute section above.</p>')
            out.append('<div class="panels">')
            out.append(panel('Aligned 37E', f'{name}-aligned-current.png'))
            out.append(panel('Aligned shape differences (red / blue)', f'{name}-aligned-ink-diff.png'))
            out.append('</div>')
        else:
            out.append('<p class="meta">No reliable first-staff alignment found; '
                       'only absolute comparisons shown.</p>')
        out.append('<details><summary>Fine-pixel comparisons and original red difference masks</summary>')
        out.append('<p class="meta">Fine-pixel red means current is darker; blue means '
                   'the reference is darker by more than the tolerance. These images are '
                   'sensitive to font rasterization and anti-aliasing.</p>')
        out.append('<div class="panels">')
        out.append(panel('Absolute fine-pixel directions', f'{name}-directional-diff.png'))
        out.append(panel('Original absolute red mask', f'{name}-normalized-diff.png'))
        if r['shift'] is not None:
            out.append(panel('Aligned fine-pixel directions', f'{name}-aligned-directional-diff.png'))
            out.append(panel('Original aligned red mask', f'{name}-aligned-diff.png'))
        out.append('</div></details></section>')
    if group_reports:
        out.append('</details>')
        out.append('<script>function setScoreGroupsOpen(shouldOpen){'
                   'document.querySelectorAll("details.score-group").forEach(function(group){'
                   'group.open=shouldOpen;});}'
                   'document.querySelectorAll("[data-score-group]").forEach(function(link){'
                   'link.addEventListener("click",function(){'
                   'var group=document.getElementById(link.dataset.scoreGroup);'
                   'if(group)group.open=true;});});</script>')
    out.append('</body></html>')
    return '\n'.join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tests', nargs='*', help='e.g. tie-1 slurs-1; default: all generated PNGs')
    parser.add_argument('--html-dir', type=Path, default=Path(__file__).resolve().parent / 'html')
    parser.add_argument('--inputs-manifest', type=Path,
                        help='exact prepared MSCX inputs selected by gen')
    parser.add_argument('--scores-dir', type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument('--baseline-manifest', type=Path)
    parser.add_argument('--historical-manifest', type=Path,
                        help='optional third-renderer manifest for MuseScore 3.6.2/AppImage')
    parser.add_argument('--beam-manifest', type=Path,
                        help='optional 37E beam-layout A/B manifest')
    parser.add_argument('--reference-label', default='Historical 3.x reference')
    parser.add_argument('--reference-manifest', type=Path,
                        help='manifest from the requested reference renderer')
    parser.add_argument('--primary-historical', action='store_true',
                        help='the primary reference is the independent historical executable')
    parser.add_argument('--optional-bundled-reference', action='store_true',
                        help='allow scores without a bundled reference; show current and baseline')
    parser.add_argument('--tolerance', type=int, choices=range(0, 256), metavar='0..255', default=5)
    parser.add_argument('--ink-cutoff', type=int, choices=range(1, 256),
                        metavar='1..255', default=DEFAULT_INK_CUTOFF,
                        help='normalized grayscale cutoff for the red/blue strong-ink shape view')
    args = parser.parse_args()
    html_dir = args.html_dir.resolve()
    names = args.tests or sorted(p.name[:-len('-1.png')] for p in html_dir.glob('*-1.png'))
    if not names:
        parser.error(f'No rendered *-1.png images in {html_dir}; run ./gen first')
    try:
        inputs = load_inputs(args.inputs_manifest.resolve(), names) if args.inputs_manifest else None
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        parser.error(str(error))
    provenance = None
    label = args.reference_label
    if args.reference_manifest is not None:
        try:
            provenance = load_reference_manifest(args.reference_manifest.resolve(), names, html_dir)
            label = provenance['renderer']['label']
            if inputs:
                for name in names:
                    expected = (inputs[name]['origin_sha256'] if args.primary_historical
                                else inputs[name]['studio_source_sha256'])
                    if provenance['scores'][name]['source_sha256'] != expected:
                        raise ValueError(f'{name}: reference manifest uses a different input than this run')
        except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
            parser.error(str(error))
    historical = None
    if args.primary_historical and args.reference_manifest is None:
        parser.error('--primary-historical requires --reference-manifest')
    if args.optional_bundled_reference and args.reference_manifest is None:
        label = 'Bundled historical PNGs (when present)'
    if args.historical_manifest is not None:
        if provenance is None or inputs is None:
            parser.error('third historical renderer requires Studio and input manifests')
        try:
            historical = load_historical_manifest(args.historical_manifest.resolve(),
                                                  names, inputs, provenance['dpi'])
        except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
            parser.error(str(error))
    beam = None
    if args.beam_manifest is not None:
        if provenance is None or inputs is None:
            parser.error('beam-layout A/B requires both Studio and prepared input manifests')
        try:
            beam = load_beam_manifest(args.beam_manifest.resolve(), names, inputs,
                                      provenance['dpi'])
        except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
            parser.error(str(error))
    baseline = None
    if args.baseline_manifest:
        try:
            baseline = load_baseline_manifest(args.baseline_manifest.resolve())
        except (OSError, ValueError, json.JSONDecodeError) as error:
            parser.error(str(error))
    if baseline is not None and provenance is not None and baseline.get('dpi') != provenance.get('dpi'):
        parser.error('baseline DPI differs from Studio reference DPI; recapture at matching DPI')
    results = []
    for name in names:
        current_pages = numbered_pages(html_dir, name)
        if not current_pages:
            parser.error(f'No 37E pages found for {name}')
        if provenance is not None:
            reference_pages = [args.reference_manifest.resolve().parent / info['file']
                               for info in provenance['scores'][name]['pages']]
        else:
            # Historical 3.x references are per-fixture page 1. Never pretend
            # later pages were compared when the old suite has no reference.
            bundled = html_dir / f'{name}-ref.png'
            reference_pages = [bundled] if bundled.is_file() else []
        baseline_info = baseline['scores'].get(name) if baseline else None
        source_sha = (inputs[name]['source_sha256'] if inputs
                      else sha256(args.scores_dir.resolve() / f'{name}.mscx'))
        valid_baseline = (baseline_info is not None and
                          baseline_info.get('source_sha256') == source_sha)
        counts = {'37E current': len(current_pages)}
        if reference_pages:
            counts['Configured reference' if provenance else 'Bundled 3.x'] = len(reference_pages)
        historical_pages = ([args.historical_manifest.resolve().parent / info['file']
                             for info in historical['scores'][name]['pages']]
                            if historical is not None else [])
        if historical is not None:
            counts['Additional historical'] = len(historical_pages)
        beam_pages = ([args.beam_manifest.resolve().parent / info['file']
                       for info in beam['scores'][name]['pages']]
                      if beam is not None and name in beam['scores'] else None)
        if beam_pages is not None:
            counts['37E beam legacy'] = len(beam_pages)
        if valid_baseline:
            counts['37E baseline'] = len(baseline_info['pages'])
        total_pages = max(counts.values())
        for index in range(1, total_pages + 1):
            key = name if index == 1 else f'{name}-p{index}'
            if index > len(current_pages):
                if index <= len(reference_pages):
                    lone_page = reference_pages[index - 1]
                    page_label = 'Studio page' if provenance else 'Historical reference page'
                    message = ('No corresponding 37E page — page counts differ'
                               if current_pages else '37E did not render a page for this score')
                elif index <= len(historical_pages):
                    lone_page = historical_pages[index - 1]
                    page_label = 'Historical MuseScore page'
                    message = 'Historical MuseScore has this page; current 37E does not'
                else:
                    lone_page = args.baseline_manifest.resolve().parent / f'{name}-{index}.png'
                    page_label = 'Recorded 37E baseline page'
                    message = 'Recorded 37E baseline has this page; current 37E does not'
                results.append({'name': name, 'page': index, 'key': key,
                                'page_counts': counts, 'pages_total': total_pages,
                                'missing_image': os.path.relpath(lone_page, html_dir),
                                'missing_image_label': page_label,
                                'missing': message})
                continue
            if index > len(reference_pages):
                if provenance is None and args.optional_bundled_reference:
                    current_image, _ = normalize_ink(current_pages[index - 1])
                    current_image.save(html_dir / f'{key}-normalized-current.png')
                    r = {'name': name, 'page': index, 'key': key,
                         'pages_total': total_pages, 'page_counts': counts,
                         'current_only': True}
                    if inputs:
                        r['input'] = inputs[name]
                    if baseline_info is not None:
                        if baseline_info['source_sha256'] != source_sha:
                            r['baseline'] = {'status': 'input score SHA-256 changed since capture'}
                        else:
                            r['baseline'] = baseline_comparison(
                                html_dir, args.baseline_manifest.resolve().parent,
                                name, index, key, baseline_info,
                                current_pages[index - 1], None,
                                args.tolerance, args.ink_cutoff)
                    results.append(r)
                    print(f'{key}: current 37E only (no reference PNG)')
                    continue
                missing_label = provenance['renderer']['label'] if provenance else 'historical 3.x'
                results.append({'name': name, 'page': index, 'key': key,
                                'page_counts': counts, 'pages_total': total_pages,
                                'missing_image': os.path.relpath(current_pages[index - 1], html_dir),
                                'missing_image_label': '37E page',
                                'missing': f'37E rendered this page, but no {missing_label} reference exists'})
                continue
            try:
                r = analyse(html_dir, name, args.tolerance, args.ink_cutoff,
                            page_number=index, reference_path=reference_pages[index - 1])
                r['pages_total'] = total_pages
                if inputs:
                    r['input'] = inputs[name]
                r['page_counts'] = counts
                if historical is not None:
                    if index > len(historical_pages):
                        r['historical'] = {'status': 'Historical renderer has no corresponding page'}
                    elif len(historical_pages) != len(current_pages) or len(historical_pages) != len(reference_pages):
                        r['historical'] = {'status':
                            'Renderer page counts differ: three-way pixel percentages suppressed '
                            'because musical content may have reflowed.',
                            'image': os.path.relpath(historical_pages[index - 1], html_dir)}
                    else:
                        r['historical'] = historical_comparison(
                            html_dir, key, current_pages[index - 1],
                            reference_pages[index - 1], historical_pages[index - 1],
                            args.tolerance, args.ink_cutoff)
                if beam_pages is not None:
                    if len(beam_pages) != len(current_pages) or len(beam_pages) != len(reference_pages):
                        r['beam_ab'] = {'status':
                            'Page counts differ between beam variants or Studio; '
                            'A/B percentages suppressed because measures may have reflowed.'}
                    elif index <= len(beam_pages):
                        r['beam_ab'] = beam_ab_comparison(
                            html_dir, name, key,
                            current_pages[index - 1], reference_pages[index - 1],
                            beam_pages[index - 1], args.tolerance, args.ink_cutoff)
                if baseline_info is not None:
                    if baseline_info['source_sha256'] != source_sha:
                        r['baseline'] = {'status': 'input score SHA-256 changed since capture'}
                    else:
                        r['baseline'] = baseline_comparison(
                            html_dir, args.baseline_manifest.resolve().parent, name, index,
                            key, baseline_info, current_pages[index - 1],
                            reference_pages[index - 1] if provenance else None,
                            args.tolerance, args.ink_cutoff)
                results.append(r)
                shift = r['shift']
                print(f'{key}: absolute={r["raw_count"]:,} px, first staff shift={shift}, '
                      f'aligned={r["aligned_count"]:,} px' if shift is not None else
                      f'{key}: absolute={r["raw_count"]:,} px; no reliable staff alignment')
            except (OSError, ValueError, KeyError, TypeError) as error:
                parser.error(str(error))
    dest = html_dir / 'engraving.html'
    dest.write_text(page(results, args.tolerance, label, provenance, args.ink_cutoff,
                         historical, args.primary_historical), encoding='utf-8')
    print(f'Report: {dest}')


if __name__ == '__main__':
    main()
