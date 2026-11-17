#!/usr/bin/env python3
# Generate the XLARGE (12x24) and XXLARGE (16x32) native bitmap faces from the
# Terminus font (copyright Dimitar Toshkov Zhekov, SIL Open Font License 1.1;
# see components/esp32_s3_piggyback/TERMINUS-OFL-1.1.txt).
#
# The two source files are the Unicode BDF bitmaps shipped with
# terminus-font 4.49 (XLARGE uses the bold weight so its stems match the
# visual weight of the 9x16 VGA face on the TFT; XXLARGE keeps the regular
# weight, which already draws 2 px stems at this size):
#
#   ter-u24b.bdf  ->  12x24 font bounding box (XLARGE,  bold)
#   ter-u32n.bdf  ->  16x32 font bounding box (XXLARGE, regular)
#
# They are the native 1-bit bitmaps of the font (not scaled outlines), so the
# glyphs are already authored for exactly these pixel grids. The script
# rasterizes printable ASCII 32..126 into the font bounding box and prints
# one C table per face on stdout; the tables are stored in
# components/esp32_s3_piggyback/esp32_s3_piggyback_font.c as static const
# uint16_t[][]. One uint16_t holds one glyph row with the leftmost glyph
# pixel at bit 15 (matching TFT_EC11_FONT_XLARGE/XXLARGE in the renderer).
#
# Regenerate the tables with (paths as arguments, isolated interpreter):
#
#   python3 -I tools/font_gen/generate_fonts.py ter-u24b.bdf ter-u32n.bdf
#
# and paste the printed blocks over the two generated tables in
# esp32_s3_piggyback_font.c (between the BEGIN/END GENERATED markers).
# No other glyphs need a dedicated bitmap: the component folds accented
# Latin-1 letters onto their A-Z base letters and renders every other byte as
# the hollow-box placeholder, so ASCII 32..126 is the complete dedicated
# coverage for every face.

import os
import sys

# (expected width, expected height, C table name)
FACES = (
    (12, 24, "font_xlarge_12x24"),
    (16, 32, "font_xxlarge_16x32"),
)

FIRST = 32
LAST = 126


def parse_bdf(path, want_width, want_height):
    """Return {codepoint: [row_uint16, ...]} for ASCII FIRST..LAST, one
    uint16_t per row of the font bounding box, leftmost pixel at bit 15."""
    font_box = None
    glyphs = {}
    name = None
    encoding = None
    bbx = None
    rows = []
    in_bitmap = False

    with open(path, "r", encoding="latin-1") as handle:
        for line in handle:
            parts = line.split()
            if not parts:
                continue
            key = parts[0]
            if key == "FONTBOUNDINGBOX":
                font_box = (int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4]))
            elif key == "STARTCHAR":
                name = parts[1]
                encoding = None
                bbx = None
                rows = []
                in_bitmap = False
            elif key == "ENCODING":
                encoding = int(parts[1])
            elif key == "BBX":
                bbx = (int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4]))
            elif key == "BITMAP":
                in_bitmap = True
            elif key == "ENDCHAR":
                in_bitmap = False
                if encoding is not None and FIRST <= encoding <= LAST and bbx and font_box:
                    glyphs[encoding] = rasterize(bbx, rows, font_box, want_width)
                    name = None
            elif key == "ENDFONT":
                break
            elif in_bitmap:
                rows.append(int(parts[0], 16))

    if font_box is None:
        raise SystemExit("%s: no FONTBOUNDINGBOX found" % path)
    if font_box[0] != want_width or font_box[1] != want_height:
        raise SystemExit("%s: FONTBOUNDINGBOX %dx%d does not match expected %dx%d"
                         % (path, font_box[0], font_box[1], want_width, want_height))
    missing = [c for c in range(FIRST, LAST + 1) if c not in glyphs]
    if missing:
        raise SystemExit("%s: missing encodings %r" % (path, missing))
    return glyphs


def rasterize(bbx, hex_rows, font_box, want_width):
    """Place one glyph's BBX rows into the font bounding box canvas and
    return one uint16_t per canvas row (leftmost canvas pixel at bit 15)."""
    fw, fh, fx, fy = font_box
    bw, bh, bx, by = bbx
    canvas = [0] * fh

    # BDF packs each raster line into whole bytes with the glyph data
    # left-aligned (most significant bit first), so pixel col c of a row is
    # bit (bytes * 8 - 1 - c) of the row value.
    row_bits = ((bw + 7) // 8) * 8
    for row_index, value in enumerate(hex_rows):
        if row_index >= bh:
            break
        # BDF y grows upward; row 0 of the glyph is the top of its BBX.
        y = by + bh - 1 - row_index
        canvas_row = (fy + fh - 1) - y
        if canvas_row < 0 or canvas_row >= fh:
            continue
        for col in range(bw):
            if not (value >> (row_bits - 1 - col)) & 1:
                continue
            canvas_col = bx + col
            if canvas_col < 0 or canvas_col >= fw:
                continue
            canvas[canvas_row] |= 1 << (15 - canvas_col)
    return canvas


def emit_table(glyphs, table_name, width, height):
    lines = ["static const uint16_t %s[%d][%d] = {" % (table_name, LAST - FIRST + 1, height)]
    for code in range(FIRST, LAST + 1):
        rows = glyphs[code]
        if code == FIRST:
            label = " "
        else:
            label = chr(code)
        lines.append("    { " + ",".join("0x%04X" % v for v in rows) + " }, /* 0x%02X %s */"
                     % (code, label))
    lines.append("};")
    return "\n".join(lines)


def main(argv):
    if len(argv) != 3:
        raise SystemExit("usage: generate_fonts.py <ter-u24n.bdf> <ter-u32n.bdf>")
    for path, (width, height, table_name) in zip(argv[1:], FACES):
        glyphs = parse_bdf(path, width, height)
        print("/* ---- generated from %s (%dx%d, %s) ---- */"
              % (os.path.basename(path), width, height, "Terminus, SIL OFL 1.1"))
        print(emit_table(glyphs, table_name, width, height))
        print()
        sys.stderr.write("%s: %d glyphs, %d rows, %d bytes of table data\n"
                         % (table_name, LAST - FIRST + 1, height,
                            (LAST - FIRST + 1) * height * 2))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
