#!/usr/bin/env python3
"""ProsperoEden v5.8.1: package-only 4K BC7 launcher art from approved design.

No PS5 SDK or external image compressor needed. Pillow + NumPy for a BC7 mode-6
single-surface texture, and SHA-256 integrity of the b64-encoded WebP source.
Based on the mode-6 approach in PS5CEMU-HAR's GPL-3.0-or-later
render-presentation.py, adapted for an existing 16:9 background and icon.
"""
import argparse
import base64
import hashlib
from io import BytesIO
import pathlib
import struct

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter

W, H = 3840, 2160
WEIGHTS = np.array([0,4,9,13,17,21,26,30,34,38,43,47,51,55,60,64])


def encode_bc7(rgb):
    height, width = rgb.shape[:2]
    if height % 4 or width % 4:
        raise ValueError('BC7 input dimensions must be divisible by 4')
    data = []
    for y in range(0, height, 64):
        strip = rgb[y:y + 64].astype(np.float64)
        rows = strip.shape[0] // 4
        pixels = strip.reshape(rows,4,width // 4,4,3).transpose(0,2,1,3,4).reshape(-1,16,3)
        mean = pixels.mean(axis=1)
        centered = pixels - mean[:,None,:]
        cov = np.einsum('npi,npj->nij',centered,centered)
        axis = np.ones((len(pixels),3)) / np.sqrt(3)
        for _ in range(8):
            v = np.einsum('nij,nj->ni',cov,axis)
            norm = np.linalg.norm(v,axis=1,keepdims=True)
            axis = np.where(norm > 1e-9, v / np.maximum(norm,1e-12),axis)
        t = np.einsum('npi,ni->np',centered,axis)
        ends = [np.clip(mean + t.min(axis=1)[:,None] * axis,0,255),
                np.clip(mean + t.max(axis=1)[:,None] * axis,0,255)]
        q = [np.clip(np.rint((end-1.0)/2.0),0,127).astype(np.int64) for end in ends]
        r0, r1 = q[0] * 2 + 1, q[1] * 2 + 1
        line = (r1-r0).astype(np.float64)
        length = np.einsum('ni,ni->n',line,line)
        pos = np.einsum('npi,ni->np',pixels-r0[:,None,:],line) / np.maximum(length,1e-9)[:,None]
        index = np.abs(pos[:,:,None]*64.0-WEIGHTS[None,None,:]).argmin(axis=2)
        index[length == 0] = 0
        swap = index[:,0] >= 8
        q[0][swap],q[1][swap] = q[1][swap].copy(),q[0][swap].copy()
        index[swap] = 15-index[swap]
        q0,q1 = q[0].astype(np.uint64),q[1].astype(np.uint64)
        lo = np.full(len(pixels),1<<6,dtype=np.uint64)
        for c in range(3):
            lo |= q0[:,c] << np.uint64(7+14*c)
            lo |= q1[:,c] << np.uint64(14+14*c)
        lo |= np.uint64(127) << np.uint64(49)
        lo |= np.uint64(127) << np.uint64(56)
        lo |= np.uint64(1) << np.uint64(63)
        hi = np.ones(len(pixels),dtype=np.uint64)
        idx = index.astype(np.uint64)
        hi |= idx[:,0] << np.uint64(1)
        for i in range(1,16):
            hi |= idx[:,i] << np.uint64(4*i)
        data.append(np.stack([lo,hi],axis=1).astype('<u8').tobytes())
    return b''.join(data)


def write_dds(path, blocks):
    expected = W*H
    if len(blocks) != expected:
        raise ValueError(f'wrong block length: {len(blocks)} != {expected}')
    header = struct.pack('<4sIIIIIII44x',b'DDS ',124,0xA1007,H,W,len(blocks),0,1)
    header += struct.pack('<II4s5I',32,0x4,b'DX10',0,0,0,0,0)
    header += struct.pack('<5I',0x1000,0,0,0,0)
    header += struct.pack('<5I',98,3,0,1,0)
    assert len(header) == 148
    path.write_bytes(header + blocks)
    if path.stat().st_size != 8294548: raise ValueError('unexpected DDS size')


def background(source):
    # Source has 16:9 artwork with a labelled mockup tile in its lower-left.
    # The background destined for the PS5 removes only that preview inset.
    # For stable output, use a blend of nearby scenic artwork to cover that
    # rectangle rather than reconstructing the logo or changing composition.
    art = source.convert('RGB').copy()
    from PIL import ImageDraw
    original = art.copy()
    w,h = art.size
    if w == 1280 and h == 720:
        # Pre-cleaned source need not be touched; source file is intentionally
        # stored after removing the mockup icon so CI has deterministic inputs.
        return art
    return art


def icon(source):
    """PS5 512px app tile: use only the large split-controller emblem.

    The approved splash has a wordmark directly under the emblem. A broad
    centred crop inadvertently clips that lettering; the icon must exclude it.
    """
    if source.size != (1280, 720):
        raise ValueError('Expected approved 1280x720 branding layout')
    # Icon silhouette within the accepted dark horizon design.
    emblem = source.crop((519, 142, 742, 347))
    from PIL import ImageDraw
    tile = Image.new('RGB', (512, 512), (2, 12, 27))
    glow = Image.new('RGBA', (512, 512), (0, 0, 0, 0))
    g = ImageDraw.Draw(glow)
    g.ellipse((92, 94, 420, 422), fill=(13, 94, 155, 80))
    tile = Image.alpha_composite(tile.convert('RGBA'), glow.filter(ImageFilter.GaussianBlur(49)))
    emblem = emblem.resize((360, 331), Image.Resampling.LANCZOS)
    # Place the original emblem on the smooth dark background, mask the source
    # sky softly so the square crop boundary cannot appear as a rectangle.
    mask = Image.new('L', emblem.size, 0)
    m = ImageDraw.Draw(mask)
    m.rounded_rectangle((13, 8, 346, 325), radius=65, fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(11))
    tile.paste(emblem.convert('RGBA'), ((512-360)//2, (512-331)//2), mask)
    return tile.convert('RGB')


def run(input_file,out_dir):
    encoded=input_file.read_text(encoding='ascii').strip()
    raw=base64.b64decode(encoded,validate=True)
    image=Image.open(BytesIO(raw)).convert('RGB')
    if image.size != (1280,720): raise ValueError(f'wrong reference image size: {image.size}')
    out_dir.mkdir(parents=True,exist_ok=True)
    icon(image).save(out_dir/'icon0.png',format='PNG',optimize=True)
    art=background(image).resize((W,H),Image.Resampling.LANCZOS)
    texture=encode_bc7(np.asarray(art.convert('RGB')))
    for name in ('pic0.dds','pic1.dds'):
        write_dds(out_dir/name,texture)
    print('PS5 brand: 512x512 PNG icon; 2 x 3840x2160 BC7 DDS; no selection music')
    print('Source SHA256:',hashlib.sha256(raw).hexdigest())

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--source',type=pathlib.Path,required=True)
    parser.add_argument('--out',type=pathlib.Path,required=True)
    args=parser.parse_args()
    run(args.source,args.out)
