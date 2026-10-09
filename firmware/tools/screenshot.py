"""Grabs a screenshot from the board over USB serial.

    pip install pyserial pillow
    python tools/screenshot.py COMx out.png [keys-to-send-first]

The optional third argument is sent before the screenshot, e.g. "s" to open the
schedule page first (see the serial commands in README.md).
"""
import sys
import time

import serial
from PIL import Image


def main():
    port, out = sys.argv[1], sys.argv[2]
    keys = sys.argv[3] if len(sys.argv) > 3 else ""
    s = serial.Serial(port, 115200, timeout=30)
    time.sleep(0.3)
    s.reset_input_buffer()
    if keys:
        s.write(keys.encode())
        time.sleep(1.5)
        s.reset_input_buffer()
    s.write(b"p")
    while True:
        line = s.readline().decode(errors="replace").strip()
        if line.startswith("SCREENSHOT "):
            _, w, h = line.split()
            w, h = int(w), int(h)
            break
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        x = 0
        while x < w:
            run, lo, hi = s.read(3)
            v = lo | (hi << 8)
            rgb = ((v >> 8) & 0xF8, (v >> 3) & 0xFC, (v << 3) & 0xF8)
            for i in range(run):
                px[x + i, y] = rgb
            x += run
    img.save(out)
    print("saved", out)


if __name__ == "__main__":
    main()
