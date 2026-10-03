#!/usr/bin/env python3
"""Deploy Qt and recursively resolve non-system Windows runtime DLLs."""
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import urllib.request

root = Path(__file__).resolve().parent.parent
stage = root / 'build/windows-package'
output = root / 'build/windows-output'
if stage.exists():
    shutil.rmtree(stage)
(stage / 'bin').mkdir(parents=True)
output.mkdir(parents=True, exist_ok=True)
exe = stage / 'bin/Aurora-JTTY.exe'
shutil.copy2(root / 'build/windows-build/wsjtx.exe', exe)
deploy = shutil.which('windeployqt-qt5') or shutil.which('windeployqt')
if not deploy:
    raise RuntimeError('Qt Windows deployment tool not found')
subprocess.run([deploy, '--release', '--no-translations', '--no-opengl-sw', '--no-angle', '--no-system-d3d-compiler', str(exe)], check=True)
# Aurora uses only SQLite; other Qt database plugins require unrelated servers.
for plugin in (stage / 'bin/sqldrivers').glob('*.dll'):
    if plugin.name.lower() != 'qsqlite.dll':
        plugin.unlink()
# Preserve the upstream resource-directory relationship to the executable.
resources = stage / 'share/wsjtx'
resources.mkdir(parents=True)
for name in ('cty.dat', 'cty.dat_copyright.txt', 'ALLCALL7.TXT', 'CALL3.TXT', 'grid.dat'):
    for base in (root / 'build/wsjtx-jtty-source', root / 'build/windows-build'):
        if (base / name).exists():
            shutil.copy2(base / name, resources / name)
            break
mingw = Path(subprocess.check_output(['cygpath', '-w', '/mingw64'], text=True).strip())
pacman_db = Path(subprocess.check_output(['cygpath', '-w', '/var/lib/pacman/local'], text=True).strip())
shutil.copytree(root / 'build/wsjtx-jtty-source/Palettes', resources / 'Palettes', dirs_exist_ok=True)
prefixes = [root / 'build/hamlib-prefix/bin', mingw / 'bin']
search = {p.name.lower(): p for base in prefixes for p in base.glob('*.dll')}
system = Path('C:/Windows/System32')
origins = set()
seen = set()
while True:
    binaries = [p for p in stage.rglob('*') if p.suffix.lower() in ('.dll', '.exe') and p not in seen]
    if not binaries:
        break
    for binary in binaries:
        seen.add(binary)
        imports = re.findall(r'DLL Name:\s*(\S+)', subprocess.check_output(['objdump', '-p', str(binary)], text=True))
        for name in imports:
            lowered = name.lower()
            if lowered in search:
                origin = search[lowered]
                origins.add(origin)
                target = stage / 'bin' / origin.name
                if not target.exists():
                    shutil.copy2(origin, target)
            elif lowered.startswith(('api-ms-', 'ext-ms-')) or (system / name).exists():
                continue
            else:
                raise RuntimeError(f'Unresolved dependency: {binary.name} -> {name}')
# Associate deployed MSYS2 DLLs with package versions and their source archives.
packages = set()
for path in origins:
    if '/hamlib-prefix/' in path.as_posix():
        continue
    result = subprocess.run(['pacman', '-Qqo', str(path)], text=True, capture_output=True, check=True)
    packages.add(result.stdout.strip())
licenses = stage / 'licenses'
licenses.mkdir()
shutil.copy2(root / 'upstream/wsjtx/contrib/QDarkStyleSheet/LICENSE.md', licenses / 'QDarkStyleSheet-LICENSE.md')
shutil.copy2(root / 'upstream/wsjtx/cty.dat_copyright.txt', licenses / 'cty.dat_copyright.txt')
for name in ('COPYING', 'THIRD_PARTY_NOTICES.md', 'docs/PUBLICATION-REQUIREMENTS.md'):
    shutil.copy2(root / name, licenses / Path(name).name)
for path in (root / 'build/hamlib-src').glob('COPYING*'):
    if path.is_file():
        shutil.copy2(path, licenses / ('Hamlib-' + path.name))
shutil.copy2(root / 'docs/WINDOWS-INSTALL.md', stage / 'README.md')
shutil.copy2(root / 'native/Assets/Aurora-JTTY.ico', stage / 'Aurora-JTTY.ico')
source_lines = []
for package in sorted(packages):
    metadata_dirs = list(pacman_db.glob(package + '-*'))
    metadata = (metadata_dirs[0] / 'desc').read_text()
    def field(name):
        match = re.search('%' + name + '%\n([^\n]+)', metadata)
        return match.group(1) if match else ''
    base = field('BASE') or field('NAME').replace('mingw-w64-x86_64-', 'mingw-w64-')
    version = field('VERSION')
    url = f'https://mirror.msys2.org/mingw/sources/{base}-{version}.src.tar.zst'
    source_lines.append(f'- {package} {version}: {url}')
    for candidate in ((mingw / 'share/licenses') / package, (mingw / 'share/licenses') / package.replace('mingw-w64-x86_64-', '')):
        if candidate.is_dir():
            shutil.copytree(candidate, licenses / package, dirs_exist_ok=True)
    # Some packages place notices outside share/licenses (for example ICU).
    package_files = subprocess.check_output(['pacman', '-Ql', package], text=True)
    for line in package_files.splitlines():
        installed = line.partition(' ')[2]
        if not installed.startswith('/mingw64/') or installed.endswith('/'):
            continue
        if not re.search(r'(?:license|copying|copyright)', Path(installed).name, re.I):
            continue
        notice = mingw / installed.removeprefix('/mingw64/')
        if notice.is_file():
            destination = licenses / package / notice.name
            destination.parent.mkdir(exist_ok=True)
            shutil.copy2(notice, destination)
(licenses / 'DEPENDENCY-SOURCES.md').write_text('# Bundled dependency source packages\n\nExact MSYS2 source packages corresponding to bundled DLLs:\n\n' + '\n'.join(source_lines) + '\n\nHamlib 4.7.2 source and build scripts are included in Aurora-JTTY-1.0-windows-source.tar.gz.\n')
# Complete Aurora and Hamlib source, with build scripts and unmodified upstream files.
with tarfile.open(output / 'Aurora-JTTY-1.0-windows-source.tar.gz', 'w:gz') as archive:
    names = subprocess.check_output(['git', 'ls-files', '-z'], cwd=root).decode().split('\0')
    for name in names:
        path = root / name
        if name and path.is_file() and not name.startswith('installers/'):
            archive.add(path, arcname='Aurora-JTTY-1.0/' + name, recursive=False)
    for directory in ('upstream/wsjtx', 'build/hamlib-src'):
        for path in (root / directory).rglob('*'):
            rel = path.relative_to(root / directory)
            if not path.is_file() or '.git' in rel.parts or path.suffix.lower() in ('.dll', '.exe', '.o', '.a', '.lo', '.la'):
                continue
            if directory == 'upstream/wsjtx' and rel.parts[0] == 'samples' and rel.parts[1] != 'JTTY':
                continue
            archive.add(path, arcname='Aurora-JTTY-1.0/' + directory + '/' + str(rel), recursive=False)
# Fetch dependency source packages alongside binaries, rather than relying on mutable links.
source_out = output / 'dependency-sources'
source_out.mkdir(exist_ok=True)
for line in source_lines:
    url = line.split()[-1]
    destination = source_out / url.rsplit('/', 1)[-1]
    if not destination.exists():
        urllib.request.urlretrieve(url, destination)
print(f'Deployed {len(seen)} Windows binaries; corresponding dependency sources: {len(packages)} packages.')
