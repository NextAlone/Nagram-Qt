"""Renders the Nagram icons from their Icon Composer sources.

macOS gets the Liquid Glass renditions of the alternate icons. Windows and
Linux get the same renditions without the lit glass rim around the icon
body; the layers keep their gradients and effects.

Run from the repository root on macOS with Xcode 26 or later:
    uv run --no-project --with pillow python \
        Telegram/Resources/branding/export_icons.py
"""

import io
import pathlib
import subprocess
import sys
import tempfile

from PIL import Image, ImageCms, ImageFilter, ImageOps

ICTOOL = pathlib.Path(
    '/Applications/Xcode.app/Contents/Applications/Icon Composer.app'
    '/Contents/Executables/ictool')
SIZE = 512
FLAT_SIZE = 1024
# The lit rim of the icon body, as a share of the icon size.
RIM = 0.035
ICONS = {
    'NagramBlock': 'block',
    'NagramBlockBlack': 'block_black',
    'NagramBlockBlue': 'block_blue',
    'NagramBlockNiello': 'block_niello',
    'NagramBlockPurple': 'block_purple',
    'NagramClassic': 'classic',
    'NagramColorful': 'colorful',
    'NagramCyan': 'cyan',
    'NagramBlack': 'black',
}
RENDITIONS = {'Default': '', 'Dark': '_dark'}
# The default icon on Windows and Linux, in Telegram/Resources/art.
ART_SIZES = (16, 32, 48, 64, 128, 256, 512)
ART_LOGOS = ('logo_256.png', 'logo_256_no_margin.png')
ICO_SIZES = (16, 32, 48, 64, 128, 256)


def render(source, rendition, size):
    with tempfile.TemporaryDirectory() as directory:
        raw = pathlib.Path(directory) / 'icon.png'
        subprocess.run([
            str(ICTOOL), str(source), '--export-image',
            '--output-file', str(raw),
            '--platform', 'macOS',
            '--rendition', rendition,
            '--width', str(size),
            '--height', str(size),
            '--scale', '1',
        ], check=True, stdout=subprocess.DEVNULL)
        image = Image.open(raw)
        profile = image.info.get('icc_profile')
        image = image.convert('RGBA')
        if profile:
            image = ImageCms.profileToProfile(
                image,
                ImageCms.ImageCmsProfile(io.BytesIO(profile)),
                ImageCms.createProfile('sRGB'),
                outputMode='RGBA')
        return image


def without_rim(image):
    # Ictool always lights the rim of the body. Paint the rim with the body
    # color found further inside, keeping the body outline.
    size = image.width
    rim = round(size * RIM)
    alpha = image.getchannel('A')
    # The body touches the canvas edges, so erode it inside a clear border.
    inner = ImageOps.expand(
        alpha.point(lambda value: 255 if value == 255 else 0),
        border=1,
        fill=0)
    for _ in range(rim):
        inner = inner.filter(ImageFilter.MinFilter(3))
    inner = inner.crop((1, 1, size + 1, size + 1))
    inner = inner.filter(ImageFilter.GaussianBlur(rim / 8))
    skip = round(rim * 1.3)
    inside = image.crop((skip, skip, size - skip, size - skip)).resize(
        (size, size), Image.LANCZOS)
    result = Image.composite(image, inside, inner)
    result.putalpha(alpha)
    return result


def render_flat(source):
    return without_rim(render(source, 'Default', FLAT_SIZE))


def save(image, target, size, root):
    if image.width != size:
        image = image.resize((size, size), Image.LANCZOS)
    image.save(target, optimize=True)
    print(target.relative_to(root))


def main():
    if not ICTOOL.exists():
        sys.exit(f'ictool not found at {ICTOOL}')
    branding = pathlib.Path(__file__).resolve().parent
    resources = branding.parent
    output = resources / 'nagram' / 'icons'
    output.mkdir(parents=True, exist_ok=True)
    for name, identifier in ICONS.items():
        source = branding / f'{name}.icon'
        if not source.is_dir():
            sys.exit(f'missing icon source {source}')
        for rendition, suffix in RENDITIONS.items():
            target = output / f'{identifier}{suffix}.png'
            save(render(source, rendition, SIZE), target, SIZE, resources)
        target = output / f'{identifier}_flat.png'
        save(render_flat(source), target, SIZE, resources)

    art = resources / 'art'
    flat = render_flat(branding / 'Nagram.icon')
    for size in ART_SIZES:
        save(flat, art / f'icon{size}.png', size, resources)
        save(flat, art / f'icon{size}@2x.png', size * 2, resources)
    for logo in ART_LOGOS:
        save(flat, art / logo, 256, resources)
    ico = art / 'icon256.ico'
    flat.save(ico, sizes=[(size, size) for size in ICO_SIZES])
    print(ico.relative_to(resources))


if __name__ == '__main__':
    main()
