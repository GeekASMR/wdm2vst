from PIL import Image
import struct, io, math
from collections import deque

src_path = r'C:\Users\Administrator\.gemini\antigravity\brain\ccaf5d43-c964-49ce-a22e-f2918a21fad3\wdm2vst_logo_v3_1774030077234.png'
out_logo = r'D:\wdm2vst\ASMRTOP_Plugins\installer\wdm2vst_logo.png'
out_small = r'D:\wdm2vst\ASMRTOP_Plugins\installer\wizard_small.png'
out_ico = r'D:\wdm2vst\ASMRTOP_Plugins\installer\wdm2vst.ico'

img = Image.open(src_path).convert('RGBA')
w, h = img.size
pixels = img.load()

# Sample bg color from corners
corners = [pixels[0,0], pixels[w-1,0], pixels[0,h-1], pixels[w-1,h-1]]
bgR = sum(c[0] for c in corners) // 4
bgG = sum(c[1] for c in corners) // 4
bgB = sum(c[2] for c in corners) // 4
print(f"Background color: ({bgR}, {bgG}, {bgB})")

threshold = 50

# Flood fill from edges
visited = [[False]*h for _ in range(w)]
is_bg = [[False]*h for _ in range(w)]

queue = deque()
for x in range(w):
    queue.append((x, 0))
    queue.append((x, h-1))
for y in range(1, h-1):
    queue.append((0, y))
    queue.append((w-1, y))

while queue:
    x, y = queue.popleft()
    if x < 0 or x >= w or y < 0 or y >= h: continue
    if visited[x][y]: continue
    visited[x][y] = True
    
    r, g, b, a = pixels[x, y]
    dist = math.sqrt((r-bgR)**2 + (g-bgG)**2 + (b-bgB)**2)
    
    if dist < threshold:
        is_bg[x][y] = True
        for dx, dy in [(1,0),(-1,0),(0,1),(0,-1)]:
            nx, ny = x+dx, y+dy
            if 0 <= nx < w and 0 <= ny < h and not visited[nx][ny]:
                queue.append((nx, ny))

# Apply transparency
bg_count = 0
edge_count = 0
for y in range(h):
    for x in range(w):
        if is_bg[x][y]:
            pixels[x, y] = (0, 0, 0, 0)
            bg_count += 1
        else:
            # Soft edge near background
            near_bg = False
            for dx in range(-2, 3):
                for dy in range(-2, 3):
                    nx, ny = x+dx, y+dy
                    if 0 <= nx < w and 0 <= ny < h and is_bg[nx][ny]:
                        near_bg = True
                        break
                if near_bg: break
            if near_bg:
                r, g, b, a = pixels[x, y]
                dist = math.sqrt((r-bgR)**2 + (g-bgG)**2 + (b-bgB)**2)
                if dist < threshold * 2:
                    alpha = int(min(255, dist / (threshold * 2) * 255))
                    pixels[x, y] = (r, g, b, alpha)
                    edge_count += 1

print(f"Removed {bg_count} bg pixels, softened {edge_count} edges")
img.save(out_logo)
print(f"Logo saved: {out_logo}")

# wizard_small.png 147x147 transparent
small = img.resize((147, 147), Image.LANCZOS)
small.save(out_small)
print(f"wizard_small.png saved")

# ICO
sizes = [256, 128, 64, 48, 32, 16]
png_data = []
for s in sizes:
    resized = img.resize((s, s), Image.LANCZOS)
    buf = io.BytesIO()
    resized.save(buf, format='PNG')
    png_data.append(buf.getvalue())

ico = io.BytesIO()
ico.write(struct.pack('<HHH', 0, 1, len(sizes)))
offset = 6 + 16 * len(sizes)
for i, s in enumerate(sizes):
    d = png_data[i]
    ico.write(struct.pack('<BBBBHHII',
        0 if s >= 256 else s,
        0 if s >= 256 else s,
        0, 0, 1, 32, len(d), offset))
    offset += len(d)
for d in png_data:
    ico.write(d)

with open(out_ico, 'wb') as f:
    f.write(ico.getvalue())
print(f"ICO saved: {out_ico}")
print("Done!")
