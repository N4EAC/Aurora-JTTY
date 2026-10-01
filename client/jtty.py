#!/usr/bin/env python3
"""JTTY Workbench recorded-audio client. GPL-3.0-or-later, 2026-10-01."""
import argparse
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import wave

ROOT = Path(__file__).resolve().parent.parent
PROFILES = {"unknown": 0, "field-day": 1, "rtty-roundup": 2}
ALPHABET = set('0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ +-./?!"#$%,&*()_\'=[]{}<>|:;')


def run_engine(*args):
    engine = Path(os.environ.get("JTTY_ENGINE", ROOT / "build/jtty-engine"))
    if not engine.is_file():
        raise ValueError("Engine missing. Run scripts/build.sh first.")
    result = subprocess.run([str(engine), *map(str, args)], text=True,
                            capture_output=True, timeout=180)
    if result.returncode:
        raise ValueError(result.stderr.strip() or result.stdout.strip() or "Engine failed")
    return result.stdout.strip()


def encode(message, output, frequency=1500, profile="unknown"):
    if not math.isfinite(frequency) or not 200 <= frequency <= 2600:
        raise ValueError("Frequency must be between 200 and 2600 Hz.")
    normalized_input = " ".join(message.upper().split())
    if not normalized_input or len(normalized_input) > 80:
        raise ValueError("Enter 1–80 characters after whitespace normalization.")
    unsupported = sorted(set(normalized_input) - ALPHABET)
    if unsupported:
        raise ValueError("Unsupported characters: " + " ".join(unsupported))
    destination = Path(output).expanduser().resolve()
    if not destination.parent.is_dir():
        raise ValueError("Output folder does not exist.")
    with tempfile.TemporaryDirectory(prefix="jtty-") as temporary:
        pcm = Path(temporary) / "audio.pcm"
        frames, text = run_engine("encode", pcm, frequency, PROFILES[profile], normalized_input).split("\t", 1)
        data = pcm.read_bytes()
        # Create in the destination folder, then replace atomically on success.
        fd, temporary_output = tempfile.mkstemp(prefix=".jtty-", suffix=".wav", dir=destination.parent)
        os.close(fd)
        try:
            with wave.open(temporary_output, "wb") as wav:
                wav.setparams((1, 2, 12000, 0, "NONE", "not compressed"))
                wav.writeframes(data)
            os.replace(temporary_output, destination)
        finally:
            if os.path.exists(temporary_output):
                os.unlink(temporary_output)
    return {"output": str(destination), "message": text, "frames": int(frames),
            "signal_seconds": int(frames) * 1.888, "wav_seconds": len(data) / 24000,
            "sample_rate": 12000, "frequency_hz": frequency, "profile": profile}


def decode(recording, frequency=1500, tolerance=50):
    if not math.isfinite(frequency) or not 200 <= frequency <= 2600:
        raise ValueError("Frequency must be between 200 and 2600 Hz.")
    if not math.isfinite(tolerance) or not 1 <= tolerance <= 1000:
        raise ValueError("Tolerance must be between 1 and 1000 Hz.")
    source = Path(recording).expanduser().resolve()
    with wave.open(str(source), "rb") as wav:
        if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getcomptype()) != (1, 2, 12000, "NONE"):
            raise ValueError("Use uncompressed mono, 16-bit PCM WAV at 12000 Hz.")
        if not 0 < wav.getnframes() <= 180 * 12000:
            raise ValueError("Recording must contain 0–180 seconds of audio.")
        data = wav.readframes(wav.getnframes())
        if len(data) != wav.getnframes() * 2:
            raise ValueError("WAV data is truncated.")
    with tempfile.TemporaryDirectory(prefix="jtty-") as temporary:
        pcm = Path(temporary) / "recording.pcm"
        pcm.write_bytes(data)
        output = run_engine("decode", pcm, frequency, tolerance)
    messages = {}
    for line in output.splitlines():
        identifier, hz, seconds, complete, text = line.split("\t", 4)
        messages[int(identifier)] = {"id": int(identifier), "frequency_hz": float(hz),
            "start_seconds": float(seconds), "complete": complete == "1", "text": text}
    return {"input": str(source), "messages": list(messages.values()), "duration_seconds": len(data) / 24000}


def main():
    parser = argparse.ArgumentParser(description="Encode and decode JTTY recorded audio")
    commands = parser.add_subparsers(dest="command", required=True)
    tx = commands.add_parser("encode")
    tx.add_argument("message")
    tx.add_argument("output")
    tx.add_argument("--profile", choices=PROFILES, default="unknown")
    tx.add_argument("--frequency", type=float, default=1500)
    rx = commands.add_parser("decode")
    rx.add_argument("recording")
    rx.add_argument("--frequency", type=float, default=1500)
    rx.add_argument("--tolerance", type=float, default=50)
    options = parser.parse_args()
    try:
        if options.command == "encode":
            result = encode(options.message, options.output, options.frequency, options.profile)
        else:
            result = decode(options.recording, options.frequency, options.tolerance)
        print(json.dumps(result, indent=2))
    except (ValueError, OSError, wave.Error, EOFError, subprocess.TimeoutExpired) as error:
        print(str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
