import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import zipfile
from scripts import package_beta as package
from scripts import release_prebuilt as release

class PrebuiltReleaseTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name); runtime = self.root / 'runtime'; runtime.mkdir()
        launcher = self.root / 'launcher.exe'; launcher.write_bytes(b'MZsynthetic-launcher')
        for name in package.RUNTIME_FILES: (runtime / name).write_bytes(b'MZsynthetic-runtime')
        (runtime / 'jfg-native-boot.exe.support').write_text('build_dirty=0\nbuild_source=fixture\nbuild_runtime_sha256=' + package.digest(runtime / 'jfg-native-boot.exe') + '\n')
        self.version = '1.1.0-beta.1'
        self.archive = package.package(launcher, runtime, self.root / 'bundle', self.version)
        self.inventory_path = self.archive.with_suffix('.inventory.json')
        self.inventory = json.loads(self.inventory_path.read_text())
        self.metadata = {'version': self.version, 'commit': self.inventory['launcher_source']}
        self.runtime = runtime; self.launcher = launcher

    def test_complete_playable_bundle(self):
        self.assertEqual(len(release.verify_bundle(self.root, self.metadata)), 3)

    def test_wrong_source_and_version_rejected(self):
        with self.assertRaisesRegex(ValueError, 'source or version'):
            release.verify_bundle(self.root, {**self.metadata, 'commit': '0' * 40})

    def test_bad_checksum_rejected(self):
        self.archive.with_suffix('.sha256').write_text('0' * 64 + '  ' + self.archive.name)
        with self.assertRaisesRegex(ValueError, 'checksum'):
            release.verify_bundle(self.root, self.metadata)

    def rewrite(self, change):
        with zipfile.ZipFile(self.archive) as z: files = {name: z.read(name) for name in z.namelist()}
        change(files)
        with zipfile.ZipFile(self.archive, 'w') as z:
            for name, data in files.items(): z.writestr(name, data)
        self.inventory['members'] = {name: hashlib.sha256(data).hexdigest() for name, data in files.items()}
        self.inventory['sha256'] = package.digest(self.archive)
        self.inventory_path.write_text(json.dumps(self.inventory))
        self.archive.with_suffix('.sha256').write_text(self.inventory['sha256'] + '  ' + self.archive.name + '\n')

    def test_launcher_only_package_rejected(self):
        self.rewrite(lambda files: files.pop('jfg-native-boot.exe'))
        with self.assertRaisesRegex(ValueError, 'playable runtime'):
            release.verify_bundle(self.root, self.metadata)

    def test_game_asset_or_rom_rejected_even_with_updated_inventory(self):
        self.rewrite(lambda files: files.update({'game.z64': b'PRIVATE-CANARY'}))
        with self.assertRaisesRegex(ValueError, 'Unexpected bundle member'):
            release.verify_bundle(self.root, self.metadata)

    def test_runtime_manifest_tampering_rejected(self):
        def change(files):
            manifest = json.loads(files['jfg-package.json']); manifest['files'][0]['size'] += 1
            files['jfg-package.json'] = json.dumps(manifest).encode()
        self.rewrite(change)
        with self.assertRaisesRegex(ValueError, 'digest mismatch'):
            release.verify_bundle(self.root, self.metadata)

    def test_stable_package_requires_clean_launcher_identity(self):
        receipt = self.launcher.parent / 'launcher-build.json'
        receipt.write_text(json.dumps({'source_commit': 'fixture', 'executable_sha256': package.digest(self.launcher), 'inputs': {}}))
        def git(args, **kwargs): return 'fixture' if args[1] == 'rev-parse' else ''
        with mock.patch.object(package.subprocess, 'check_output', side_effect=git):
            archive = package.package(self.launcher, self.runtime, self.root / 'stable', '1.1.0')
        self.assertTrue(archive.is_file())
        with mock.patch.object(package.subprocess, 'check_output', side_effect=['fixture', ' M changed.cs']):
            with self.assertRaisesRegex(ValueError, 'clean source'):
                package.package(self.launcher, self.runtime, self.root / 'dirty', '1.1.0')


class DraftPublicationTests(unittest.TestCase):
    def publish(self, draft=True, assets=None, tag='v1.1.0'):
        metadata = {'tag': 'v1.1.0', 'version': '1.1.0', 'commit': 'fixture', 'prerelease': False}
        paths = [Path('bundle.zip'), Path('bundle.sha256'), Path('bundle.inventory.json')]
        response = {'isDraft': draft, 'tagName': tag, 'url': 'https://example.invalid/release',
                    'assets': assets if assets is not None else [{'name': p.name} for p in paths]}
        with mock.patch('sys.argv', ['release_prebuilt.py', 'publish', '--output', '.']), \
             mock.patch.dict(release.os.environ, {'GITHUB_REPOSITORY': 'owner/repo'}), \
             mock.patch.object(release, 'release_metadata', return_value=metadata), \
             mock.patch.object(release, 'verify_bundle', return_value=paths), \
             mock.patch.object(release.subprocess, 'check_output', return_value=json.dumps(response)) as lookup, \
             mock.patch.object(release.subprocess, 'run') as edit:
            try:
                result = release.main()
            except ValueError:
                edit.assert_not_called()
                raise
            self.assertEqual(result, 0)
            self.assertEqual(lookup.call_args.args[0][:4], ['gh', 'release', 'view', 'v1.1.0'])
            self.assertIn('--draft=false', edit.call_args.args[0])

    def test_authenticated_draft_lookup_publishes_verified_bundle(self):
        self.publish()

    def test_public_release_cannot_be_overwritten(self):
        with self.assertRaisesRegex(ValueError, 'already public'):
            self.publish(draft=False)

    def test_different_assets_or_tag_cannot_be_published(self):
        with self.assertRaisesRegex(ValueError, 'assets differ'):
            self.publish(assets=[])
        with self.assertRaisesRegex(ValueError, 'tag differs'):
            self.publish(tag='v9.0.0')

if __name__ == '__main__': unittest.main()
