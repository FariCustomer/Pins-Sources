#!/usr/bin/env python3
"""Install only the local fork's application-menu entry, without sudo."""
from pathlib import Path
import os
import subprocess

root = Path(__file__).resolve().parent
if not (root / 'build-local/pins-sources').is_file():
    raise SystemExit('Build first with python3 build-local.py.')

def quote_exec(path):
    value = str(path)
    for character in ('\\', '"', '`', '$'):
        value = value.replace(character, '\\' + character)
    return '"' + value + '"'

data_home = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share'))
applications = data_home / 'applications'
applications.mkdir(parents=True, exist_ok=True)
target = applications / 'io.github.mechamorf.PinsSources.desktop'
content = (
    '[Desktop Entry]\nVersion=1.0\nType=Application\nName=Pins Sources\n'
    'Comment=Edit application shortcuts and filter them by source\n'
    'Comment[ru]=Редактор ярлыков с фильтрами по источнику приложения\n'
    f'Exec=/usr/bin/bash {quote_exec(root / "run-local.sh")} %u\n'
    f'Icon={root / "data/icons/hicolor/scalable/apps/io.github.fabrialberio.pinapp.svg"}\n'
    'Terminal=false\nStartupNotify=true\nCategories=Utility;\n'
)
preview = root / 'build-local' / target.name
preview.write_text(content)
subprocess.run(['desktop-file-validate', str(preview)], check=True)
target.write_text(content)
print(f'Installed application-menu entry: {target}')
