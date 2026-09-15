import importlib.util
from pathlib import Path
import math
import struct
import tempfile
import wave

spec = importlib.util.spec_from_file_location("sound_workshop", Path(__file__).parents[1] / "scripts/sound_workshop.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

def write(path, values):
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1); out.setsampwidth(2); out.setframerate(48000)
        out.writeframes(struct.pack("<" + "h" * len(values), *[round(v * 32767) for v in values]))

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory); (root / "originals").mkdir(); (root / "imports").mkdir()
    reference = root / "originals/reference.wav"; incoming = root / "input.wav"
    write(reference, [math.sin(i * .1) * .3 for i in range(4800)])
    original_bytes = reference.read_bytes()
    write(incoming, [math.sin(i * .1) * .05 for i in range(4800)])
    result = module.prepare(reference, incoming, root / "imports/matched.wav")
    assert abs(result["output_rms_dbfs"] - result["reference_rms_dbfs"]) < .01
    assert not result["peak_limited"] and reference.read_bytes() == original_bytes
    write(incoming, [.9] + [.001] * 4799)
    result = module.prepare(reference, incoming, root / "imports/limited.wav")
    assert result["peak_limited"]
    _, _, values = module.read_pcm(root / "imports/limited.wav")
    assert max(abs(v) for v in values) <= 10 ** (-1 / 20) + 1 / 32768
    for destination in (reference, incoming, root / "originals/new.wav", root / "imports/matched.wav"):
        try: module.prepare(reference, incoming, destination)
        except (ValueError, FileExistsError): pass
        else: raise AssertionError("Overwrite/original-directory write was accepted")
    write(incoming, [0] * 100)
    try: module.prepare(reference, incoming, root / "imports/silent.wav")
    except ValueError: pass
    else: raise AssertionError("Silent source accepted")
print("PASS: RMS matching, peak limit, silent-source rejection, original/duplicate output protection")
