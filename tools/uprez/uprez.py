"""Local 2x texture baseline. Originals are read-only; output must be new."""
from pathlib import Path
from PIL import Image
import argparse, hashlib, json


def upscale(image):
    rgba = image.convert('RGBA')
    # Pillow RGBa is premultiplied alpha, avoiding black/hidden-RGB halos.
    return rgba.convert('RGBa').resize(
        (rgba.width * 2, rgba.height * 2), Image.Resampling.LANCZOS
    ).convert('RGBA')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(source, output):
    rows = json.loads(source.read_text())
    if output.exists():
        raise FileExistsError('Output already exists; choose a new sibling directory')
    validated = []
    targets = set()
    for row in rows:
        p = Path(row['path'])
        if digest(p) != row['sha256']:
            raise ValueError(f'Source hash mismatch: {p.name}')
        relative = Path(row['archive']) / (Path(row['texture']).stem + '.png')
        if relative.is_absolute() or '..' in relative.parts or relative in targets:
            raise ValueError('Unsafe or duplicate output path')
        targets.add(relative)
        validated.append((row, p, relative))
    output.mkdir(parents=True)
    manifest = []
    for row, p, relative in validated:
        with Image.open(p) as source_image:
            source_image.load()
            result = upscale(source_image)
            destination = output / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            result.save(destination)
            with Image.open(destination) as check:
                assert check.size == (source_image.width * 2, source_image.height * 2)
                assert check.mode == 'RGBA'
            manifest.append(dict(source_archive=row['archive'], source_member=row['texture'],
                source_sha256=row['sha256'], source_dimensions=list(source_image.size),
                output=str(relative), output_sha256=digest(destination),
                output_dimensions=list(result.size), method='2x premultiplied-alpha Lanczos',
                alpha_source=source_image.convert('RGBA').getchannel('A').getextrema(),
                alpha_output=result.getchannel('A').getextrema()))
        if digest(p) != row['sha256']:
            raise ValueError('Source changed while processing')
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2))
    return manifest


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--inventory', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    print(f'Processed {len(run(a.inventory, a.output))} original textures; no original writes.')
