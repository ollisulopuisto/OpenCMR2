"""Compile icons for non-Xcode, unsigned iOS presets after bundle creation."""
import argparse
import plistlib
import subprocess
import tempfile
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--catalog', required=True)
p.add_argument('--bundle', required=True)
p.add_argument('--platform', choices=('iphoneos', 'iphonesimulator'), required=True)
p.add_argument('--minimum', required=True)
a = p.parse_args()
with tempfile.TemporaryDirectory(prefix='opencmr2-appicon-') as temporary:
    partial = Path(temporary) / 'icons.plist'
    subprocess.run(['xcrun', '--sdk', a.platform, 'actool', a.catalog,
                    '--compile', a.bundle, '--platform', a.platform,
                    '--minimum-deployment-target', a.minimum,
                    '--target-device', 'iphone', '--target-device', 'ipad',
                    '--app-icon', 'AppIcon', '--output-partial-info-plist', str(partial)], check=True)
    info = Path(a.bundle) / 'Info.plist'
    with info.open('rb') as stream:
        bundle = plistlib.load(stream)
    with partial.open('rb') as stream:
        bundle.update(plistlib.load(stream))
    with info.open('wb') as stream:
        plistlib.dump(bundle, stream)
