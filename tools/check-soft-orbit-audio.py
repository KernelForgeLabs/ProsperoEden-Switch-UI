#!/usr/bin/env python3
"""Fail a PS5 package if any Soft Orbit UI sound is missing, stale, or malformed.

Each expected SHA-256 comes from the exact 2026-10-14 approved audition WAV.
Only Python's standard library is needed on developer machines and GitHub Actions.
"""
import hashlib
from pathlib import Path
import sys
import wave

SHA256 = {
    "back_01.wav": "1e614f270fd163d61c589515e120a72ab4a6367b37f3d034b171094cac5552f9",
    "error_01.wav": "f6f31d90ac00b0ece773569392ab5432f35cc04f8684a3f0061f75700ee9d021",
    "focus_01.wav": "dc2db2a093aa3ae2a87b981c7095a158fadd1446a8e9565c2adfe673328b7c19",
    "focus_02.wav": "14a21cb66f12541b5c5ed012f42e0af78db565507e708a02d2209126e379a624",
    "launch_01.wav": "e3dba0b9545bbd4c7dd3edbb03ff328a6eb08107ce8752f86ef080e7c086c1c3",
    "modal_close_01.wav": "46151b11a5269cd0c27a13aea76ca2737f03530bd8791bbd84e924efa9c5795e",
    "modal_open_01.wav": "66d28b011c2c0dc210a987075918512b67fe2a02c88877bead37a3f76f265f93",
    "notify_01.wav": "e9fa16301ee5380217a726d52de79be03c681e2c3e6273846dc82b9eccd1872a",
    "open_01.wav": "0320a3e34a6fac1d4398492b8c5719c21bb78aded1530f63d1e30a340daf9911",
    "page_01.wav": "04a020b9469fc3bd15a47d40fa8e44431e52e62348e84b2fa7714e570bc8b055",
    "resume_01.wav": "51744dbdb68688cd4d8f658c0998ca89d8a978ea5ab150016f7a429de05349de",
    "saved_01.wav": "b15685234410f5ce8dc2ad90f6c67604e5fa313375eb6889f1af2b812751871d",
    "select_01.wav": "4f64233b4c241d3c81e313f71da8a111e1101309909a2a0dcf9b97eb1613f08f",
    "slider_01.wav": "4170a455de068568b8e4d84e8c717a0e2f1edd20a0e51cf881d0ba7ead55fddc",
    "slider_02.wav": "7dd29db864c69f74bb03a72dad5e9f597a93319bb719ff1bb51298cfd548e1db",
    "slider_03.wav": "8f80cda770d3be9b944a67edf825eafc95af47d985616c20fa5e3c9ba4a0488d",
    "toggle_01.wav": "935e53ab0d5b4130a260e7f531037203d8cb622ed926f46d44965eb02408dd5b",
    "toggle_02.wav": "085414e631c11d5dcf7b438567bb5dd23a94d01de8a98464845d5d20f4163ff9",
    "toggle_03.wav": "28ee7886cf5250706a7819fdced87b42fb96ffb81e47170930c780ca0a30fb0c",
    "welcome_01.wav": "929938f8c5df13a1006c4c2c4164a9aa6002ab1f9d9d6544256131b57a576316"
}

def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check-soft-orbit-audio.py SOUND_DIRECTORY")
    directory = Path(sys.argv[1])
    found = {p.name for p in directory.glob("*.wav")}
    expected = set(SHA256)
    if found != expected:
        raise SystemExit(f"Soft Orbit sound set mismatch; missing={sorted(expected-found)} extra={sorted(found-expected)}")
    for name, digest in SHA256.items():
        path = directory / name
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != digest:
            raise SystemExit(f"{path}: audio SHA256 does not match approved Soft Orbit clip")
        with wave.open(str(path), "rb") as audio:
            if (audio.getnchannels(), audio.getsampwidth(), audio.getframerate(), audio.getcomptype()) != (1, 2, 48000, "NONE"):
                raise SystemExit(f"{path}: expected mono 48kHz 16-bit uncompressed PCM")
            if audio.getnframes() < 100:
                raise SystemExit(f"{path}: empty or truncated WAV")
    print("Soft Orbit: 20 exact approved 48kHz PCM16 WAVs verified")

if __name__ == "__main__":
    main()
