#!/usr/bin/env python3
"""Generate maneuver label textures for ADAS HMI."""
from PIL import Image, ImageDraw, ImageFont
import os

os.makedirs("assets/textures/maneuvers", exist_ok=True)

labels = {
    "keep_lane": ("KEEP LANE", (100, 200, 100)),
    "lane_left": ("LANE LEFT", (100, 150, 255)),
    "lane_right": ("LANE RIGHT", (100, 150, 255)),
}

for key, (text, color) in labels.items():
    img = Image.new("RGBA", (256, 48), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 28)
    except:
        font = ImageFont.load_default()
    
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    
    x = (256 - text_width) // 2
    y = (48 - text_height) // 2 - 2
    
    # Shadow
    draw.text((x+2, y+2), text, font=font, fill=(0, 0, 0, 180))
    # Main text
    draw.text((x, y), text, font=font, fill=color + (255,))
    
    filename = f"assets/textures/maneuvers/{key}.png"
    img.save(filename)
    print(f"Generated: {filename}")

print("Maneuver textures generated!")
