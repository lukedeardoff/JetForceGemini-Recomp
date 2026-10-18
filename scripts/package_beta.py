#!/usr/bin/env python3
"""Create the owner-requested prebuilt beta; never include a ROM or asset files."""
from __future__ import annotations
import argparse, hashlib, json, re, shutil, subprocess, zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME_FILES = ('jfg-native-boot.exe', 'SDL2.dll', 'dxcompiler.dll', 'dxil.dll')
VC_RUNTIME_FILES = ('concrt140.dll', 'msvcp140.dll', 'msvcp140_1.dll', 'msvcp140_2.dll', 'msvcp140_atomic_wait.dll', 'msvcp140_codecvt_ids.dll', 'vccorlib140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll', 'vcruntime140_threads.dll')
RUNTIME_FILES = GAME_FILES + VC_RUNTIME_FILES

def runtime_inputs(runtime: Path, vc_runtime: Path | None = None) -> dict[str, Path]:
    crt = vc_runtime if vc_runtime is not None else runtime
    return {name: (crt if name in VC_RUNTIME_FILES else runtime)/name for name in RUNTIME_FILES}

def digest(path: Path) -> str:
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def runtime_manifest(runtime: Path, version: str, vc_runtime: Path | None = None) -> dict:
    entries=[]
    for name,path in runtime_inputs(runtime,vc_runtime).items():
        if not path.is_file() or path.is_symlink() or path.stat().st_size <= 0:
            raise ValueError('Incomplete runtime: '+name)
        with path.open('rb') as stream:
            if stream.read(2) != b'MZ': raise ValueError('Expected Windows executable: '+name)
        entries.append({'name':name,'size':path.stat().st_size,'sha256':digest(path)})
    return {'schema':1,'version':version,'files':entries}

def package(launcher: Path, runtime: Path, output: Path, version: str, vc_runtime: Path | None = None) -> Path:
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-beta\.[0-9]+)?',version):
        raise ValueError('Use an explicit release or beta version')
    if output.exists(): raise ValueError('Preserve existing package output; select a new destination')
    if '-beta.' not in version:
        build=json.loads((launcher.parent/'launcher-build.json').read_text(encoding='utf-8-sig'))
        source=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
        if build['source_commit'] != source or build['executable_sha256'] != digest(launcher):
            raise ValueError('Launcher build identity differs from the release source')
        if subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True).strip():
            raise ValueError('Stable releases require a clean source checkout')
        if any(digest(ROOT/name) != value for name,value in build['inputs'].items()):
            raise ValueError('Launcher source inputs changed after building')
    manifest=runtime_manifest(runtime,version,vc_runtime)
    receipt=runtime/'jfg-native-boot.exe.support'
    if not receipt.is_file(): raise ValueError('Runtime build identity is required')
    identity=dict(line.split('=',1) for line in receipt.read_text().splitlines() if '=' in line)
    if identity.get('build_runtime_sha256') != manifest['files'][0]['sha256']:
        raise ValueError('Runtime differs from its build identity')
    if identity.get('build_dirty') != '0': raise ValueError('Use a reviewed clean runtime build')
    output.mkdir(parents=True)
    files=runtime_inputs(runtime,vc_runtime)
    files.update({'JFG-Launcher.exe':launcher,'jfg-native-boot.exe.support':receipt,
                  'LICENSE':ROOT/'LICENSE','RUNTIME-NOTICES.md':ROOT/'THIRD_PARTY_NOTICES.md',
                  'THIRD-PARTY.txt':ROOT/'launcher/ui/THIRD-PARTY.txt',
                  'dependencies.lock.json':ROOT/'dependencies.lock.json', 'CHANGELOG.md':ROOT/'CHANGELOG.md'})
    for folder in ('licenses','fonts'):
        for item in (ROOT/'launcher/ui'/folder).glob('*.txt'):files['licenses/'+item.name]=item
    for relative in ('fonts/manifest.json','licenses/provenance.json'):
        files['licenses/'+('font-manifest.json' if relative.startswith('fonts') else 'provenance.json')]=ROOT/'launcher/ui'/relative
    for name in ('n64recomp','rt64','plume','cic','ares'):
        files['licenses/'+name+'-LICENSE.txt']=ROOT/'patches'/name/'LICENSE.upstream'
    for item in (ROOT/'launcher/runtime-licenses').glob('*.txt'):
        files['licenses/runtime/'+item.name]=item
    # Fixed names only: never recurse into a game build, source, profile or ROM folder.
    for name,path in files.items():
        if not path.is_file() or path.is_symlink(): raise ValueError('Missing package input: '+name)
        destination=output/name;destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(path,destination)
    (output/'jfg-package.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (output/'START HERE.txt').write_text(
        'JFG '+version+' - Windows x64\n\n'
        '1. Extract the complete ZIP to a writable folder.\n'
        '2. Open JFG-Launcher.exe.\n'
        '3. Use Select ROM to import your supported North American .z64 ROM once.\n'
        '4. Click Play. The original ROM file can be moved or deleted after import.\n\n'
        'No Git, Python, Visual Studio or WSL installation is required.\n'
        'Required Microsoft runtime libraries are included beside the game.\n'
        'Your imported ROM stays in your local profile and supplies game data. No ROM or\n'
        'game asset files are supplied. Game > Verify game files checks this package.\n'
        'Installation idea: credit to the Zelda64Recomp team and its plug-and-play\n'
        'model: https://github.com/Zelda64Recomp/Zelda64Recomp#plug-and-play\n'
        'This is an independent implementation. We do not ship game assets or ROMs.\n\n'
        'Playing requires a compatible hardware graphics device. Software-only\n'
        'virtual machines can verify installation, but cannot run the game.\n'
        'Keep all files together when moving or updating the application.\n'
        'Saves remain in your existing JFG profile.\n\n'
        'This package includes the\n'
        'compiled game program. Original project and third-party license scopes\n'
        'remain separate; see LICENSE and RUNTIME-NOTICES.md.\n',encoding='utf-8')
    members=sorted(p for p in output.rglob('*') if p.is_file())
    archive=output.parent/('JFG-'+version+'-windows-x64.zip')
    if archive.exists(): raise ValueError('Preserve the existing beta archive')
    with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for path in members:z.write(path,path.relative_to(output).as_posix())
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert set(z.namelist())=={p.relative_to(output).as_posix() for p in members}
        assert not any(Path(name).suffix.lower() in ('.z64','.n64','.v64','.pdb','.flash','.pak') for name in z.namelist())
    evidence={'version':version,'archive':archive.name,'sha256':digest(archive),'bytes':archive.stat().st_size,
              'launcher_source':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
              'runtime_source':identity.get('build_source'),'members':{p.relative_to(output).as_posix():digest(p) for p in members},
              'scope':'Prebuilt game and host components; no ROM, extracted assets, saves, or debug symbols'}
    archive.with_suffix('.inventory.json').write_text(json.dumps(evidence,indent=2)+'\n')
    archive.with_suffix('.sha256').write_text(evidence['sha256']+'  '+archive.name+'\n')
    return archive

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--launcher',required=True,type=Path)
    parser.add_argument('--runtime',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--vc-runtime',required=True,type=Path,help='x64 Microsoft.VC143.CRT from the maintainer Visual Studio redist directory')
    parser.add_argument('--version',default='1.2.0')
    args=parser.parse_args()
    print(package(args.launcher.resolve(),args.runtime.resolve(),args.output.resolve(),args.version,args.vc_runtime.resolve()))
