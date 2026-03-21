from PIL import Image, ImageChops, ImageFilter
import struct, io, math

src_path = r'C:\Users\Administrator\.gemini\antigravity\brain\ccaf5d43-c964-49ce-a22e-f2918a21fad3\wdm2vst_clean_logo_v4_1774030452255.png'
out_logo = r'D:\wdm2vst\ASMRTOP_Plugins\installer\wdm2vst_logo.png'
out_small = r'D:\wdm2vst\ASMRTOP_Plugins\installer\wizard_small.png'
out_ico = r'D:\wdm2vst\ASMRTOP_Plugins\installer\wdm2vst.ico'

img = Image.open(src_path).convert('RGBA')
w, h = img.size
pixels = img.load()

# Step 1: Extract alpha from brightness (since it's on black)
# The logo is a glowing waveform on black, so its brightness is its transparency
for y in range(h):
    for x in range(w):
        r, g, b, a = pixels[x, y]
        # Calculate luminescence/perceived brightness
        brightness = int(0.2126 * r + 0.7152 * g + 0.0722 * b)
        # Use brightness as new alpha
        # We also want to boost the colors slightly to make it look less 'muddy' when transparent
        if brightness > 0:
            boost = 1.0 + (1.0 - brightness/255.0) * 0.5
            new_r = min(int(r * boost), 255)
            new_g = min(int(g * boost), 255)
            new_b = min(int(b * boost), 255)
            # Ensure the glow isn't cut off too early (tweak the curve)
            new_alpha = int(pow(brightness / 255.0, 0.7) * 255)
            pixels[x, y] = (new_r, new_g, new_b, new_alpha)
        else:
            pixels[x, y] = (0, 0, 0, 0)

# Step 2: Crop to content
bbox = img.getbbox()
if bbox:
    img = img.crop(bbox)
    # Give it some padding
    w, h = img.size
    final_size = max(w, h) + 20
    new_img = Image.new('RGBA', (final_size, final_size), (0, 0, 0, 0))
    new_img.paste(img, ((final_size - w) // 2, (final_size - h) // 2))
    img = new_img

img.save(out_logo)
print(f"Branded transparent logo saved: {out_logo}")

# Create small version (147x147)
small = img.resize((147, 147), Image.LANCZOS)
small.save(out_small)

# Create ICO
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

print("All branding assets updated with TRUE transparency. Done!")
