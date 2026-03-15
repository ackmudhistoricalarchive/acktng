#!/usr/bin/env python3
"""Generate PNG map images from docs/world_map.md.

Reads the fenced code blocks (``` ... ```) from world_map.md and renders each
one as a PNG with DejaVuSansMono, using the site's dark colour palette.

Run from anywhere in the repository:
    python3 web/generate_map_images.py

Output files land in web/img/:
    world_map_overview.png      – full geographic overview
    world_map_five_cities.png   – five-city network diagram
    world_map_corridor.png      – oasis-pyramid corridor detail
"""

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT      = Path(__file__).resolve().parent.parent
DOCS_FILE = ROOT / "docs" / "world_map.md"
OUT_DIR   = Path(__file__).resolve().parent / "img"

FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
FONT_SIZE = 13
PAD       = 18   # px padding on every edge

# Site theme colours (matching base.html :root vars)
BG     = (5,   8,  15)   # --bg       #05080f
TEXT   = (229, 236, 255) # --text     #e5ecff
ACCENT = (121, 183, 255) # --accent   #79b7ff  (box drawing, arrows)
MUTED  = (158, 176, 223) # --muted    #9eb0df  (vnum brackets)

BOX_CHARS = frozenset("─│┌┐└┘├┤┬┴┼╔╗╚╝║═◄►↑↓←→↔")

# Map each code block (in order of appearance) to its output filename and title
BLOCK_META = [
    ("world_map_overview",     "World Overview"),
    ("world_map_five_cities",  "Five-City Network"),
    ("world_map_desert_route", "Desert to Sea Route"),
    ("world_map_corridor",     "Oasis-Pyramid Corridor"),
]


def extract_code_blocks(text: str) -> list[str]:
    """Return the content of every fenced ``` block in *text*, in order."""
    blocks: list[str] = []
    current: list[str] = []
    in_block = False
    for line in text.splitlines():
        stripped = line.rstrip()
        if stripped == "```":
            if in_block:
                blocks.append("\n".join(current))
                current = []
                in_block = False
            else:
                in_block = True
        elif in_block:
            current.append(stripped)
    return blocks


def char_color(ch: str, in_bracket: bool) -> tuple[int, int, int]:
    if in_bracket:
        return MUTED
    if ch in BOX_CHARS:
        return ACCENT
    return TEXT


def render_block(text: str, font: ImageFont.FreeTypeFont) -> Image.Image:
    """Render *text* as a PNG image with per-character colouring."""
    lines = text.split("\n")

    # Measure a fixed-width reference cell
    bbox   = font.getbbox("M")
    char_w = bbox[2] - bbox[0]
    char_h = bbox[3] - bbox[1] + 3   # +3 px extra leading

    max_cols = max((len(l) for l in lines), default=1)
    img_w = max_cols * char_w + PAD * 2
    img_h = len(lines) * char_h  + PAD * 2

    img  = Image.new("RGB", (img_w, img_h), BG)
    draw = ImageDraw.Draw(img)

    for row, line in enumerate(lines):
        y = PAD + row * char_h
        in_bracket = False
        for col, ch in enumerate(line):
            x = PAD + col * char_w
            if ch == "[":
                in_bracket = True
            color = char_color(ch, in_bracket)
            draw.text((x, y), ch, font=font, fill=color)
            if ch == "]":
                in_bracket = False

    return img


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    md_text = DOCS_FILE.read_text(encoding="utf-8")
    blocks  = extract_code_blocks(md_text)
    font    = ImageFont.truetype(FONT_PATH, FONT_SIZE)

    if len(blocks) < len(BLOCK_META):
        raise RuntimeError(
            f"Expected at least {len(BLOCK_META)} code blocks in world_map.md, "
            f"found {len(blocks)}."
        )

    for (name, title), block in zip(BLOCK_META, blocks):
        img      = render_block(block, font)
        out_path = OUT_DIR / f"{name}.png"
        img.save(str(out_path), "PNG")
        print(f"  {out_path.name:40s}  {img.size[0]}×{img.size[1]} px")

    print("Done.")


if __name__ == "__main__":
    main()
