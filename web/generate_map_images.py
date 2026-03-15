#!/usr/bin/env python3
"""Generate fantasy-style cartographic PNG map images for the ACK!TNG world.

Run from anywhere in the repository:
    python3 web/generate_map_images.py

Output files land in web/img/:
    world_map_overview.png      – full world, illustrated cartographic style
    world_map_corridor.png      – oasis-pyramid corridor, zoomed detail
    world_map_five_cities.png   – five-city network diagram
    world_map_desert_route.png  – desert-to-sea route diagram
"""

import math
import random
from pathlib import Path
from PIL import (
    Image, ImageDraw, ImageFont, ImageFilter, ImageEnhance, ImageChops
)

random.seed(42)   # reproducible parchment texture

ROOT    = Path(__file__).resolve().parent.parent
OUT_DIR = Path(__file__).resolve().parent / "img"

SERIF      = "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf"
SERIF_BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf"
SANS       = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
SANS_BOLD  = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

# ── colour palette ───────────────────────────────────────────────────────────
SEA        = ( 98, 148, 185)
SEA_DEEP   = ( 65, 108, 150)
SEA_LIGHT  = (138, 180, 210)
LAND       = (228, 205, 152)
FOREST     = (115, 152,  88)
FOREST_DK  = ( 78, 112,  60)
FOREST_LT  = (148, 182, 118)
DESERT     = (208, 172,  98)
DESERT_DK  = (182, 142,  68)
SCORCHED   = (162, 112,  55)
MOUNTAIN   = (148, 128, 102)
MTN_SNOW   = (225, 218, 205)
SWAMP      = (102, 128,  88)
RIVER      = ( 85, 148, 192)
ROAD       = (148, 102,  38)
ROAD_LT    = (195, 150,  75)
INK        = ( 30,  16,   4)
INK_SOFT   = ( 70,  42,  15)
INK_MED    = ( 55,  32,  10)
GOLD       = (172, 142,  28)
GOLD_LT    = (220, 188,  72)
GOLD_PALE  = (238, 215, 138)
PARCH      = (235, 215, 160)
PARCH_DK   = (202, 178, 112)
PARCH_PALE = (248, 235, 195)
CRIMSON    = (148,  28,  20)
RUIN       = (118,  95,  70)
CITY_CLR   = ( 30,  16,   4)
OASIS_CLR  = ( 62, 155, 118)
OASIS_LT   = (105, 192, 155)
PARCHMENT  = (232, 210, 155)   # used as texture base

# ── utility ──────────────────────────────────────────────────────────────────

def lerp(a, b, t):
    return a + (b - a) * t

def pt_lerp(p0, p1, t):
    return (lerp(p0[0], p1[0], t), lerp(p0[1], p1[1], t))

def dist(a, b):
    return math.hypot(b[0]-a[0], b[1]-a[1])

def angle_pt(cx, cy, angle_deg, r):
    a = math.radians(angle_deg)
    return (cx + r * math.cos(a), cy + r * math.sin(a))

def fuzz(pts, sigma=6):
    """Return a list of points with small Gaussian jitter — for organic edges."""
    return [(x + random.gauss(0, sigma), y + random.gauss(0, sigma))
            for x, y in pts]

def poly_ring(cx, cy, r, n=32, jitter=0):
    """Points of an irregular polygon centred at (cx,cy)."""
    pts = []
    for i in range(n):
        a  = 2 * math.pi * i / n
        rr = r + random.gauss(0, jitter) if jitter else r
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    return pts

# ── texture helpers ──────────────────────────────────────────────────────────

def make_parchment_base(size, seed=42):
    """Create an aged parchment background."""
    rng = random.Random(seed)
    w, h = size

    # 1. Generate coarse smooth noise at 1/8 resolution, then upscale
    sw, sh = max(2, w // 8), max(2, h // 8)
    small  = Image.new('RGB', (sw, sh))
    spix   = small.load()
    for sy in range(sh):
        for sx in range(sw):
            v  = rng.randint(-22, 22)
            vg = rng.randint(-18, 18)
            vb = rng.randint(-12, 8)
            spix[sx, sy] = (
                max(0, min(255, 230 + v)),
                max(0, min(255, 210 + v + vg)),
                max(0, min(255, 158 + v + vg + vb)),
            )
    noise = small.resize(size, Image.BILINEAR)
    noise = noise.filter(ImageFilter.GaussianBlur(4))

    # 2. Solid warm parchment base
    base = Image.new('RGB', size, (232, 210, 155))

    # 3. Blend
    result = Image.blend(base, noise, 0.35)

    # 4. Fine speckle pass for paper grain
    speck = Image.new('RGB', size, (232, 210, 155))
    sd    = ImageDraw.Draw(speck)
    n_spots = w * h // 120
    for _ in range(n_spots):
        x  = rng.randint(0, w - 1)
        y  = rng.randint(0, h - 1)
        dv = rng.randint(-30, 20)
        c  = max(0, min(255, 225 + dv))
        cg = max(0, min(255, 205 + dv))
        cb = max(0, min(255, 148 + dv - 12))
        r  = rng.choice([0, 0, 0, 1, 1])
        if r == 0:
            sd.point((x, y), fill=(c, cg, cb))
        else:
            sd.ellipse([x-r, y-r, x+r, y+r], fill=(c, cg, cb))
    speck  = speck.filter(ImageFilter.GaussianBlur(0.5))
    result = Image.blend(result, speck, 0.25)

    # 5. Edge vignette (darker towards edges for aged look)
    vign = Image.new('RGB', size, (185, 160, 100))
    mask = Image.new('L', size, 0)
    md   = ImageDraw.Draw(mask)
    cx, cy = w // 2, h // 2
    steps = 80
    for i in range(steps):
        t   = i / steps
        rw  = int(cx * (1 - t * 0.75))
        rh  = int(cy * (1 - t * 0.75))
        val = int(255 * (t ** 2.2))
        md.ellipse([cx - rw, cy - rh, cx + rw, cy + rh], fill=val)
    mask = mask.filter(ImageFilter.GaussianBlur(max(w, h) // 12))
    result.paste(vign, mask=mask)

    return result


def soft_region(img, polygon, color, blur=35, alpha=200):
    """Paint a soft-edged region onto img in place."""
    layer = Image.new('RGB', img.size, color)
    mask  = Image.new('L',   img.size, 0)
    ImageDraw.Draw(mask).polygon(polygon, fill=alpha)
    mask  = mask.filter(ImageFilter.GaussianBlur(blur))
    img.paste(layer, mask=mask)


def soft_circle(img, cx, cy, r, color, blur=25, alpha=200):
    """Paint a soft-edged filled circle."""
    layer = Image.new('RGB', img.size, color)
    mask  = Image.new('L',   img.size, 0)
    d     = ImageDraw.Draw(mask)
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=alpha)
    mask  = mask.filter(ImageFilter.GaussianBlur(blur))
    img.paste(layer, mask=mask)


# ── symbol drawing ───────────────────────────────────────────────────────────

def draw_city_marker(draw, cx, cy, r=10, color=INK):
    """Medieval walled-city symbol: filled square with corner towers."""
    hw = int(r * 0.7)
    # Base square
    draw.rectangle([cx-hw, cy-hw, cx+hw, cy+hw], fill=color)
    # Four corner towers
    tr = max(3, int(r * 0.35))
    for dx, dy in [(-hw, -hw), (hw, -hw), (-hw, hw), (hw, hw)]:
        x, y = cx + dx, cy + dy
        draw.ellipse([x-tr, y-tr, x+tr, y+tr], fill=color)
    # White centre dot
    ir = max(2, int(r * 0.3))
    draw.ellipse([cx-ir, cy-ir, cx+ir, cy+ir], fill=PARCH_PALE)


def draw_town_marker(draw, cx, cy, r=7, color=INK):
    """Small town: circle with a cross."""
    draw.ellipse([cx-r, cy-r, cx+r, cy+r], outline=color, width=2)
    draw.ellipse([cx-2, cy-2, cx+2, cy+2], fill=color)


def draw_pyramid_icon(draw, cx, cy, size=18, color=INK):
    """Stylised pyramid: triangle with inner line."""
    h   = int(size * 0.85)
    pts = [(cx, cy - h), (cx - size, cy + h // 2), (cx + size, cy + h // 2)]
    draw.polygon(pts, outline=color, fill=DESERT_DK)
    # Horizon line on the pyramid face
    mid_l = pt_lerp(pts[0], pts[1], 0.45)
    mid_r = pt_lerp(pts[0], pts[2], 0.45)
    draw.line([mid_l, mid_r], fill=color, width=1)


def draw_oasis_icon(draw, cx, cy, r=9, color=OASIS_CLR):
    """Oasis: water pool with palm fronds."""
    # Pool
    draw.ellipse([cx-r, cy-r+3, cx+r, cy+r+3], fill=OASIS_CLR, outline=INK)
    # Three palm fronds (arcs)
    for angle in [-35, 0, 35]:
        ex, ey = angle_pt(cx, cy - 3, angle - 90, r + 7)
        draw.line([(cx, cy - 3), (ex, ey)], fill=FOREST_DK, width=2)
        draw.ellipse([ex-3, ey-3, ex+3, ey+3], fill=FOREST, outline=FOREST_DK)


def draw_ruin_icon(draw, cx, cy, size=8, color=RUIN):
    """Ruined site: broken columns."""
    for dx in [-size//2, size//2]:
        x = cx + dx
        # Column shaft
        draw.rectangle([x-2, cy-size, x+2, cy+size//2], fill=color, outline=INK_SOFT)
        # Broken top (diagonal cut)
        draw.line([(x-3, cy-size+rng_int(0,4)), (x+3, cy-size+rng_int(0,4)+3)],
                  fill=INK_SOFT, width=1)


def draw_dungeon_icon(draw, cx, cy, size=6, color=INK_SOFT):
    """Underground site: down-pointing staircase symbol."""
    # Keyhole shape
    draw.ellipse([cx-size, cy-size, cx+size, cy], fill=color)
    draw.polygon([(cx-size//2, cy), (cx+size//2, cy), (cx, cy+size+2)], fill=color)


def rng_int(a, b):
    return random.randint(a, b)


def draw_trees(draw, cx, cy, n=5, spread=22, color=FOREST_DK):
    """Cluster of pine-tree symbols."""
    rng = random.Random(cx * 1000 + cy)
    for _ in range(n):
        x = cx + rng.randint(-spread, spread)
        y = cy + rng.randint(-spread//2, spread//2)
        s = rng.randint(6, 10)
        # Trunk
        draw.line([(x, y + s), (x, y + s + 4)], fill=INK_SOFT, width=1)
        # Two-tier triangles (pine silhouette)
        draw.polygon([(x, y - s), (x - s, y + s//2), (x + s, y + s//2)],
                     fill=color, outline=FOREST_DK)
        draw.polygon([(x, y - s//2), (x - s+2, y + s), (x + s-2, y + s)],
                     fill=FOREST_LT, outline=FOREST_DK)


def draw_mountains(draw, cx, cy, n=4, spread=30, color=MOUNTAIN):
    """Row of mountain peaks."""
    rng = random.Random(cx * 137 + cy)
    pts = []
    base_y = cy + 18
    for i in range(n):
        x = cx - spread + i * (spread * 2 // (n - 1)) + rng.randint(-6, 6)
        h = rng.randint(20, 35)
        w = rng.randint(18, 28)
        peak = (x, cy - h + rng.randint(-4, 4))
        left = (x - w, base_y)
        right = (x + w, base_y)
        pts.append((left, peak, right))
    # Draw back row slightly lighter
    for left, peak, right in pts[1::2]:
        draw.polygon([left, peak, right], fill=MOUNTAIN, outline=INK_SOFT)
    for left, peak, right in pts[::2]:
        draw.polygon([left, peak, right], fill=MTN_SNOW, outline=INK_SOFT)
        # Shadow side
        draw.polygon([peak, right,
                      (right[0], peak[1]+4), (peak[0]+2, peak[1]+2)],
                     fill=MOUNTAIN)


def draw_wave_marks(draw, cx, cy, n=3, spread=20, color=SEA_LIGHT):
    """Decorative sea wave marks."""
    for i in range(n):
        x = cx - spread + i * spread
        draw.arc([x-10, cy-4, x+10, cy+4], 180, 0, fill=color, width=2)


def draw_compass_rose(draw, cx, cy, r=55):
    """Ornate compass rose."""
    # Cardinal spikes
    for angle in range(0, 360, 45):
        tip = angle_pt(cx, cy, angle - 90, r)
        lft = angle_pt(cx, cy, angle - 90 + 10, r * 0.4)
        rgt = angle_pt(cx, cy, angle - 90 - 10, r * 0.4)
        fill = INK if angle % 90 == 0 else PARCH_DK
        draw.polygon([tip, lft, (cx, cy), rgt], fill=fill, outline=INK)
    # Inner ring
    draw.ellipse([cx - r//4, cy - r//4, cx + r//4, cy + r//4],
                 fill=CRIMSON, outline=INK)
    # Centre dot
    draw.ellipse([cx - 4, cy - 4, cx + 4, cy + 4], fill=INK)
    # Cardinal labels
    for label, angle in [('N', -90), ('S', 90), ('E', 0), ('W', 180)]:
        lx, ly = angle_pt(cx, cy, angle, r + 14)
        f = ImageFont.truetype(SERIF_BOLD, 14)
        bb = draw.textbbox((0, 0), label, font=f)
        tw, th = bb[2]-bb[0], bb[3]-bb[1]
        draw.text((lx - tw//2, ly - th//2), label, font=f, fill=INK)


def draw_border(draw, size, margin=18):
    """Ornate double-rule border with corner rosettes."""
    w, h = size
    m, m2 = margin, margin + 6
    # Outer rule
    draw.rectangle([m, m, w-m, h-m], outline=GOLD, width=2)
    # Inner rule
    draw.rectangle([m2, m2, w-m2, h-m2], outline=INK, width=1)
    # Corner rosettes
    for cx, cy in [(m, m), (w-m, m), (m, h-m), (w-m, h-m)]:
        draw.ellipse([cx-7, cy-7, cx+7, cy+7], fill=GOLD, outline=INK)
        draw.ellipse([cx-3, cy-3, cx+3, cy+3], fill=INK)
    # Mid-edge flourishes
    for pos in [(w//2, m), (w//2, h-m), (m, h//2), (w-m, h//2)]:
        px, py = pos
        draw.polygon([(px, py-6), (px-4, py+4), (px+4, py+4)], fill=GOLD, outline=INK)
        draw.polygon([(px, py+6), (px-4, py-4), (px+4, py-4)], fill=GOLD, outline=INK)


def draw_cartouche(draw, cx, cy, text_lines, fonts, title_font):
    """Decorative scroll cartouche for map title."""
    line_h = 22
    total_h = len(text_lines) * line_h + 28
    maxw = max(draw.textbbox((0,0), ln, font=fonts[0])[2] for ln in text_lines)
    maxw = max(maxw, draw.textbbox((0,0), text_lines[0], font=title_font)[2])
    pw = maxw + 48
    ph = total_h
    x0, y0 = cx - pw//2, cy - ph//2

    # Scroll shadow
    draw.rectangle([x0+5, y0+5, x0+pw+5, y0+ph+5],
                   fill=(180, 155, 95))
    # Scroll body
    draw.rectangle([x0, y0, x0+pw, y0+ph], fill=PARCH_PALE, outline=GOLD, width=2)
    # Curl ends (top/bottom)
    draw.rectangle([x0-8, y0+4, x0+pw+8, y0+18], fill=PARCH_DK, outline=GOLD, width=1)
    draw.rectangle([x0-8, y0+ph-18, x0+pw+8, y0+ph-4], fill=PARCH_DK, outline=GOLD, width=1)
    # Dividing line under title
    draw.line([(x0+12, y0+line_h+14), (x0+pw-12, y0+line_h+14)], fill=GOLD, width=1)

    # Text
    y = y0 + 14
    # Title
    tb = draw.textbbox((0,0), text_lines[0], font=title_font)
    draw.text((cx - (tb[2]-tb[0])//2, y), text_lines[0], font=title_font,
              fill=INK, stroke_width=0)
    y += line_h + 4
    for ln in text_lines[1:]:
        tb = draw.textbbox((0,0), ln, font=fonts[0])
        draw.text((cx - (tb[2]-tb[0])//2, y), ln, font=fonts[0], fill=INK_MED)
        y += line_h


def label(draw, cx, cy, text, font, color=INK, halo=True, halign='c'):
    """Draw a text label, optionally with a light halo for legibility."""
    bb  = draw.textbbox((0, 0), text, font=font)
    tw  = bb[2] - bb[0]
    x   = cx - tw//2 if halign == 'c' else (cx if halign == 'l' else cx - tw)
    if halo:
        for dx, dy in [(-1,-1),(1,-1),(-1,1),(1,1),(0,-1),(0,1),(-1,0),(1,0)]:
            draw.text((x+dx, cy+dy), text, font=font, fill=PARCH_PALE)
    draw.text((x, cy), text, font=font, fill=color)


def curved_road(draw, points, color=ROAD, width=3, dash=None):
    """Draw a road as a series of line segments, optionally dashed."""
    if dash is None:
        draw.line(points, fill=color, width=width)
        return
    seg, gap = dash
    total = sum(dist(points[i], points[i+1]) for i in range(len(points)-1))
    drawn = 0.0
    drawing = True
    remaining = 0.0
    for i in range(len(points)-1):
        p0, p1 = points[i], points[i+1]
        seg_d = dist(p0, p1)
        t = 0.0
        while t < seg_d:
            if remaining <= 0:
                remaining = seg if drawing else gap
                drawing = not drawing
            step = min(remaining, seg_d - t)
            if drawing:
                tp0 = pt_lerp(p0, p1, t / seg_d)
                tp1 = pt_lerp(p0, p1, min(1.0, (t + step) / seg_d))
                draw.line([tp0, tp1], fill=color, width=width)
            t += step
            remaining -= step
            drawn += step


# ── main map builder ─────────────────────────────────────────────────────────

def build_overview_map():
    W, H = 1800, 1360
    img  = make_parchment_base((W, H), seed=7)

    # ── 1. Sea areas ─────────────────────────────────────────────────────────
    # Northern sea (above Kowloon's coast)
    sea_north = [(820,0),(1100,0),(1300,80),(1500,160),(1750,200),(W,260),(W,0)]
    soft_region(img, fuzz(sea_north, 10), SEA, blur=40, alpha=210)
    # Eastern coast sea (Mafdet's ocean)
    sea_east  = [(W,350),(W,H),(1380,H),(1250,1280),(1400,1100),(1500,900),
                 (1650,650),(1700,400),(1750,280),(W,200)]
    soft_region(img, fuzz(sea_east, 12), SEA, blur=45, alpha=215)
    # Deepen the open water
    soft_region(img, fuzz([(W-20,0),(W,0),(W,H),(W-20,H)],2), SEA_DEEP, blur=20, alpha=180)
    soft_region(img, fuzz([(820,0),(1080,0),(1200,60),(1300,0)],5), SEA_DEEP, blur=25, alpha=120)

    draw = ImageDraw.Draw(img)

    # ── 2. Terrain regions ───────────────────────────────────────────────────
    # Great Northern Forest (large oval)
    gnf_poly = poly_ring(880, 285, 240, n=48, jitter=30)
    gnf_poly  = [(x, max(60, y)) for x,y in gnf_poly]   # clip to land
    soft_region(img, gnf_poly, FOREST, blur=38, alpha=195)
    soft_region(img, poly_ring(880, 285, 150, n=40, jitter=20), FOREST_DK, blur=25, alpha=80)

    # Eccentric Woodland (south of Midgaard)
    ecc_poly = poly_ring(700, 695, 130, n=40, jitter=22)
    soft_region(img, ecc_poly, FOREST, blur=30, alpha=185)

    # Forest of Confusion + western forest complex
    foc_poly = poly_ring(390, 625, 160, n=44, jitter=28)
    soft_region(img, foc_poly, FOREST, blur=35, alpha=180)
    soft_region(img, poly_ring(390, 625, 90, n=36, jitter=18), FOREST_DK, blur=20, alpha=75)

    # Withered Depths (blight patch inside western forest)
    soft_circle(img, 310, 555, 80, SWAMP, blur=28, alpha=150)

    # Graveyard / dark woodland region (southwest)
    grave_poly = poly_ring(380, 920, 120, n=36, jitter=18)
    soft_region(img, grave_poly, SWAMP, blur=28, alpha=140)

    # Eastern Desert (large sandy region)
    desert_poly = poly_ring(1120, 540, 280, n=52, jitter=35)
    soft_region(img, desert_poly, DESERT, blur=42, alpha=200)
    soft_region(img, poly_ring(1120, 540, 175, n=44, jitter=25), DESERT_DK, blur=28, alpha=90)

    # Scorching Sands (intense desert south-east)
    soft_circle(img, 1310, 1130, 200, SCORCHED, blur=50, alpha=200)
    soft_circle(img, 1310, 1130, 110, DESERT_DK, blur=30, alpha=100)

    # Saltglass Reach (coastal/littoral strip)
    soft_circle(img, 1470, 1240, 130, DESERT, blur=35, alpha=140)

    # Scorched Wastes (northeast desert)
    soft_circle(img, 1380, 590, 90, SCORCHED, blur=30, alpha=150)

    # ── 3. Roads and paths ───────────────────────────────────────────────────
    # Roc Road: KIESS → MIDGAARD (slight curve, road)
    curved_road(draw, [(188,520),(380,508),(560,520),(690,530),(720,530)],
                ROAD, width=4)
    # Roc Road guard-border (lighter inner stripe)
    curved_road(draw, [(188,520),(380,508),(560,520),(690,530),(720,530)],
                ROAD_LT, width=2)

    # Lantern Road: MIDGAARD → KOWLOON (through Great Northern Forest)
    curved_road(draw, [(720,530),(730,460),(760,380),(810,290),(875,210),(930,140)],
                ROAD, width=4)
    curved_road(draw, [(720,530),(730,460),(760,380),(810,290),(875,210),(930,140)],
                ROAD_LT, width=2)

    # Eastern road: MIDGAARD → EASTERN DESERT → Great Oasis
    curved_road(draw, [(720,530),(820,520),(920,500),(1020,480),(1090,490),
                       (1130,530),(1150,620),(1160,720)],
                ROAD, width=3, dash=(14, 6))

    # Corridor south: Great Oasis → Oases → Pyramids → Scorching Sands → Mafdet
    curved_road(draw, [(1160,720),(1060,840),(980,980),(1010,1070),(1090,1120),
                       (1190,1130),(1300,1130),(1410,1200),(1520,1270)],
                ROAD, width=3, dash=(14, 6))

    # Southern road: MIDGAARD → Eccentric Woodland → RAKUEN
    curved_road(draw, [(720,530),(710,600),(700,680),(690,780),(670,895)],
                ROAD, width=3, dash=(12, 5))

    # Akhenet spur (off Eastern Desert main road)
    curved_road(draw, [(1020,480),(1060,450),(1100,430)],
                ROAD, width=2, dash=(8, 5))

    # ── 4. Rivers ────────────────────────────────────────────────────────────
    # Iseth Reach (vanished river — shown as faint dotted blue)
    curved_road(draw, [(1120,300),(1100,420),(1080,540),(1110,650),(1140,720)],
                RIVER, width=2, dash=(4, 8))
    # Kowloon delta river
    curved_road(draw, [(930,140),(920,200),(900,240),(890,290)],
                RIVER, width=3)
    # Small forest streams
    curved_road(draw, [(700,450),(720,490),(720,530)],
                RIVER, width=2)

    # ── 5. Tree symbols ──────────────────────────────────────────────────────
    # Great Northern Forest
    for fx, fy in [(750,200),(820,230),(900,210),(970,220),(840,280),(920,290),
                   (780,320),(860,340),(940,360),(770,370),(1010,250),(1020,320),
                   (830,180),(980,180),(700,270),(700,330),(1060,270)]:
        draw_trees(draw, fx, fy, n=4, spread=18, color=FOREST_DK)

    # Eccentric Woodland
    for fx, fy in [(640,650),(700,670),(750,655),(690,720),(740,730),(660,700)]:
        draw_trees(draw, fx, fy, n=3, spread=14, color=FOREST_DK)

    # Forest of Confusion
    for fx, fy in [(340,580),(410,600),(360,640),(430,660),(380,700),(440,720)]:
        draw_trees(draw, fx, fy, n=3, spread=14, color=FOREST_DK)

    # ── 6. Desert features: dune hatch lines ─────────────────────────────────
    draw_dune_marks(draw, 1150, 560, n=6, spacing=18)
    draw_dune_marks(draw, 1250, 620, n=5, spacing=16)
    draw_dune_marks(draw, 1290, 1120, n=8, spacing=20)
    draw_dune_marks(draw, 1340, 1050, n=7, spacing=18)

    # ── 7. Sea features ──────────────────────────────────────────────────────
    for wx, wy in [(1580,120),(1660,180),(1620,250),(1700,320),(1730,440),
                   (1680,550),(1720,620),(1690,720),(1650,820),(1700,920)]:
        draw_wave_marks(draw, wx, wy, n=3, spread=22, color=SEA_LIGHT)

    # ── 8. Mountains (minor) ─────────────────────────────────────────────────
    draw_mountains(draw, 330, 425, n=3, spread=25, color=MOUNTAIN)
    draw_mountains(draw, 470, 430, n=4, spread=30, color=MOUNTAIN)

    # ── 9. POI markers ───────────────────────────────────────────────────────
    # Major cities
    draw_city_marker(draw, 720, 530,  r=13, color=INK)          # MIDGAARD
    draw_city_marker(draw, 930, 140,  r=12, color=INK)          # KOWLOON
    draw_city_marker(draw, 188, 520,  r=11, color=INK)          # KIESS
    draw_city_marker(draw, 668, 905,  r=10, color=INK)          # RAKUEN
    draw_city_marker(draw, 1530, 1260, r=11, color=INK)         # MAFDET

    # Secondary cities
    draw_town_marker(draw, 1090, 440, r=7)                       # Akh'enet
    draw_town_marker(draw, 985,  525, r=6)                       # Arroyo

    # Oases
    draw_oasis_icon(draw, 1160, 730, r=9)                        # Great Oasis
    draw_oasis_icon(draw, 1050, 850, r=8)                        # Northern Oasis
    draw_oasis_icon(draw, 1280, 880, r=8)                        # Southern Oasis

    # Pyramids
    draw_pyramid_icon(draw, 980,  970,  size=14)                 # N Pyramid
    draw_pyramid_icon(draw, 1215, 1050, size=14)                 # S Pyramid
    draw_pyramid_icon(draw, 1095, 1100, size=18)                 # Great Pyramid

    # Ruin/special sites
    draw_ruin_icon(draw, 1100, 440, size=7)                      # Akh'enet
    draw_ruin_icon(draw, 1220, 660, size=6)                      # Lost City
    draw_ruin_icon(draw, 1280, 545, size=6)                      # Sultan's Palace
    draw_ruin_icon(draw, 1115, 1195, size=7)                     # Khar'Daan
    draw_ruin_icon(draw, 1120, 535, size=6)                      # Arroyo canyon

    # Dungeon anchors (small symbols near Midgaard)
    draw_dungeon_icon(draw, 750, 560, size=5)                    # Gloamvault
    draw_dungeon_icon(draw, 770, 495, size=5)                    # Nightfall Catacombs
    draw_dungeon_icon(draw, 756, 530, size=5)                    # Public Dungeons

    # Western dungeon cluster
    draw_dungeon_icon(draw, 335, 460, size=5)                    # Kel'Shadra/Crypts
    draw_dungeon_icon(draw, 275, 480, size=5)                    # Void Citadel
    draw_ruin_icon(draw, 370, 900, size=6)                       # Graveyard
    draw_ruin_icon(draw, 280, 960, size=6)                       # Shadowmere
    draw_ruin_icon(draw, 460, 975, size=6)                       # Thornwood

    # ── 10. Text labels ──────────────────────────────────────────────────────
    f_title  = ImageFont.truetype(SERIF_BOLD, 22)
    f_city   = ImageFont.truetype(SERIF_BOLD, 17)
    f_town   = ImageFont.truetype(SERIF_BOLD, 14)
    f_region = ImageFont.truetype(SERIF,      13)
    f_small  = ImageFont.truetype(SERIF,      11)
    f_italic = ImageFont.truetype(SERIF,      12)

    # Major cities
    label(draw, 720, 510,  "Midgaard",        f_city,   INK)
    label(draw, 930, 108,  "Kowloon",          f_city,   INK)
    label(draw, 174, 499,  "Kiess",            f_city,   INK)
    label(draw, 668, 883,  "Rakuen",           f_city,   INK)
    label(draw, 1530, 1238,"Mafdet",           f_city,   INK)

    # Secondary cities/sites
    label(draw, 1090, 415, "Akh'enet",         f_town,   INK_SOFT)
    label(draw, 985,  502, "Arroyo",           f_small,  INK_SOFT)
    label(draw, 1220, 635, "Lost City",        f_small,  RUIN)
    label(draw, 1284, 522, "Sultan's Palace",  f_small,  RUIN)
    label(draw, 1115, 1172,"Khar'Daan",        f_small,  RUIN)

    # Oases
    label(draw, 1160, 706, "Great Oasis",      f_town,   OASIS_CLR)
    label(draw, 1050, 826, "N. Oasis",         f_small,  OASIS_CLR)
    label(draw, 1280, 856, "S. Oasis",         f_small,  OASIS_CLR)

    # Pyramids
    label(draw, 980,  940, "Northern Pyramid", f_small,  INK_MED)
    label(draw, 1215, 1020,"Southern Pyramid", f_small,  INK_MED)
    label(draw, 1095, 1128,"Great Pyramid",    f_town,   INK)

    # Dungeon icons (near Midgaard)
    label(draw, 800,  554, "Gloamvault †",     f_small,  INK_SOFT)
    label(draw, 808,  489, "Catacombs †",      f_small,  INK_SOFT)

    # Kel'Shadra cluster
    label(draw, 300,  440, "Kel'Shadra †",     f_small,  INK_SOFT)

    # Region names
    label(draw, 880,  268, "Great Northern Forest",  f_region, FOREST_DK)
    label(draw, 700,  670, "Eccentric\nWoodland",    f_region, FOREST_DK)
    label(draw, 390,  610, "Forest of\nConfusion",   f_region, FOREST_DK)
    label(draw, 310,  520, "Withered\nDepths",        f_small,  SWAMP)
    label(draw, 380,  895, "Graveyard\nRegion",       f_small,  INK_SOFT)
    label(draw, 1110, 520, "Eastern Desert",          f_region, DESERT_DK)
    label(draw, 1300, 1110,"Scorching Sands",         f_region, SCORCHED)
    label(draw, 1440, 1215,"Saltglass\nReach",        f_small,  INK_SOFT)

    # Road names
    label(draw, 445,  488, "Roc Road",                f_small,  ROAD)
    label(draw, 770,  402, "Lantern Road",             f_small,  ROAD)

    # Sea labels
    label(draw, 1650, 120, "The Northern Sea",        f_italic, SEA_DEEP)
    label(draw, 1680, 680, "The Eastern\nSea",        f_italic, SEA_DEEP)

    # Iseth Reach (vanished river label)
    label(draw, 1050, 470, "Iseth Reach ✦",           f_small,  RIVER)

    # ── 11. Decorative elements ──────────────────────────────────────────────
    draw_compass_rose(draw, W - 115, 115, r=60)
    draw_border(draw, (W, H), margin=20)

    # Title cartouche
    f_ctitle = ImageFont.truetype(SERIF_BOLD, 20)
    f_csub   = ImageFont.truetype(SERIF,      13)
    draw_cartouche(draw, 250, 1270,
                   ["The Known World",
                    "ACK!TNG Historical Archive"],
                   [f_csub], f_ctitle)

    # Scale bar
    draw_scale_bar(draw, W - 270, H - 55, length=160, label="Travel Distance")

    # "† underground / portal access" footnote
    fn = ImageFont.truetype(SERIF, 10)
    label(draw, 45, H - 32, "† Portal or underground access only", fn,
          INK_SOFT, halo=False, halign='l')

    return img


def draw_dune_marks(draw, cx, cy, n=5, spacing=16, color=DESERT_DK):
    """Stylised sand dune hatch marks (arcs)."""
    for i in range(n):
        x = cx + (i - n//2) * spacing
        y = cy + abs(i - n//2) * 3
        draw.arc([x-12, y-5, x+12, y+5], 180, 0, fill=color, width=1)


def draw_scale_bar(draw, x, y, length=150, label="Scale"):
    """Simple map scale bar."""
    f = ImageFont.truetype(SERIF, 11)
    # Alternating black/white segments
    seg = length // 4
    for i in range(4):
        fill = INK if i % 2 == 0 else PARCH_PALE
        draw.rectangle([x + i*seg, y-6, x + (i+1)*seg, y+6],
                       fill=fill, outline=INK)
    # End caps
    draw.rectangle([x, y-6, x+length, y+6], outline=INK, width=1)
    # Label
    tb = draw.textbbox((0,0), label, font=f)
    draw.text((x + length//2 - (tb[2]-tb[0])//2, y + 9), label, font=f, fill=INK_SOFT)


# ── corridor map ─────────────────────────────────────────────────────────────

def build_corridor_map():
    """Zoomed detail of the Oasis-Pyramid corridor."""
    W, H = 1050, 1380
    img  = make_parchment_base((W, H), seed=13)

    # Sea strip on right edge (Saltglass Reach → Mafdet)
    sea_poly = [(700,900),(W,780),(W,H),(600,H)]
    soft_region(img, fuzz(sea_poly, 12), SEA, blur=40, alpha=205)
    soft_region(img, fuzz([(850,1000),(W,900),(W,H),(800,H)],8), SEA_DEEP, blur=30, alpha=160)

    # Desert fills
    soft_region(img, poly_ring(520, 380, 430, n=60, jitter=45), DESERT, blur=55, alpha=200)
    soft_region(img, poly_ring(520, 380, 260, n=50, jitter=30), DESERT_DK, blur=35, alpha=90)
    soft_circle(img, 620, 1080, 250, SCORCHED, blur=60, alpha=200)
    soft_circle(img, 620, 1080, 130, DESERT_DK, blur=30, alpha=100)
    soft_circle(img, 750, 1220, 160, DESERT, blur=40, alpha=140)

    draw = ImageDraw.Draw(img)

    # Main corridor road (spine)
    curved_road(draw,
                [(515, 80),(510,180),(505,300),(510,420),(520,530),
                 (490,650),(460,760),(470,860),(510,960),
                 (540,1050),(600,1130),(680,1210),(760,1290)],
                ROAD, width=5)
    curved_road(draw,
                [(515, 80),(510,180),(505,300),(510,420),(520,530),
                 (490,650),(460,760),(470,860),(510,960),
                 (540,1050),(600,1130),(680,1210),(760,1290)],
                ROAD_LT, width=2)

    # Branch: Great Oasis ↔ Northern Oasis
    curved_road(draw, [(490,620),(380,620),(280,640),(240,700)], ROAD, width=3, dash=(10,5))
    # Branch: Great Oasis ↔ Southern Oasis
    curved_road(draw, [(510,660),(590,680),(660,720),(700,780)], ROAD, width=3, dash=(10,5))
    # Northern Pyramid spur
    curved_road(draw, [(240,700),(200,780),(190,870)],           ROAD, width=3, dash=(10,5))
    # Southern Pyramid spur
    curved_road(draw, [(700,780),(720,860),(730,950)],           ROAD, width=3, dash=(10,5))
    # Arroyo spur
    curved_road(draw, [(520,300),(620,340),(680,380)],           ROAD, width=2, dash=(7,5))
    # Sultan's Palace spur
    curved_road(draw, [(530,380),(650,390),(730,420)],           ROAD, width=2, dash=(7,5))

    # Dune marks
    for dx, dy in [(300,200),(400,250),(500,200),(600,220),(700,310),
                   (400,480),(550,500),(650,560),(400,800),(500,830),
                   (620,1050),(700,1100)]:
        draw_dune_marks(draw, dx, dy, n=5, spacing=15)

    # Sea waves
    for wx, wy in [(880,950),(950,1050),(920,1150),(980,1250),(850,1300)]:
        draw_wave_marks(draw, wx, wy, n=3, spread=20)

    # Iseth Reach (vanished)
    curved_road(draw, [(510,80),(515,180),(520,300),(510,420)],
                RIVER, width=2, dash=(4,10))

    # POI markers
    draw_town_marker(draw, 515, 80,   r=8)                    # Eastern Desert entry
    draw_oasis_icon( draw, 490, 620,  r=11)                   # Great Oasis
    draw_oasis_icon( draw, 240, 700,  r=9)                    # Northern Oasis
    draw_oasis_icon( draw, 700, 780,  r=9)                    # Southern Oasis
    draw_pyramid_icon(draw, 190, 870, size=16)                # Northern Pyramid
    draw_pyramid_icon(draw, 730, 950, size=16)                # Southern Pyramid
    draw_pyramid_icon(draw, 500, 1020, size=22)               # Great Pyramid (large)
    draw_city_marker(draw, 760, 1290, r=12, color=INK)        # Mafdet
    draw_ruin_icon(  draw, 680, 380,  size=8)                 # Sultan's Palace
    draw_ruin_icon(  draw, 640, 320,  size=7)                 # Arroyo
    draw_ruin_icon(  draw, 520, 480,  size=8)                 # Lost City (Khepra-Lesh)
    draw_ruin_icon(  draw, 510, 1140, size=8)                 # Khar'Daan

    # Labels
    f_title  = ImageFont.truetype(SERIF_BOLD, 19)
    f_city   = ImageFont.truetype(SERIF_BOLD, 16)
    f_town   = ImageFont.truetype(SERIF_BOLD, 13)
    f_region = ImageFont.truetype(SERIF,      13)
    f_small  = ImageFont.truetype(SERIF,      11)

    label(draw, 515,  52,  "Eastern Desert",    f_town,   INK)
    label(draw, 490,  592, "Great Oasis",        f_town,   OASIS_CLR)
    label(draw, 240,  672, "Northern Oasis",     f_town,   OASIS_CLR)
    label(draw, 700,  752, "Southern Oasis",     f_town,   OASIS_CLR)
    label(draw, 190,  836, "Northern Pyramid",   f_town,   INK_MED)
    label(draw, 730,  918, "Southern Pyramid",   f_town,   INK_MED)
    label(draw, 500,  990, "Great Pyramid",      f_city,   INK)
    label(draw, 760, 1262, "Mafdet",             f_city,   INK)
    label(draw, 680,  354, "Sultan's Palace",    f_small,  RUIN)
    label(draw, 645,  294, "Arroyo",             f_small,  RUIN)
    label(draw, 520,  452, "Lost City",          f_small,  RUIN)
    label(draw, 510, 1112, "Khar'Daan",          f_small,  RUIN)
    label(draw, 600, 1100, "Scorching Sands",    f_region, SCORCHED)
    label(draw, 740, 1205, "Saltglass Reach",    f_small,  INK_SOFT)
    label(draw, 910, 1050, "The Eastern Sea",    f_small,  SEA_DEEP)
    label(draw, 520,  175, "Iseth Reach (lost)", f_small,  RIVER)

    # Deepwell annotation
    label(draw, 490, 820,
          "Deepwell Confluence\n(shared aquifer)",
          ImageFont.truetype(SERIF, 10), INK_SOFT)

    draw_compass_rose(draw, W - 95, 95, r=52)
    draw_border(draw, (W, H), margin=16)

    f_ctitle = ImageFont.truetype(SERIF_BOLD, 18)
    f_csub   = ImageFont.truetype(SERIF, 12)
    draw_cartouche(draw, W//2, H - 55,
                   ["Oasis-Pyramid Corridor",
                    "the civilizational spine"],
                   [f_csub], f_ctitle)

    return img


# ── route diagram maps ────────────────────────────────────────────────────────

def build_route_diagram(title, subtitle, nodes, arrow_color=ROAD,
                        w=900, h=380, seed=99):
    """Build an elegant route-chain diagram on parchment."""
    img  = make_parchment_base((w, h), seed=seed)
    draw = ImageDraw.Draw(img)

    f_title = ImageFont.truetype(SERIF_BOLD, 20)
    f_sub   = ImageFont.truetype(SERIF,      13)
    f_node  = ImageFont.truetype(SERIF_BOLD, 14)
    f_note  = ImageFont.truetype(SERIF,      11)

    # Draw title
    label(draw, w//2, 30, title,    f_title, INK)
    label(draw, w//2, 56, subtitle, f_sub,   INK_SOFT)

    # Draw the chain of nodes
    n   = len(nodes)
    pad = 70
    yw  = h // 2 + 18
    if n == 1:
        xs = [w//2]
    else:
        xs  = [pad + i * (w - 2*pad) // (n-1) for i in range(n)]

    for i, (name, note) in enumerate(nodes):
        x = xs[i]
        # Node circle
        r = 18
        draw.ellipse([x-r, yw-r, x+r, yw+r], fill=PARCH_PALE, outline=INK, width=2)
        draw.ellipse([x-5, yw-5, x+5, yw+5], fill=INK)
        # Name above
        label(draw, x, yw - r - 18, name, f_node, INK)
        # Note below
        if note:
            label(draw, x, yw + r + 6,  note, f_note, INK_SOFT)
        # Arrow to next
        if i < n - 1:
            x2 = xs[i+1]
            ax1, ax2 = x + r + 4, x2 - r - 4
            draw.line([(ax1, yw), (ax2, yw)], fill=arrow_color, width=3)
            # Arrowhead
            draw.polygon([(ax2, yw), (ax2-10, yw-5), (ax2-10, yw+5)],
                         fill=arrow_color)

    draw_border(draw, (w, h), margin=14)
    return img


# ── entry point ───────────────────────────────────────────────────────────────

def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    print("Generating overview map …")
    img = build_overview_map()
    img.save(str(OUT_DIR / "world_map_overview.png"), "PNG")
    print(f"  world_map_overview.png  {img.size[0]}×{img.size[1]}")

    print("Generating corridor map …")
    img = build_corridor_map()
    img.save(str(OUT_DIR / "world_map_corridor.png"), "PNG")
    print(f"  world_map_corridor.png  {img.size[0]}×{img.size[1]}")

    print("Generating five-city diagram …")
    img = build_route_diagram(
        title    = "The Five-City Network",
        subtitle = "origin  ·  custody  ·  adjudication  ·  hazard  ·  sea transfer",
        nodes    = [
            ("Kiess",     "western terminus\nEvermeet ruins"),
            ("Midgaard",  "inland registry\ncentral hub"),
            ("Kowloon",   "northern port\ndelta trade"),
            ("Rakuen",    "southern city\ndisaster recovery"),
            ("Mafdet",    "maritime port\nsea terminus"),
        ],
        w=960, h=360, seed=55,
    )
    img.save(str(OUT_DIR / "world_map_five_cities.png"), "PNG")
    print(f"  world_map_five_cities.png  {img.size[0]}×{img.size[1]}")

    print("Generating desert route diagram …")
    img = build_route_diagram(
        title    = "Desert-to-Sea Route",
        subtitle = "the southern corridor from the Great Pyramid to the coast",
        nodes    = [
            ("Great Pyramid",    "Black Sun Shard\ncontainment"),
            ("Scorching Sands",  "heat-cert transit\nbelt"),
            ("Saltglass Reach",  "inland → maritime\nlegal handoff"),
            ("Mafdet",           "sea port\ndual-attestation"),
        ],
        w=880, h=340, seed=77,
    )
    img.save(str(OUT_DIR / "world_map_desert_route.png"), "PNG")
    print(f"  world_map_desert_route.png  {img.size[0]}×{img.size[1]}")

    print("Done.")


if __name__ == "__main__":
    main()
