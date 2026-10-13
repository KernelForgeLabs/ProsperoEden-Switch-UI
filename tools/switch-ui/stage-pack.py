#!/usr/bin/env python3
"""Convert a Themezer Switch pack ZIP into a page-aware ProsperoEden theme.

Runs on Linux with Pillow, not on PS5. The ZIP should contain .nxtheme
archives in the newer ZIP-based NXTheme format. Original console layout patches
are intentionally ignored; only background artwork is imported.
"""
import argparse
import io
import re
import shutil
import zipfile
from ftplib import FTP
from pathlib import Path

from PIL import Image, ImageFilter, ImageOps

TARGETS = ('home', 'all_apps', 'settings', 'user', 'player_select', 'news', 'lockscreen')
ROLES = {'home':'home', 'library':'all_apps', 'settings':'settings', 'themes':'settings',
         'profiles':'user', 'multiplayer':'home', 'rooms':'home',
         'game_details':'all_apps', 'about':'settings', 'startup':'lockscreen'}
MAX_PACK = 64 * 1024 * 1024
MAX_THEME = 18 * 1024 * 1024
MAX_IMAGE = 16 * 1024 * 1024


def safe_members(z, limit):
    members = z.infolist()
    if len(members) > limit:
        raise ValueError('Archive contains too many files')
    for info in members:
        name = info.filename.replace('\\', '/')
        if name.startswith('/') or '..' in name.split('/') or info.file_size > MAX_THEME:
            raise ValueError('Unsafe or oversized archive entry')
        if (info.external_attr >> 16) & 0o170000 == 0o120000:
            raise ValueError('Symlinks are not allowed in pack archives')
    return members


def normalize_role(text):
    v = re.sub('[^a-z0-9]', '', str(text).lower())
    if any(q in v for q in ['lockscreen', 'entrance']): return 'lockscreen'
    if any(q in v for q in ['allapps','allapp','flaunch']): return 'all_apps'
    if any(q in v for q in ['settings','setmenu']): return 'settings'
    if any(q in v for q in ['playerselection','playerselect','psl']): return 'player_select'
    if any(q in v for q in ['userpage','mypage','userpage']): return 'user'
    if any(q in v for q in ['news','newspage']): return 'news'
    if any(q in v for q in ['homemenu','home','residentmenu']): return 'home'
    return None


def read_wallpaper(nested):
    with zipfile.ZipFile(io.BytesIO(nested)) as z:
        entries = safe_members(z, 128)
        import json
        meta = {}
        for entry in entries:
            if entry.filename.lower().split('/')[-1] == 'info.json' and entry.file_size <= 16384:
                try: meta = json.loads(z.read(entry).decode('utf-8'))
                except (ValueError, UnicodeDecodeError): pass
                break
        def score(name):
            base = name.lower().split('/')[-1]
            if 'preview' in name.lower() or 'screenshot' in name.lower(): return 999
            return {'image.jpg':0,'image.jpeg':0,'background.jpg':1,
                    'background.png':2,'image.png':3,'bg.jpg':4,'bg.png':4}.get(base, 50 if base.endswith(('.jpg','.jpeg','.png')) else 999)
        images = sorted((i for i in entries if not i.is_dir() and i.file_size <= MAX_IMAGE), key=lambda i:score(i.filename))
        if not images or score(images[0].filename) >= 999:
            raise ValueError('NXTheme has no compatible image')
        payload = z.read(images[0])
        with Image.open(io.BytesIO(payload)) as img:
            img.load()
            if img.width < 500 or img.height < 280 or img.width > 4096 or img.height > 4096:
                raise ValueError('Invalid wallpaper image dimensions')
            ratio = img.width / img.height
            if not 1.2 < ratio < 2.8: raise ValueError('Unsupported wallpaper aspect ratio')
            return meta, img.convert('RGB')


def render(img, path):
    # Lanczos resize and mild sharpening improve appearance but do not restore
    # image detail that wasn't present in the Switch artwork.
    image = ImageOps.fit(img, (2048, 1152), Image.Resampling.LANCZOS)
    image = image.filter(ImageFilter.UnsharpMask(radius=1.2, percent=55, threshold=3))
    image.save(path, format='TGA')
    small = ImageOps.fit(img, (512, 288), Image.Resampling.LANCZOS)
    small = small.filter(ImageFilter.GaussianBlur(9))
    small.save(path.with_name(path.stem+'-blur.tga'), format='TGA')


def build(pack_path, pack_id, name, out):
    if not re.fullmatch(r'P[0-9A-F]{4,8}', pack_id):
        raise ValueError('Pack ID must be a P quick code such as P1948A')
    if not 1 <= len(name) <= 60 or '\n' in name or '=' in name:
        raise ValueError('Name must be 1 to 60 characters without = or newlines')
    if pack_path.stat().st_size > MAX_PACK:
        raise ValueError('Pack ZIP exceeds the 64 MB limit')
    # Pack ZIP names can vary. Read each .nxtheme and use its metadata, falling
    # back to its filename for the menu-role mapping.
    pages = {}
    with zipfile.ZipFile(pack_path) as pack:
        entries = safe_members(pack, 96)
        for info in entries:
            if not info.filename.lower().endswith('.nxtheme') or info.file_size > MAX_THEME:
                continue
            meta, image = read_wallpaper(pack.read(info))
            candidates = [meta.get(k,'') for k in ('Target','target','Menu','menu','SzsName','szsName')]
            candidates.append(info.filename)
            role = next((r for s in candidates if (r:=normalize_role(s))),None)
            if role and role not in pages:
                pages[role] = image
    if 'home' not in pages:
        raise ValueError('No Home Menu NXTheme found. Pack contents may need manual mapping.')
    target = out / 'installed' / pack_id
    if target.exists():
        raise FileExistsError(f'Existing theme not overwritten: {target}')
    target.mkdir(parents=True)
    try:
        art = target / 'art'; art.mkdir()
        page_dir = art / 'pages'; page_dir.mkdir()
        for role,img in pages.items():
            render(img, page_dir / f'{role}.tga')
        shutil.copyfile(page_dir/'home.tga',art/'backdrop.tga')
        shutil.copyfile(page_dir/'home-blur.tga',art/'backdrop-blur.tga')
        (target/'palette.ini').write_text(f'name={name}\naccent=#60d5db\n')
        (target/'pages.ini').write_text('# Screen to Switch menu mapping; safe fallback is Home Menu.\n'+
          ''.join(f'{r}={v}\n' for r,v in ROLES.items()), encoding='utf-8')
    except Exception:
        shutil.rmtree(target)
        raise
    return target, sorted(pages)


def upload(local, host, port):
    remote='/data/prosperoeden/themes/installed'
    tmp=remote+'/.pending-'+local.name
    final=remote+'/'+local.name
    ftp=FTP()
    try:
        ftp.connect(host,port,timeout=30)
        ftp.login()
        def mkdir(path):
            partial=''
            for chunk in path.strip('/').split('/'):
                partial+='/'+chunk
                try:ftp.mkd(partial)
                except Exception:
                    # Directory may already exist; FTP server decides.
                    ftp.cwd(partial)
        mkdir(remote)
        # Do not delete or overwrite an existing theme on the console.
        try:
            ftp.cwd(final)
            raise FileExistsError(f'Already installed on PS5: {final}')
        except FileExistsError: raise
        except Exception: pass
        mkdir(tmp)
        for path in sorted(local.rglob('*')):
            rel=path.relative_to(local).as_posix()
            if path.is_dir(): mkdir(tmp+'/'+rel)
            else:
                with path.open('rb') as src:
                    ftp.storbinary('STOR '+tmp+'/'+rel,src,blocksize=64*1024)
        ftp.rename(tmp,final)
    finally:
        try:ftp.quit()
        except Exception:ftp.close()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pack',required=True,type=Path,help='Themezer pack ZIP downloaded from website')
    p.add_argument('--id',required=True,help='Pack Quick ID, e.g. P1948A')
    p.add_argument('--name',required=True,help='Display name in ProsperoEden')
    p.add_argument('--out',type=Path,default=Path('switch-themes-v5.5'))
    p.add_argument('--ftp-host',help='PS5 IP (optional, otherwise stage locally)')
    p.add_argument('--ftp-port',type=int,default=2121)
    args=p.parse_args()
    target, roles = build(args.pack,args.id.upper(),args.name,args.out)
    print('Created:',target)
    print('Pack pages:',', '.join(roles))
    if args.ftp_host:
        upload(target,args.ftp_host,args.ftp_port)
        print('Installed on PS5:', '/data/prosperoeden/themes/installed/'+target.name)
    else:
        print('Use FTP to copy the folder under /data/prosperoeden/themes/installed/')

if __name__ == '__main__':main()
