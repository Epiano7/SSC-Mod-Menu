"""Local sound preparation only; does not hook playback or modify game assets.

Python standard library. PCM WAV import supports 8/16/24/32-bit mono/stereo.
Other encodings are explicitly rejected pending a packaged decoder.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import wave


def read_pcm(path):
    if Path(path).stat().st_size > 64 * 1024 * 1024:
        raise ValueError("Sound exceeds the 64 MiB import limit")
    with wave.open(str(path), "rb") as source:
        channels, width, rate, frames = (source.getnchannels(), source.getsampwidth(),
                                         source.getframerate(), source.getnframes())
        if source.getcomptype() != "NONE" or channels not in (1, 2) or width not in (1, 2, 3, 4):
            raise ValueError("Use an uncompressed PCM WAV, mono or stereo, 8/16/24/32 bit")
        if not 8000 <= rate <= 192000 or frames == 0 or frames * channels * width > 64 * 1024 * 1024:
            raise ValueError("Invalid or oversized audio stream")
        raw = source.readframes(frames)
    if len(raw) != frames * channels * width:
        raise ValueError("Truncated WAV data")
    scale = float(1 << (width * 8 - 1))
    if width == 1:
        samples = [(v - 128) / 128.0 for v in raw]
    else:
        samples = [int.from_bytes(raw[i:i + width], "little", signed=True) / scale
                   for i in range(0, len(raw), width)]
    return channels, rate, samples


def rms(samples):
    return math.sqrt(sum(v * v for v in samples) / len(samples))


def db(value):
    return 20 * math.log10(value) if value > 0 else None


def prepare(reference, replacement, output):
    reference, replacement, output = map(lambda p: Path(p).resolve(), (reference, replacement, output))
    if output in (reference, replacement) or output.is_relative_to(reference.parent):
        raise ValueError("Write to a separate mod-workspace folder, outside the original sound directory")
    rc, rr, original = read_pcm(reference)
    ic, ir, incoming = read_pcm(replacement)
    # Avoid a low-quality implicit resampler or stereo downmix: preserve the reference format.
    if (rc, rr) != (ic, ir):
        raise ValueError(f"Convert the replacement to {rr} Hz / {rc} channel(s) before importing")
    target, source = rms(original), rms(incoming)
    if target < 1e-8 or source < 1e-8:
        raise ValueError("Silent reference or replacement cannot be level-matched")
    peak = max(abs(v) for v in incoming)
    requested_gain = target / source
    gain = min(requested_gain, 10 ** (-1 / 20) / peak)
    rendered = [max(-32768, min(32767, round(v * gain * 32768))) for v in incoming]
    output.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive create protects previously imported files too.
    with output.open("xb") as stream:
        with wave.open(stream, "wb") as dest:
            dest.setnchannels(rc)
            dest.setsampwidth(2)
            dest.setframerate(rr)
            dest.writeframes(struct.pack("<" + "h" * len(rendered), *rendered))
    achieved = rms([v / 32768 for v in rendered])
    return {"method": "RMS dBFS; not LUFS", "reference_rms_dbfs": db(target),
            "replacement_rms_dbfs": db(source), "applied_gain_db": db(gain),
            "output_rms_dbfs": db(achieved), "peak_limited": gain < requested_gain,
            "peak_ceiling_dbfs": -1, "output": str(output),
            "sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
            "playback_connected": False}


def catalog(game, output):
    sounds = Path(game).resolve() / "data" / "sounds"
    if not sounds.is_dir():
        raise ValueError("Game sound directory not found")
    entries = [{"asset": f.name, "display_name": None, "label_status": "unverified"}
               for f in sorted(sounds.iterdir()) if f.is_file() and f.suffix.lower() in (".wav", ".ogg")]
    with Path(output).open("x", encoding="utf-8") as dest:
        json.dump({"schema": 1, "sounds": entries,
                   "note": "No official label mapping inferred from filenames"}, dest, indent=2)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    scan = commands.add_parser("catalog")
    scan.add_argument("game")
    scan.add_argument("output")
    normal = commands.add_parser("prepare")
    normal.add_argument("reference")
    normal.add_argument("replacement")
    normal.add_argument("output")
    args = parser.parse_args()
    try:
        if args.command == "catalog":
            catalog(args.game, args.output)
        else:
            print(json.dumps(prepare(args.reference, args.replacement, args.output), indent=2))
    except (ValueError, OSError, wave.Error) as error:
        parser.exit(1, str(error) + "\n")
