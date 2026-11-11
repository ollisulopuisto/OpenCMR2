"""Resize existing project art; no new artwork. Requires Pillow."""
import json
from pathlib import Path
from PIL import Image
root = Path(__file__).resolve().parents[1]
catalog = root / 'res/Assets.xcassets'
destination = catalog / 'AppIcon.appiconset'
destination.mkdir(parents=True, exist_ok=True)
source = Image.open(root / 'res/opencmr2.png').convert('RGB')
slots = [('iphone', s, k) for s in (20, 29, 40, 60) for k in (2, 3)]
slots += [('ipad', s, k) for s in (20, 29, 40, 76) for k in (1, 2)]
slots += [('ipad', 83.5, 2), ('ios-marketing', 1024, 1)]
images = []
for idiom, size, scale in slots:
    pixels = round(size * scale)
    filename = f'icon-{pixels}.png'
    source.resize((pixels, pixels), Image.Resampling.LANCZOS).save(destination / filename)
    images.append(dict(idiom=idiom, size=f'{size}x{size}', scale=f'{scale}x', filename=filename))
(destination / 'Contents.json').write_text(json.dumps(dict(images=images, info=dict(author='xcode', version=1)), indent=2) + '\n')
(catalog / 'Contents.json').write_text(json.dumps(dict(info=dict(author='xcode', version=1)), indent=2) + '\n')
