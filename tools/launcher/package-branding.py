#!/usr/bin/env python3
"""Generate and verify the launcher brand art from this package's PS5 icon.

The opening animation, rail icon and missing-cover fallback all read brand.tga.
Creating it from the packaged sce_sys/icon0.png prevents identity drift.
The separate forged-K badge is used only for KernelForgeLabs attribution.
"""
import argparse
import base64
import hashlib
from io import BytesIO
from pathlib import Path

from PIL import Image

FORGE_SHA256 = "b4eaa911429771c5a3983a30b0b8f3d519ad647f8821bfc2ebcb20ab7aebff04"
BRAND_SIZE = (384, 384)
FORGE_SIZE = (160, 160)


def expected_images(icon_path: Path, forge_source: Path):
    with Image.open(icon_path) as icon:
        if icon.size != (512, 512):
            raise ValueError(f"Expected packaged PS5 icon to be 512x512, got {icon.size}")
        brand = icon.convert("RGBA").resize(BRAND_SIZE, Image.Resampling.LANCZOS)
    source = base64.b64decode(forge_source.read_text(encoding="ascii").strip(), validate=True)
    if hashlib.sha256(source).hexdigest() != FORGE_SHA256:
        raise ValueError("Forge badge source checksum mismatch")
    with Image.open(BytesIO(source)) as badge:
        if badge.size != FORGE_SIZE:
            raise ValueError(f"Unexpected KernelForgeLabs badge size: {badge.size}")
        forge = badge.convert("RGBA")
    return brand, forge


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--icon", type=Path, required=True)
    parser.add_argument("--forge", type=Path, required=True)
    parser.add_argument("--art", type=Path, required=True)
    parser.add_argument("--check", action="store_true",
                        help="Verify the packaged images without changing them")
    args = parser.parse_args()
    brand, forge = expected_images(args.icon, args.forge)
    args.art.mkdir(parents=True, exist_ok=True)
    for name, expected in (("brand.tga", brand), ("kernel-forge-labs.tga", forge)):
        target = args.art / name
        if not args.check:
            expected.save(target, format="TGA", orientation=1)
        with Image.open(target) as file_image:
            actual = file_image.convert("RGBA")
            if actual.size != expected.size or actual.tobytes() != expected.tobytes():
                raise ValueError(f"Packaged branding mismatch: {target}")
    print("Branding verified: app icon = launch/rail image; forge badge = About only")


if __name__ == "__main__":
    main()
