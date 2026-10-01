"""Continuous receive and actual Hamlib dummy-backend tests. No radio RF."""
import ctypes as C
from pathlib import Path
import sys
import tempfile
import time
import unittest
import wave
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "client"))
import jtty

ROOT = Path(__file__).resolve().parents[1]
lib = C.CDLL(str(ROOT / "build/libjtty_live.dylib"))
lib.jw_feed.argtypes = [C.POINTER(C.c_int16), C.c_int, C.c_float, C.c_float]
lib.jw_encode.argtypes = [C.c_char_p, C.c_int, C.c_float, C.POINTER(C.c_int16), C.c_int, C.c_char_p]
lib.jw_pop.argtypes = [C.POINTER(C.c_int64), C.POINTER(C.c_float), C.POINTER(C.c_double), C.POINTER(C.c_int), C.c_char_p]
lib.jw_radio_open.argtypes = [C.c_int, C.c_char_p, C.c_char_p, C.c_int, C.c_int, C.c_int, C.c_char_p]
lib.jw_radio_open.restype = C.c_void_p
lib.jw_radio_state.argtypes = [C.c_void_p, C.POINTER(C.c_double), C.POINTER(C.c_int), C.c_char_p]
lib.jw_radio_frequency.argtypes = [C.c_void_p, C.c_double]
lib.jw_radio_mode.argtypes = [C.c_void_p, C.c_int]
lib.jw_radio_ptt.argtypes = [C.c_void_p, C.c_int, C.c_double]
lib.jw_radio_close.argtypes = [C.c_void_p]
lib.jw_radio_watchdog.argtypes = [C.c_void_p]


def stream(data):
    updates = []
    for offset in range(0, len(data), 8192):
        block = data[offset:offset + 8192]
        samples = (C.c_int16 * (len(block) // 2)).from_buffer_copy(block)
        assert lib.jw_feed(samples, len(samples), 1500, 100) == 0
        while True:
            identifier, hz, seconds, complete = C.c_int64(), C.c_float(), C.c_double(), C.c_int()
            text = C.create_string_buffer(81)
            if not lib.jw_pop(C.byref(identifier), C.byref(hz), C.byref(seconds), C.byref(complete), text):
                break
            updates.append((identifier.value, text.value.decode().strip(), complete.value, seconds.value))
    return updates


class LiveDecoderTests(unittest.TestCase):
    def setUp(self):
        lib.jw_reset()

    def encoded_wav(self, text):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "test.wav"
            jtty.encode(text, path)
            with wave.open(str(path), "rb") as wav:
                return wav.readframes(wav.getnframes())

    def test_incremental_assembly(self):
        text = "THE QUICK BROWN FOX JUMPED OVER THE LAZY DOG, TWICE."
        updates = stream(self.encoded_wav(text) + bytes(24000 * 3))
        self.assertTrue(any(not update[2] for update in updates))
        self.assertEqual([u[1] for u in updates if u[2]], [text])
        self.assertEqual(len(set(u[0] for u in updates)), 1)

    def test_rollover_preserves_in_progress_message(self):
        text = "THE QUICK BROWN FOX JUMPED OVER THE LAZY DOG, TWICE."
        updates = stream(bytes(177 * 24000) + self.encoded_wav(text) + bytes(3 * 24000))
        final = [u for u in updates if u[2]]
        self.assertEqual([u[1] for u in final], [text])
        self.assertAlmostEqual(final[0][3], 177.5, delta=0.05)

    def test_live_encoding_matches_offline_core(self):
        samples = (C.c_int16 * 400000)()
        text = C.create_string_buffer(81)
        count = lib.jw_encode(b"CQ K1ABC CQ", 0, 1500, samples, len(samples), text)
        self.assertEqual(count, 22656)
        self.assertEqual(text.value.decode().strip(), "CQ K1ABC CQ")
        received = stream(bytes(12000) + bytes(samples)[:count * 2] + bytes(24000 * 3))
        self.assertEqual([u[1] for u in received if u[2]], ["CQ K1ABC CQ"])


class HamlibTests(unittest.TestCase):
    def setUp(self):
        error = C.create_string_buffer(256)
        self.radio = lib.jw_radio_open(1, b"", b"", 38400, 1, 0, error)
        self.assertTrue(self.radio, error.value)
        self.addCleanup(lambda: lib.jw_radio_close(self.radio))

    def state(self):
        hz, ptt, mode = C.c_double(), C.c_int(), C.create_string_buffer(32)
        self.assertEqual(lib.jw_radio_state(self.radio, C.byref(hz), C.byref(ptt), mode), 0)
        return hz.value, ptt.value, mode.value.decode()

    def test_cat_frequency_and_mode(self):
        self.assertEqual(lib.jw_radio_frequency(self.radio, 14090000), 0)
        self.assertEqual(lib.jw_radio_mode(self.radio, 1), 0)
        self.assertEqual(self.state(), (14090000, 0, "PKTUSB"))

    def test_ptt_on_off_and_tuning_guard(self):
        self.assertEqual(lib.jw_radio_ptt(self.radio, 1, 2), 0)
        self.assertEqual(self.state()[1], 1)
        self.assertNotEqual(lib.jw_radio_frequency(self.radio, 14090000), 0)
        self.assertEqual(lib.jw_radio_ptt(self.radio, 0, 0), 0)
        self.assertEqual(self.state()[1], 0)

    def test_watchdog_releases_ptt(self):
        self.assertEqual(lib.jw_radio_ptt(self.radio, 1, 0.2), 0)
        time.sleep(0.45)
        self.assertEqual(self.state()[1], 0)
        self.assertEqual(lib.jw_radio_watchdog(self.radio), 1)

    def test_reject_excessive_transmission_limit(self):
        self.assertNotEqual(lib.jw_radio_ptt(self.radio, 1, 46), 0)
        self.assertEqual(self.state()[1], 0)

    def test_disconnected_control_returns_error(self):
        self.assertNotEqual(lib.jw_radio_ptt(None, 1, 2), 0)


if __name__ == "__main__":
    unittest.main()
