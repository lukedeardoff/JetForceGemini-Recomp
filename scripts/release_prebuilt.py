#!/usr/bin/env python3
"""Verify a maintainer-built ROM-required bundle before publishing its draft."""
from __future__ import annotations
import argparse, hashlib, json, os, subprocess, zipfile
from pathlib import Path
try:
    from .package_beta import RUNTIME_FILES
    from .release_launcher import release_metadata
except ImportError:
    from package_beta import RUNTIME_FILES
    from release_launcher import release_metadata
ROOT = Path(__file__).resolve().parents[1]

def verify_bundle(output: Path, metadata: dict) -> list[Path]:
    stem = 'JFG-' + metadata['version'] + '-windows-x64'
    archive = output / (stem + '.zip')
    inventory_path = output / (stem + '.inventory.json')
    checksum = output / (stem + '.sha256')
    inventory = json.loads(inventory_path.read_text(encoding='utf-8'))
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if inventory['version'] != metadata['version'] or inventory['launcher_source'] != metadata['commit']:
        raise ValueError('Bundle source or version differs from the release tag')
    if inventory['sha256'] != digest or checksum.read_text().strip() != digest + '  ' + archive.name:
        raise ValueError('Bundle checksum mismatch')
    with zipfile.ZipFile(archive) as z:
        names = z.namelist()
        if len(names) != len(set(names)) or set(names) != set(inventory['members']) or z.testzip():
            raise ValueError('Bundle membership or ZIP integrity mismatch')
        required = {'JFG-Launcher.exe', 'jfg-package.json', 'jfg-native-boot.exe.support',
                    'LICENSE', 'RUNTIME-NOTICES.md', 'THIRD-PARTY.txt', 'START HERE.txt',
                    'CHANGELOG.md', 'dependencies.lock.json', *RUNTIME_FILES}
        if not required <= set(names):
            raise ValueError('Bundle is missing the playable runtime or release documentation')
        for name in names:
            if name not in required and not (name.startswith('licenses/') and name.endswith(('.txt', '.json'))):
                raise ValueError('Unexpected bundle member: ' + name)
            if '\\' in name or any(p in ('', '.', '..') for p in name.split('/')):
                raise ValueError('Unsafe bundle path')
            if hashlib.sha256(z.read(name)).hexdigest() != inventory['members'][name]:
                raise ValueError('Member hash mismatch: ' + name)
        manifest = json.loads(z.read('jfg-package.json'))
        if manifest['schema'] != 1 or manifest['version'] != metadata['version']:
            raise ValueError('Runtime manifest version mismatch')
        if len(manifest['files']) != len(RUNTIME_FILES) or {e['name'] for e in manifest['files']} != set(RUNTIME_FILES):
            raise ValueError('Runtime manifest does not include all required libraries')
        for entry in manifest['files']:
            data = z.read(entry['name'])
            if not data.startswith(b'MZ') or len(data) != entry['size'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
                raise ValueError('Runtime manifest digest mismatch')
        identity = dict(line.split('=', 1) for line in z.read('jfg-native-boot.exe.support').decode().splitlines() if '=' in line)
        if identity.get('build_dirty') != '0' or identity.get('build_runtime_sha256') != inventory['members']['jfg-native-boot.exe'] or identity.get('build_source') != inventory['runtime_source']:
            raise ValueError('Runtime build identity mismatch')
    return [archive, inventory_path, checksum]

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('verify', 'publish'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    metadata = release_metadata(ROOT, os.environ.get('RELEASE_TAG', ''), os.environ.get('RELEASE_REF_TYPE', ''))
    assets = verify_bundle(args.output, metadata)
    if args.action == 'publish':
        repo = os.environ['GITHUB_REPOSITORY']
        # The REST tag endpoint excludes drafts. The CLI resolves draft releases
        # by tag through its authenticated release listing.
        release = json.loads(subprocess.check_output(
            ['gh', 'release', 'view', metadata['tag'], '--repo', repo,
             '--json', 'isDraft,assets,url,tagName'], text=True))
        if release['tagName'] != metadata['tag']:
            raise ValueError('Draft tag differs from the verified source')
        if {a['name'] for a in release['assets']} != {p.name for p in assets}:
            raise ValueError('Draft assets differ from the verified bundle')
        if not release['isDraft']:
            raise ValueError('Release is already public; do not overwrite it')
        subprocess.run(['gh', 'release', 'edit', metadata['tag'], '--repo', repo,
                        '--draft=false', '--prerelease=' + ('true' if metadata['prerelease'] else 'false'), '--latest=' + ('false' if metadata['prerelease'] else 'true')], check=True)
        print(release['url'])
    else:
        print(json.dumps({'verified': True, 'tag': metadata['tag'], 'assets': [p.name for p in assets]}))
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
