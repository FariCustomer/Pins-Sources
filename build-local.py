#!/usr/bin/env python3
"""Build the native fork using GCC and installed GTK development files.

PINS_SDK_DIR optionally points to an unpacked Fedora development SDK.
"""
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / 'build-local'
BUILD.mkdir(exist_ok=True)
env = dict(os.environ)
sdk = env.get('PINS_SDK_DIR')
if sdk:
    env['PKG_CONFIG_PATH'] = ':'.join((f'{sdk}/usr/lib64/pkgconfig', f'{sdk}/usr/share/pkgconfig'))
    env['PKG_CONFIG_SYSROOT_DIR'] = sdk

def run(args):
    subprocess.run([str(arg) for arg in args], cwd=ROOT, env=env, check=True)

def flags(*packages):
    return shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', *packages], env=env, text=True))

compiler = shutil.which('glib-compile-resources')
if not compiler and sdk:
    compiler = str(Path(sdk) / 'usr/bin/glib-compile-resources')
if not compiler:
    raise SystemExit('Install glib2-devel to get glib-compile-resources.')

locale_dir = BUILD / 'locale'
for po in (ROOT / 'po').glob('*.po'):
    destination = locale_dir / po.stem / 'LC_MESSAGES' / 'pinapp.mo'
    destination.parent.mkdir(parents=True, exist_ok=True)
    run(['msgfmt', po, '-o', destination])

(BUILD / 'config.h').write_text(
    '#define PACKAGE_VERSION "2.4.8-sources.1"\n'
    '#define GETTEXT_PACKAGE "pinapp"\n'
    f'#define LOCALEDIR {json.dumps(str(locale_dir))}\n')
schemas = BUILD / 'schemas'
schemas.mkdir(exist_ok=True)
shutil.copy2(ROOT / 'data/io.github.fabrialberio.pinapp.gschema.xml', schemas)
run(['glib-compile-schemas', '--strict', schemas])
resource = BUILD / 'pins-resources.c'
run([compiler, 'src/pins.gresource.xml', '--sourcedir=src',
     '--generate-source', '--c-name=pins', f'--target={resource}'])

base = ['gcc', '-std=gnu11', '-O2', '-g', '-Wall', '-Wextra',
        '-Wno-unused-parameter', '-Wno-missing-field-initializers', '-Isrc', f'-I{BUILD}']
sources = sorted((ROOT / 'src').glob('*.c'))
gtk_flags = flags('gtk4', 'libadwaita-1')
run(base + sources + [resource, '-o', BUILD / 'pins-sources'] + gtk_flags)
run(base + ['tests/test-app-origin.c', 'src/pins-app-origin.c', '-o',
            BUILD / 'test-app-origin'] + flags('glib-2.0'))
run([BUILD / 'test-app-origin'])
if (ROOT / 'tests/test-ui.c').exists():
    run(base + [source for source in sources if source.name != 'main.c']
        + ['tests/test-ui.c', resource, '-o', BUILD / 'test-ui'] + gtk_flags)
print(f'Built: {BUILD / "pins-sources"}')
