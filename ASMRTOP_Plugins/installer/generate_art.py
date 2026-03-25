import os
import sys
import subprocess

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', 'pillow'])
    from PIL import Image, ImageDraw, ImageFont

# Sizes
W_LARGE, H_LARGE = 164, 314
W_SMALL, H_SMALL = 55, 58

# Colors
BG_COLOR = (22, 23, 27)      # Dark grey/blueish from the screenshot
ACCENT_PINK = (255, 30, 95)  # The bright pink from the line/slider
TEXT_WHITE = (255, 255, 255)
TEXT_GRAY = (150, 150, 150)
GREEN_BAR = (0, 200, 50)     # From the volume meters

# 1. Generate WizardImageFile (164x314)
img_large = Image.new('RGB', (W_LARGE, H_LARGE), BG_COLOR)
draw_l = ImageDraw.Draw(img_large)

# Draw accent lines at the top
draw_l.rectangle([0, 0, W_LARGE, 4], fill=ACCENT_PINK)
draw_l.rectangle([0, 10, W_LARGE//2, 12], fill=ACCENT_PINK)

# Draw some UI mockup elements in the background to match the vibe
y_offset = 60
for i in range(3):
    draw_l.rounded_rectangle([15, y_offset, W_LARGE-15, y_offset+25], radius=4, fill=(30, 32, 36), outline=(50, 50, 55))
    draw_l.rectangle([25, y_offset+10, 25+40, y_offset+15], fill=TEXT_GRAY) # fake text
    draw_l.ellipse([W_LARGE-30, y_offset+8, W_LARGE-20, y_offset+18], fill=ACCENT_PINK) # fake slider knob
    y_offset += 35

# Try to use Arial, fallback to default
try:
    font_large = ImageFont.truetype('arialbd.ttf', 20)
    font_small = ImageFont.truetype('arial.ttf', 10)
except:
    font_large = ImageFont.load_default()
    font_small = ImageFont.load_default()

# Text directly mapped to the vibe of the installer
draw_l.text((15, 25), 'WDM2VST', fill=TEXT_WHITE, font=font_large)
draw_l.text((15, 50), 'AUDIO ROUTER', fill=ACCENT_PINK, font=font_small)

# Draw green level meters at the bottom
draw_l.rectangle([15, H_LARGE-40, W_LARGE-40, H_LARGE-34], fill=(30, 32, 36))
draw_l.rectangle([15, H_LARGE-40, 15+60, H_LARGE-34], fill=GREEN_BAR)

draw_l.rectangle([15, H_LARGE-25, W_LARGE-25, H_LARGE-19], fill=(30, 32, 36))
draw_l.rectangle([15, H_LARGE-25, 15+90, H_LARGE-19], fill=GREEN_BAR)

draw_l.text((15, H_LARGE-70), 'Memory Mapped', fill=TEXT_GRAY, font=font_small)
draw_l.text((15, H_LARGE-55), 'SYNC LATENCY: 7.3ms', fill=ACCENT_PINK, font=font_small)

img_large.save('D:/Autigravity/wdm2vst/ASMRTOP_Plugins/installer/wizard_image.bmp')

# 2. Generate WizardSmallImageFile (55x58)
img_small = Image.new('RGB', (W_SMALL, H_SMALL), BG_COLOR)
draw_s = ImageDraw.Draw(img_small)

# Small top pink line
draw_s.rectangle([0, 0, W_SMALL, 3], fill=ACCENT_PINK)

# Little abstract routing icon
draw_s.rectangle([10, 15, 20, 25], fill=TEXT_WHITE)
draw_s.rectangle([35, 30, 45, 40], fill=TEXT_GRAY)
draw_s.line([20, 20, 35, 35], fill=ACCENT_PINK, width=2)
draw_s.ellipse([30, 30, 40, 40], fill=ACCENT_PINK)

draw_s.text((10, 42), 'IPC', fill=TEXT_WHITE, font=font_small)

img_small.save('D:/Autigravity/wdm2vst/ASMRTOP_Plugins/installer/wizard_small.bmp')

print('Images generated successfully.')
