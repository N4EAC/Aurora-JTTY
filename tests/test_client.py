"""Audio interoperability and input-validation tests for the prototype."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("jtty", ROOT / "client/jtty.py")
jtty = importlib.util.module_from_spec(spec)
spec.loader.exec_module(jtty)


class AudioTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory(prefix="jtty-test-")
        self.addCleanup(self.folder.cleanup)
        self.path = Path(self.folder.name) / "message.wav"

    def test_cq_roundtrip(self):
        tx = jtty.encode("cq k1abc cq", self.path)
        self.assertEqual(tx["frames"], 1)
        messages = jtty.decode(self.path)["messages"]
        self.assertEqual([m["text"] for m in messages], ["CQ K1ABC CQ"])
        self.assertTrue(messages[0]["complete"])

    def test_long_text_roundtrip(self):
        message = "THE QUICK BROWN FOX JUMPED OVER THE LAZY DOG, TWICE."
        tx = jtty.encode(message, self.path, frequency=1800)
        self.assertGreater(tx["frames"], 1)
        messages = jtty.decode(self.path, frequency=1800)["messages"]
        self.assertEqual([m["text"] for m in messages], [message])
        self.assertTrue(messages[0]["complete"])

    def test_exchange_normalization(self):
        tx = jtty.encode("WB9XYZ 599 05", self.path, profile="rtty-roundup")
        self.assertEqual(tx["message"], "WB9XYZ 599 005")
        self.assertEqual(jtty.decode(self.path)["messages"][0]["text"], tx["message"])

    def test_upstream_generated_audio(self):
        subprocess.run([str(ROOT / "build/upstream-sjtty"), "CQ TEST DE KA1ABC", "1500", "0.5", "0", "0", "384", "1", "99"],
                       cwd=self.folder.name, check=True, capture_output=True)
        messages = jtty.decode(Path(self.folder.name) / "000000_000001.wav")["messages"]
        self.assertEqual([m["text"] for m in messages], ["CQ TEST DE KA1ABC"])
        self.assertTrue(messages[0]["complete"])

    def test_recorded_upstream_sample(self):
        sample = ROOT / "upstream/wsjtx/samples/JTTY/260807_134110.wav"
        messages = jtty.decode(sample)["messages"]
        self.assertEqual([m["text"] for m in messages], ["RAN ALL NIGHT ON BAND NOISE - NO FALSE DECODES!"])
        self.assertTrue(messages[0]["complete"])

    def test_silence(self):
        with wave.open(str(self.path), "wb") as wav:
            wav.setparams((1, 2, 12000, 0, "NONE", "not compressed"))
            wav.writeframes(bytes(24000 * 3))
        self.assertEqual(jtty.decode(self.path)["messages"], [])

    def test_reject_bad_messages_without_overwriting(self):
        self.path.write_bytes(b"preserve me")
        for message in ["", "A" * 81, "HELLO ☃"]:
            with self.subTest(message=message), self.assertRaises(ValueError):
                jtty.encode(message, self.path)
        self.assertEqual(self.path.read_bytes(), b"preserve me")

    def test_reject_wrong_audio_format(self):
        with wave.open(str(self.path), "wb") as wav:
            wav.setparams((2, 2, 48000, 0, "NONE", "not compressed"))
            wav.writeframes(bytes(48000 * 4))
        with self.assertRaisesRegex(ValueError, "mono"):
            jtty.decode(self.path)

    def test_frequency_limits(self):
        for frequency in [0, 3000, float("nan")]:
            with self.subTest(frequency=frequency), self.assertRaises(ValueError):
                jtty.encode("CQ K1ABC CQ", self.path, frequency=frequency)


if __name__ == "__main__":
    unittest.main()
