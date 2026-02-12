#!/usr/bin/env python3
"""Generate scenario name textures for ADAS HMI."""
from PIL import Image, ImageDraw, ImageFont
import os

os.makedirs("assets/textures/scenarios", exist_ok=True)

scenarios = [
    ("highway", "HIGHWAY CRUISE"),
    ("city", "CITY DRIVING"),
    ("emergency", "EMERGENCY BRAKE"),
    ("lane_change", "LANE CHANGE"),
    ("jam", "TRAFFIC JAM"),
    ("pedestrian", "PEDESTRIAN"),
    ("intersection", "INTERSECTION"),
    ("merge", "HIGHWAY MERGE"),
]

for key, text in scenarios:
    img = Image.new("RGBA", (320, 40), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 22)
    except:
        font = ImageFont.load_default()
    
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    
    x = (320 - text_width) // 2
    y = (40 - text_height) // 2 - 2
    
    # Shadow
    draw.text((x+2, y+2), text, font=font, fill=(0, 0, 0, 200))
    # Main text - cyan/blue color
    draw.text((x, y), text, font=font, fill=(100, 200, 255, 255))
    
    filename = f"assets/textures/scenarios/{key}.png"
    img.save(filename)
    print(f"Generated: {filename}")

print("Scenario textures generated!")
