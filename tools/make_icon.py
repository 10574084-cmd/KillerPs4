#!/usr/bin/env python3
import math
import struct
import sys
import zlib

SIZE = 512
pixels = bytearray()

def clamp(x):
    return max(0, min(255, int(x)))

for y in range(SIZE):
    pixels.append(0)
    for x in range(SIZE):
        nx = (x - SIZE / 2) / (SIZE / 2)
        ny = (y - SIZE / 2) / (SIZE / 2)
        t = (y / (SIZE - 1))
        r = 12 + 26 * (1 - t) + 8 * math.sin(nx * 3.2)
        g = 18 + 18 * t + 10 * math.cos(ny * 2.7)
        b = 42 + 64 * (1 - abs(nx)) + 20 * math.sin((nx + ny) * 4)
        d = math.sqrt(nx * nx + ny * ny)
        glow = max(0.0, 1.0 - d) * 22
        r += glow
        g += glow * 0.55
        b += glow * 1.1

        # Central, undistorted 4:3 frame.
        if -0.72 < nx < 0.72 and -0.98 < ny < 0.98:
            edge = min(nx + 0.72, 0.72 - nx, ny + 0.98, 0.98 - ny)
            if edge < 0.025:
                r, g, b = 85, 202, 235
            else:
                stripe = (int((ny + 1.0) * 20) % 2) * 7
                r = 20 + stripe
                g = 28 + stripe
                b = 54 + stripe * 2
        pixels.extend((clamp(r), clamp(g), clamp(b), 255))

def chunk(tag, data):
    return (struct.pack(">I", len(data)) + tag + data +
            struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff))

raw = bytes(pixels)
png = b"\x89PNG\r\n\x1a\n"
png += chunk(b"IHDR", struct.pack(">2I5B", SIZE, SIZE, 8, 6, 0, 0, 0))
png += chunk(b"IDAT", zlib.compress(raw, 9))
png += chunk(b"IEND", b"")
with open(sys.argv[1], "wb") as f:
    f.write(png)
