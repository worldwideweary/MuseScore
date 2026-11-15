#!/usr/bin/env python3
"""Reveal a score in its file manager from a vtest HTML report.

Browser pages cannot call D-Bus/Finder/Explorer directly. Install this
vtest-reveal: URI handler ONCE with --install (user account only), then score
links in the HTML report can request native file-manager selection.
No shell commands are constructed from the URL, and only local MSCX/MSCZ
files can be revealed.
"""
import argparse
import os
from pathlib import Path
import plistlib
import subprocess
import sys
from urllib.parse import urlsplit
from urllib.request import url2pathname

SCHEME = 'vtest-reveal'
DESKTOP_NAME = 'musescore-vtest-reveal.desktop'


def score_uri(path):
    """Encode a score filename as an absolute URL without losing %, # or spaces."""
    path = Path(path).expanduser().resolve()
    return SCHEME + ':' + path.as_uri()[len('file:'):]


def parse_score_uri(uri):
    parsed = urlsplit(uri)
    if (parsed.scheme.lower() != SCHEME or parsed.netloc or parsed.query or
            parsed.fragment or not parsed.path.startswith('/') or
            parsed.path.startswith('//')):
        raise ValueError('not a local vtest-reveal score URL')
    path = Path(url2pathname(parsed.path)).expanduser().resolve()
    if not path.is_file() or path.suffix.lower() not in ('.mscx', '.mscz'):
        raise ValueError(f'not an existing MSCX/MSCZ file: {path}')
    return path


def reveal(path):
    """Select a file, not open it in MuseScore. Linux fallback opens its folder."""
    if sys.platform == 'darwin':
        subprocess.run(['open', '-R', str(path)], check=True)
    elif os.name == 'nt':
        subprocess.Popen(['explorer.exe', '/select,' + str(path)])
    else:
        # Freedesktop's cross-desktop FileManager1 interface selects the file.
        # Use --print-reply so a missing/unavailable service is detectable.
        cmd = ['dbus-send', '--session', '--print-reply', '--reply-timeout=3000',
               '--dest=org.freedesktop.FileManager1', '--type=method_call',
               '/org/freedesktop/FileManager1', 'org.freedesktop.FileManager1.ShowItems',
               'array:string:' + path.as_uri(), 'string:']
        try:
            subprocess.run(cmd, check=True, timeout=6,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except (FileNotFoundError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
            print('FileManager1 unavailable; opening containing folder instead.', file=sys.stderr)
            subprocess.run(['xdg-open', str(path.parent)], check=True)


def desktop_quote(value):
    # Desktop Entry Exec quoting, not shell quoting.
    return '"' + value.replace('\\', '\\\\').replace('"', '\\"').replace('`', '\\`').replace('$', '\\$') + '"'


def install_linux():
    application_dir = Path.home() / '.local/share/applications'
    application_dir.mkdir(parents=True, exist_ok=True)
    desktop = application_dir / DESKTOP_NAME
    script = Path(__file__).resolve()
    cmd = f'{desktop_quote(sys.executable)} {desktop_quote(str(script))} %u'
    desktop.write_text('[Desktop Entry]\nType=Application\nName=MuseScore vtest Reveal\n'
                       'NoDisplay=true\nTerminal=false\n'
                       'MimeType=x-scheme-handler/vtest-reveal;\n'
                       f'Exec={cmd}\n', encoding='utf-8')
    subprocess.run(['xdg-mime', 'default', DESKTOP_NAME,
                    'x-scheme-handler/vtest-reveal'], check=True)
    print('Registered vtest-reveal: handler:', desktop)
    print('Browser may ask permission to open the handler the first time.')


def install_windows():
    import winreg
    # Per-user registration; no administrator privileges required.
    key_path = r'Software\Classes\vtest-reveal'
    with winreg.CreateKey(winreg.HKEY_CURRENT_USER, key_path) as key:
        winreg.SetValueEx(key, '', 0, winreg.REG_SZ, 'URL:MuseScore vtest Reveal')
        winreg.SetValueEx(key, 'URL Protocol', 0, winreg.REG_SZ, '')
    with winreg.CreateKey(winreg.HKEY_CURRENT_USER,
                          key_path + r'\shell\open\command') as key:
        command = f'"{sys.executable}" "{Path(__file__).resolve()}" "%1"'
        winreg.SetValueEx(key, '', 0, winreg.REG_SZ, command)
    print('Registered vtest-reveal: for the current Windows user.')


def install_macos():
    # Launch Services requires an application bundle for custom URI schemes.
    # Compile a small AppleScript application whose URL handler calls this
    # script with the original URL as one safely quoted process argument.
    app = Path.home() / 'Applications' / 'MuseScoreVtestReveal.app'
    app.parent.mkdir(parents=True, exist_ok=True)
    py = str(Path(sys.executable).resolve()).replace('\\', '\\\\').replace('"', '\\"')
    script = str(Path(__file__).resolve()).replace('\\', '\\\\').replace('"', '\\"')
    applescript = ('on open location theURL\n'
                   f'  do shell script (quoted form of "{py}") & " " & '
                   f'(quoted form of "{script}") & " " & (quoted form of theURL)\n'
                   'end open location\n')
    subprocess.run(['osacompile', '-o', str(app), '-e', applescript], check=True)
    plist = app / 'Contents/Info.plist'
    with plist.open('rb') as stream:
        values = plistlib.load(stream)
    values['CFBundleIdentifier'] = 'org.musescore.vtest.reveal'
    values['CFBundleURLTypes'] = [{'CFBundleURLName': 'MuseScore vtest Reveal',
                                   'CFBundleURLSchemes': [SCHEME]}]
    values['LSUIElement'] = True
    with plist.open('wb') as stream:
        plistlib.dump(values, stream)
    registration = Path('/System/Library/Frameworks/CoreServices.framework/Frameworks/'
                        'LaunchServices.framework/Support/lsregister')
    subprocess.run([str(registration), '-f', str(app)], check=True)
    print('Registered vtest-reveal: for the current macOS user:', app)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', action='store_true', help='install the local URL handler once')
    parser.add_argument('uri', nargs='?', help='vtest-reveal:///absolute/path/to/score.mscz')
    args = parser.parse_args()
    if args.install:
        if args.uri:
            parser.error('do not supply a score URI with --install')
        if sys.platform == 'darwin':
            install_macos()
        elif os.name == 'nt':
            install_windows()
        else:
            install_linux()
        return
    if not args.uri:
        parser.error('a score URI is required (or use --install)')
    try:
        reveal(parse_score_uri(args.uri))
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, f'Could not reveal score: {error}\n')


if __name__ == '__main__':
    main()
