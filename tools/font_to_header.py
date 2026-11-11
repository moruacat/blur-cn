#!/usr/bin/env python3
"""Convert a TTF into an ImGui `AddFontFromMemoryCompressedTTF` compatible header.

ImGui stores embedded fonts using the old `stb_compress` stream format (the same
one produced by misc/fonts/binary_to_compressed_c.cpp).  zlib/gzip are *not*
compatible, so this file contains a small Python port of the compressor and the
matching decompressor (used to verify the round trip).

Usage:
    python tools/font_to_header.py <font.ttf> <SYMBOL_NAME> <output.h>
"""

from __future__ import annotations

import sys
from pathlib import Path

MASK32 = 0xFFFFFFFF
WINDOW = 0x40000  # 256K
HASH_SIZE = 32768


# ---------------------------------------------------------------------------
# stb_compress
# ---------------------------------------------------------------------------
class _Writer:
    def __init__(self) -> None:
        self.buf = bytearray()

    def out(self, value: int, size: int) -> None:
        for shift in range((size - 1) * 8, -1, -8):
            self.buf.append((value >> shift) & 0xFF)

    def raw(self, data: bytes) -> None:
        self.buf.extend(data)


def _adler32(adler: int, data: bytes) -> int:
    mod = 65521
    s1 = adler & 0xFFFF
    s2 = adler >> 16
    pos = 0
    remaining = len(data)
    blocklen = remaining % 5552
    if remaining and blocklen == 0:
        blocklen = 5552
    while remaining:
        end = pos + blocklen
        while pos < end:
            s1 += data[pos]
            s2 += s1
            pos += 1
        s1 %= mod
        s2 %= mod
        remaining -= blocklen
        blocklen = 5552
    return ((s2 << 16) + s1) & MASK32


def _match_len(data: bytes, a: int, b: int, maxlen: int) -> int:
    i = 0
    while i < maxlen and data[a + i] == data[b + i]:
        i += 1
    return i


def _not_crap(best: int, dist: int) -> bool:
    return (
        (best > 2 and dist <= 0x00100)
        or (best > 5 and dist <= 0x04000)
        or (best > 7 and dist <= 0x80000)
    )


def _out_literals(w: _Writer, data: bytes, pos: int, numlit: int) -> None:
    while numlit > 65536:
        _out_literals(w, data, pos, 65536)
        pos += 65536
        numlit -= 65536

    if numlit == 0:
        pass
    elif numlit <= 32:
        w.out(0x000020 + numlit - 1, 1)
    elif numlit <= 2048:
        w.out(0x000800 + numlit - 1, 2)
    else:
        w.out(0x070000 + numlit - 1, 3)

    w.raw(data[pos : pos + numlit])


def stb_compress(data: bytes) -> bytes:
    w = _Writer()
    length = len(data)
    chash: dict[int, int] = {}
    mask = HASH_SIZE - 1

    # stream signature
    w.out(0x57, 1)
    w.out(0xBC, 1)
    w.out(0, 2)
    w.out(0, 4)  # 64-bit length, leading 32 bits
    w.out(length, 4)
    w.out(WINDOW, 4)

    running_adler = 1
    literals = 0
    lit_start = 0
    q = 0

    while q < length and q + 12 < length:
        best = 2
        dist = 0

        match_max = min(65536, length - q)

        h = ((data[q] << 14) + (data[q + 1] << 7) + data[q + 2]) & MASK32
        h1 = ((h + (h >> 16)) & mask)
        t = chash.get(h1)
        if t is not None:
            m = _match_len(data, t, q, match_max)
            if m > best and q - t <= WINDOW and (m > 9 or _not_crap(m, q - t)):
                best, dist = m, q - t

        h = (((h << 14) + (h >> 18) + (data[q + 3] << 7) + data[q + 4])) & MASK32
        h2 = (h + (h >> 16)) & mask
        h = (((h << 14) + (h >> 18) + (data[q + 5] << 7) + data[q + 6])) & MASK32
        t = chash.get(h2)
        if t is not None and dist != q - t:
            m = _match_len(data, t, q, match_max)
            if m > best and q - t <= WINDOW and (m > 9 or _not_crap(m, q - t)):
                best, dist = m, q - t

        h = (((h << 14) + (h >> 18) + (data[q + 7] << 7) + data[q + 8])) & MASK32
        h3 = (h + (h >> 16)) & mask
        h = (((h << 14) + (h >> 18) + (data[q + 9] << 7) + data[q + 10])) & MASK32
        t = chash.get(h3)
        if t is not None and dist != q - t:
            m = _match_len(data, t, q, match_max)
            if m > best and q - t <= WINDOW and (m > 9 or _not_crap(m, q - t)):
                best, dist = m, q - t

        h = (((h << 14) + (h >> 18) + (data[q + 11] << 7) + data[q + 12])) & MASK32
        h4 = (h + (h >> 16)) & mask
        t = chash.get(h4)
        if t is not None and dist != q - t:
            m = _match_len(data, t, q, match_max)
            if m > best and q - t <= WINDOW and (m > 9 or _not_crap(m, q - t)):
                best, dist = m, q - t

        chash[h1] = chash[h2] = chash[h3] = chash[h4] = q

        if best < 3:
            q += 1
        elif best <= 0x80 and dist <= 0x100:
            _out_literals(w, data, lit_start, q - lit_start)
            q += best
            lit_start = q
            w.out(0x80 + best - 1, 1)
            w.out(dist - 1, 1)
        elif best > 5 and best <= 0x100 and dist <= 0x4000:
            _out_literals(w, data, lit_start, q - lit_start)
            q += best
            lit_start = q
            w.out(0x4000 + dist - 1, 2)
            w.out(best - 1, 1)
        elif best > 7 and best <= 0x100 and dist <= 0x80000:
            _out_literals(w, data, lit_start, q - lit_start)
            q += best
            lit_start = q
            w.out(0x180000 + dist - 1, 3)
            w.out(best - 1, 1)
        elif best > 8 and best <= 0x10000 and dist <= 0x80000:
            _out_literals(w, data, lit_start, q - lit_start)
            q += best
            lit_start = q
            w.out(0x100000 + dist - 1, 3)
            w.out(best - 1, 2)
        elif best > 9 and dist <= 0x1000000:
            best = min(best, 65536)
            _out_literals(w, data, lit_start, q - lit_start)
            q += best
            lit_start = q
            if best <= 0x100:
                w.out(0x06, 1)
                w.out(dist - 1, 3)
                w.out(best - 1, 1)
            else:
                w.out(0x04, 1)
                w.out(dist - 1, 3)
                w.out(best - 1, 2)
        else:
            q += 1

    if q < length:
        q = length

    literals = q - lit_start
    running_adler = _adler32(running_adler, data[:q])

    _out_literals(w, data, length - literals, literals)

    w.out(0x05FA, 2)
    w.out(running_adler, 4)
    return bytes(w.buf)


# ---------------------------------------------------------------------------
# stb_decompress (verification only)
# ---------------------------------------------------------------------------
def _in4(data: bytes, p: int) -> int:
    return (data[p] << 24) | (data[p + 1] << 16) | (data[p + 2] << 8) | data[p + 3]


def _in3(data: bytes, p: int) -> int:
    return (data[p] << 16) | (data[p + 1] << 8) | data[p + 2]


def _in2(data: bytes, p: int) -> int:
    return (data[p] << 8) | data[p + 1]


def stb_decompress(data: bytes) -> bytes:
    if _in4(data, 0) != 0x57BC0000 or _in4(data, 4) != 0:
        raise ValueError("not an stb_compress stream")

    olen = _in4(data, 8)
    out = bytearray()
    i = 16

    while True:
        b = data[i]
        old_i = i

        if b >= 0x20:
            if b >= 0x80:
                length = b - 0x80 + 1
                dist = data[i + 1] + 1
                i += 2
                src = len(out) - dist
                for k in range(length):
                    out.append(out[src + k])
            elif b >= 0x40:
                dist = _in2(data, i) - 0x4000 + 1
                length = data[i + 2] + 1
                i += 3
                src = len(out) - dist
                for k in range(length):
                    out.append(out[src + k])
            else:
                length = b - 0x20 + 1
                out.extend(data[i + 1 : i + 1 + length])
                i += 1 + length
        else:
            if b >= 0x18:
                dist = _in3(data, i) - 0x180000 + 1
                length = data[i + 3] + 1
                i += 4
                src = len(out) - dist
                for k in range(length):
                    out.append(out[src + k])
            elif b >= 0x10:
                dist = _in3(data, i) - 0x100000 + 1
                length = _in2(data, i + 3) + 1
                i += 5
                src = len(out) - dist
                for k in range(length):
                    out.append(out[src + k])
            elif b >= 0x08:
                length = _in2(data, i) - 0x0800 + 1
                out.extend(data[i + 2 : i + 2 + length])
                i += 2 + length
            elif b == 0x07:
                length = _in2(data, i + 1) + 1
                out.extend(data[i + 3 : i + 3 + length])
                i += 3 + length
            elif b == 0x06:
                dist = _in3(data, i + 1) + 1
                length = data[i + 4] + 1
                i += 5
                src = len(out) - dist
                for k in range(length):
                    out.append(out[src + k])
            elif b == 0x04:
                dist = _in3(data, i + 1) + 1
                length = _in2(data, i + 4) + 1
                i += 6
                src = len(out) - dist
                for k in range(length):
                    out.append(out[src + k])

        if i == old_i:
            if data[i] == 0x05 and data[i + 1] == 0xFA:
                if len(out) != olen:
                    raise ValueError(f"size mismatch: got {len(out)}, expected {olen}")
                if _adler32(1, bytes(out)) != _in4(data, i + 2):
                    raise ValueError("adler32 mismatch")
                return bytes(out)
            raise ValueError(f"invalid token at {i}")


# ---------------------------------------------------------------------------
# header generation
# ---------------------------------------------------------------------------
def write_header(symbol: str, compressed: bytes, out_path: Path) -> None:
    lines = [
        "#pragma once",
        "",
        f"inline constexpr std::array<unsigned char, {len(compressed)}> {symbol} = {{",
        "\t// clang-format off",
    ]

    row: list[str] = []
    for value in compressed:
        row.append(str(value))
        if len(row) == 32:
            lines.append("    " + ",".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ",".join(row) + ",")

    lines.append("\t// clang-format on")
    lines.append("};")
    lines.append("")

    out_path.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main() -> int:
    if len(sys.argv) != 4:
        print(__doc__)
        return 1

    ttf_path = Path(sys.argv[1])
    symbol = sys.argv[2]
    out_path = Path(sys.argv[3])

    raw = ttf_path.read_bytes()
    print(f"input: {ttf_path} ({len(raw)} bytes)")

    compressed = stb_compress(raw)
    print(f"compressed: {len(compressed)} bytes ({len(compressed) / len(raw) * 100:.1f}%)")

    restored = stb_decompress(compressed)
    if restored != raw:
        raise SystemExit("round trip failed: decompressed data differs from input")

    print("round trip: OK")
    write_header(symbol, compressed, out_path)
    print(f"wrote: {out_path} ({out_path.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
