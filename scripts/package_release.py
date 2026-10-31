#!/usr/bin/env python3
"""Validate version metadata and create the Arduino/PlatformIO release assets."""

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tag', help='Release tag, such as v1.0.0')
    parser.add_argument('--output', type=Path, default=ROOT / 'dist')
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'library.json').read_text())
    properties = dict(line.split('=', 1) for line in
                      (ROOT / 'library.properties').read_text().splitlines()
                      if line and not line.startswith('#'))
    version = manifest['version']
    cmake_version = re.search(r'project\([^)]*\bVERSION\s+([^\s)]+)',
                              (ROOT / 'CMakeLists.txt').read_text())[1]
    if (not re.fullmatch(r'\d+\.\d+\.\d+', version)
            or version != properties['version'] or version != cmake_version
            or manifest['name'] != properties['name']
            or (args.tag and args.tag != f'v{version}')):
        raise SystemExit('Tag, CMake, Arduino and PlatformIO version/name mismatch')
    notes = re.search(rf'(?ms)^## {re.escape(version)}[^\n]*\n(.*?)(?=^## |\Z)',
                      (ROOT / 'CHANGELOG.md').read_text())
    if not notes:
        raise SystemExit(f'Missing changelog entry for {version}')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    stem = f'INA229-SPI-Driver-{version}'
    archive = output / f'{stem}.tar.gz'
    subprocess.run([sys.executable, '-m', 'platformio', 'pkg', 'pack',
                    '--output', str(archive)], cwd=ROOT, check=True)
    zipped = output / f'{stem}.zip'
    with tarfile.open(archive) as package, zipfile.ZipFile(
            zipped, 'w', zipfile.ZIP_DEFLATED) as arduino_zip:
        members = sorted((m for m in package.getmembers() if m.isfile()),
                         key=lambda m: m.name)
        names = {str(PurePosixPath(m.name)) for m in members}
        required = {'library.json', 'library.properties', 'LICENSE',
                    'src/INA229_SPI_Driver.h', 'examples/README.md'}
        if not required <= names:
            raise SystemExit('Release archive is missing required files')
        for member in members:
            path = PurePosixPath(member.name)
            if (path.is_absolute() or '..' in path.parts or '.pio' in path.parts
                    or '.git' in path.parts or 'Zone.Identifier' in str(path)):
                raise SystemExit(f'Unexpected release member: {path}')
            arduino_zip.writestr(f'INA229_SPI_Driver/{path}',
                                package.extractfile(member).read())
    (output / 'SHA256SUMS').write_text(''.join(
        f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n'
        for path in (archive, zipped)))
    (output / 'release-notes.md').write_text(notes[1].strip() + '\n')
    print(f'Validated release v{version}: {archive.name}, {zipped.name}, SHA256SUMS')


if __name__ == '__main__':
    main()
