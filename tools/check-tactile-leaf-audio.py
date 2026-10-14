#!/usr/bin/env python3
"""Fail a PS5 package if any Tactile Leaf v3 UI sound is missing, stale, or malformed.

Each expected SHA-256 comes from the exact 2026-10-14 approved Tactile Leaf v3 WAV.
Only Python's standard library is needed on developer machines and GitHub Actions.
"""
import hashlib
from pathlib import Path
import sys
import wave

SHA256 = {
    "back_01.wav": "8a8c572b7cb576bac40c17143226162bbecddcac1983324f44d7e1f1fa10870d",
    "error_01.wav": "f58640b95ef32a05d112abc2766cb405f45b51755cd5b75efe94735646a28edd",
    "focus_01.wav": "25e9e0a21860b2c5b5d6be03d6dbffef33f25a9ee7be8e131d8774231faa1bb1",
    "focus_02.wav": "8c6b431abff7e145bca7aa481eb409a8df8d845a16e6b660e7022811c0b0bdab",
    "launch_01.wav": "0190157b230ad33fa4ecdeea7cdcf7c506d7c769d617f2d9564df296a260c8cd",
    "modal_close_01.wav": "83fb61cfc8d7778b42e7ad01499c4dca25b10d40c9015d55a4da80ff707bedad",
    "modal_open_01.wav": "a982e234feb63224d6d9026c94ab012544408c39bcc8d49ea28add68eaa841a1",
    "notify_01.wav": "8e27e77849d36f89ca0765775dbc5bbc88513b399f7def4f6435505e18f78f0f",
    "open_01.wav": "119ce3443b24a347ba2ec7cd7b9309bc55f6e5c48ead4b2f3c1cf2aecf4cd707",
    "page_01.wav": "88be74f10e59cf9a4d1edd8b9832f7ed81a587828a2e25a7883cb528d8840bff",
    "resume_01.wav": "95ac0fdef65c5efc073e94ccbe5347a9b992c9dd5cb9d036e05fdd75fa3cb9e8",
    "saved_01.wav": "80cebdf9f62fc91ac133953eb64ea362e48afca9747e05f98cd5a845785f3303",
    "select_01.wav": "306ea1ea29fa946eb60f7ca0209c921eae7689a27b4200cb1ad64e099ea2d084",
    "slider_01.wav": "e1cd9040e38b2b7d4207d2b913a7b146b212a2591079a0035600ffb3e37f53a4",
    "slider_02.wav": "3d68e3b8adf5b22759a3a2449291e8d777e62fbff2d8c305f9449e319319ad90",
    "slider_03.wav": "f711db40740df08b9e8aabccb2e03da84c8781d02794229c4c0e89334fdd2730",
    "toggle_01.wav": "c1d99f38d5d61e26136d15bab244aefdfc45ea773f7504e1aa6f1099fb920398",
    "toggle_02.wav": "83a59b2ed464fc2b8fde820ee8fd0a85b33c5be333b9d6a64d5db282563e3d6c",
    "toggle_03.wav": "57bddd6294228b071519a9adf3033a519838ed908f684a7322684ac5a8217bfb",
    "welcome_01.wav": "42e8da6a80b1c99602510354d9a84f5c59b0b2a8f2c5b806146e6afeb9b7fa57"
}

def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check-tactile-leaf-audio.py SOUND_DIRECTORY")
    directory = Path(sys.argv[1])
    found = {p.name for p in directory.glob("*.wav")}
    expected = set(SHA256)
    if found != expected:
        raise SystemExit(f"Tactile Leaf v3 sound set mismatch; missing={sorted(expected-found)} extra={sorted(found-expected)}")
    for name, digest in SHA256.items():
        path = directory / name
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != digest:
            raise SystemExit(f"{path}: audio SHA256 does not match approved Tactile Leaf v3 clip")
        with wave.open(str(path), "rb") as audio:
            if (audio.getnchannels(), audio.getsampwidth(), audio.getframerate(), audio.getcomptype()) != (1, 2, 48000, "NONE"):
                raise SystemExit(f"{path}: expected mono 48kHz 16-bit uncompressed PCM")
            if audio.getnframes() < 100:
                raise SystemExit(f"{path}: empty or truncated WAV")
    print("Tactile Leaf v3: 20 exact approved 48kHz PCM16 WAVs verified")

if __name__ == "__main__":
    main()
