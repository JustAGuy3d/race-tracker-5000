"""Drives the touch screens over USB for testing (uses the `t<x>,<y>` serial command).

    python tools/tap.py COMx tap 200,146          # one tap
    python tools/tap.py COMx type "abc123"        # type on the on-screen keyboard
    python tools/tap.py COMx key OK|DEL|SHIFT|MODE|CANCEL

Key positions mirror layoutKeys() in src/ui_wifi.cpp.
"""
import sys
import time

import serial

W, KB_Y, ROW_H, GAP, KEY_W = 800, 150, 74, 8, 68
LETTERS = ["qwertyuiop", "asdfghjkl", "zxcvbnm"]
SYM1 = ["1234567890", "-/:;()$&@", ".,?!'\"_"]
SYM2 = ["[]{}#%^*+=", "\\|~<>`", ".,?!'\"_"]


def row_y(r):
    return KB_Y + r * (ROW_H + GAP) + ROW_H // 2


def char_pos(rows, ch):
    for r, chars in enumerate(rows):
        if ch in chars:
            i = chars.index(ch)
            if r < 2:
                x0 = (W - (len(chars) * KEY_W + (len(chars) - 1) * GAP)) // 2
            else:
                wide = (W - 48 - len(chars) * KEY_W - (len(chars) + 1) * GAP) // 2
                x0 = 24 + wide + GAP
            return x0 + i * (KEY_W + GAP) + KEY_W // 2, row_y(r)
    return None


SPECIAL = {  # see layoutKeys() in src/ui_wifi.cpp
    "SHIFT": (60, row_y(2)),
    "DEL": (W - 60, row_y(2)),
    "MODE": (84, row_y(3)),
    "SPACE": (400, row_y(3)),
    "OK": (W - 124, row_y(3)),
    "CANCEL": (710, 27),
}


class Board:
    def __init__(self, port):
        self.s = serial.Serial(port, 115200, timeout=0.2)
        time.sleep(0.3)

    def tap(self, x, y, pause=0.35):
        self.s.write(f"t{x},{y}\n".encode())
        time.sleep(pause)

    def key(self, name):
        self.tap(*SPECIAL[name])

    def type(self, text):
        mode = "letters"  # the keyboard opens in lower-case letters
        for ch in text:
            if ch == " ":
                self.key("SPACE")
                continue
            if ch.isalpha():
                if mode != "letters":
                    self.key("MODE")
                    mode = "letters"
                if ch.isupper():
                    self.key("SHIFT")
                self.tap(*char_pos(LETTERS, ch.lower()))
                continue
            for name, rows in (("sym1", SYM1), ("sym2", SYM2)):
                if char_pos(rows, ch):
                    if mode == "letters":
                        self.key("MODE")
                        mode = "sym1"
                    if mode != name:
                        self.key("SHIFT")  # the #+= / 123 key flips symbol pages
                        mode = name
                    self.tap(*char_pos(rows, ch))
                    break


if __name__ == "__main__":
    b = Board(sys.argv[1])
    cmd = sys.argv[2]
    if cmd == "tap":
        x, y = map(int, sys.argv[3].split(","))
        b.tap(x, y)
    elif cmd == "type":
        b.type(sys.argv[3])
    elif cmd == "key":
        b.key(sys.argv[3])
