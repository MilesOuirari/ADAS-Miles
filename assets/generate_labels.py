#!/usr/bin/env python3
"""Generate object label textures for ADAS HMI."""
from PIL import Image, ImageDraw, ImageFont
import os

os.makedirs("assets/textures/labels", exist_ok=True)

labels = ["CAR", "TRUCK", "BUS", "BIKE", "PERSON"]
colors = {
    "CAR": (100, 150, 255),      # Blue
    "TRUCK": (80, 80, 80),        # Dark Grey
    "BUS": (255, 180, 50),        # Orange
    "BIKE": (50, 200, 50),        # Green
    "PERSON": (255, 100, 100),    # Pink
}

for label in labels:
    img = Image.new("RGBA", (128, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 20)
    except:
        font = ImageFont.load_default()
    
    # Get text bounding box
    bbox = draw.textbbox((0, 0), label, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    
    x = (128 - text_width) // 2
    y = (32 - text_height) // 2 - 2
    
    # Draw shadow
    draw.text((x+1, y+1), label, font=font, fill=(0, 0, 0, 180))
    # Draw main text
    draw.text((x, y), label, font=font, fill=colors.get(label, (255, 255, 255)) + (255,))
    
    filename = f"assets/textures/labels/{label.lower()}.png"
    img.save(filename)
    print(f"Generated: {filename}")

print("Label textures generated successfully!")
