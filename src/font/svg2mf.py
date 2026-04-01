import re
import struct
import sys
import xml.etree.ElementTree as ET

# the no-no zone, we can't manage those in the MF format.
CURVE_COMMANDS = set("cCsSqQtTaA")


def tokenize_path(d):
    # don't ask me where i found this, i will shiver and cry
    tokens = re.findall(
        r"[MmLlHhVvZzCcSsQqTtAa]|[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?", d
    )
    cmd = None
    args = []
    for t in tokens:
        if t.isalpha():
            if cmd:
                yield cmd, args
            cmd, args = t, []
        else:
            args.append(float(t))
    if cmd:
        yield cmd, args


def parse_path(d, glyph_id):
    pts = []
    cx, cy = 0.0, 0.0
    start_x, start_y = 0.0, 0.0

    for cmd, args in tokenize_path(d):
        if cmd in CURVE_COMMANDS:
            print(f"WARN: glyph '{glyph_id}' contains curves ('{cmd}')!!! ignored.")
            if len(args) >= 2:
                if cmd.islower():
                    cx += args[-2]
                    cy += args[-1]
                else:
                    cx, cy = args[-2], args[-1]
            continue

        if cmd == "M":
            cx, cy = args[0], args[1]
            start_x, start_y = cx, cy
            pts.append((cx, cy))
            for i in range(2, len(args) - 1, 2):
                cx, cy = args[i], args[i + 1]
                pts.append((cx, cy))

        elif cmd == "m":
            cx += args[0]
            cy += args[1]
            start_x, start_y = cx, cy
            pts.append((cx, cy))
            for i in range(2, len(args) - 1, 2):
                cx += args[i]
                cy += args[i + 1]
                pts.append((cx, cy))

        elif cmd == "L":
            for i in range(0, len(args) - 1, 2):
                cx, cy = args[i], args[i + 1]
                pts.append((cx, cy))

        elif cmd == "l":
            for i in range(0, len(args) - 1, 2):
                cx += args[i]
                cy += args[i + 1]
                pts.append((cx, cy))

        elif cmd == "H":
            for x in args:
                cx = x
                pts.append((cx, cy))

        elif cmd == "h":
            for dx in args:
                cx += dx
                pts.append((cx, cy))

        elif cmd == "V":
            for y in args:
                cy = y
                pts.append((cx, cy))

        elif cmd == "v":
            for dy in args:
                cy += dy
                pts.append((cx, cy))

        elif cmd in ("Z", "z"):
            cx, cy = start_x, start_y
            pts.append((cx, cy))

    return pts


def encode_vertices(pts, glyph_id):
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    mn_x, mx_x = min(xs), max(xs)
    mn_y, mx_y = min(ys), max(ys)

    def q(val, mn, mx):
        if mx == mn:
            return 0
        return round((val - mn) / (mx - mn) * 15)

    return [(q(x, mn_x, mx_x) << 4) | q(y, mn_y, mx_y) for x, y in pts]


def main(svg_path, out_path):
    tree = ET.parse(svg_path)
    root = tree.getroot()

    glyphs = {}
    for el in root.iter("{http://www.w3.org/2000/svg}path"):
        glyph_id = el.get("id", "")

        if len(glyph_id) != 1 or not glyph_id.isascii() or ord(glyph_id) == 0:
            if glyph_id:
                print(f"warning: skipping path with invalid glyph id '{glyph_id}'")
            continue

        verts = encode_vertices(parse_path(el.get("d", ""), glyph_id), glyph_id)

        if len(verts) > 255:
            print(
                f"warning: glyph '{glyph_id}' has {len(verts)} vertices, truncating to 255"
            )
            verts = verts[:255]

        glyphs[glyph_id] = verts

    glyphs = dict(sorted(glyphs.items(), key=lambda kv: ord(kv[0])))

    with open(out_path, "wb") as f:
        f.write(b"MSv01\x00")
        f.write(struct.pack("<H", len(glyphs)))

        geometry_start = 8 + len(glyphs) * 4
        offsets = {}
        pos = geometry_start
        for cp, verts in glyphs.items():
            offsets[cp] = pos
            pos += len(verts)

        for cp, verts in glyphs.items():
            f.write(struct.pack("<BHB", ord(cp), offsets[cp], len(verts)))

        for verts in glyphs.values():
            f.write(bytes(verts))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} input.svg output.mf")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
