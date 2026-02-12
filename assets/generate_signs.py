from PIL import Image, ImageDraw, ImageFont
import os

os.makedirs('assets/textures', exist_ok=True)

def create_speed_limit(limit):
    size = (256, 256)
    img = Image.new('RGBA', size, (0,0,0,0))
    draw = ImageDraw.Draw(img)
    
    # White Circle
    draw.ellipse((10, 10, 246, 246), fill='white', outline='red', width=30)
    
    # Text
    # Load fallback font
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 100)
    except:
        font = ImageFont.load_default()
        
    text = str(limit)
    bbox = draw.textbbox((0,0), text, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    
    draw.text(((256-w)/2, (256-h)/2 - 10), text, fill='black', font=font)
    
    img.save(f'assets/textures/limit_{limit}.png')

def create_stop():
    size = (256, 256)
    img = Image.new('RGBA', size, (0,0,0,0))
    draw = ImageDraw.Draw(img)
    
    # Red Octagon
    # 8 points
    import math
    points = []
    r = 120
    cx, cy = 128, 128
    for i in range(8):
        angle = math.pi/8 + i * math.pi/4
        x = cx + r * math.cos(angle)
        y = cy + r * math.sin(angle)
        points.append((x,y))
        
    draw.polygon(points, fill='red', outline='white', width=5)
    
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 60)
    except:
        font = ImageFont.load_default()
        
    text = "STOP"
    bbox = draw.textbbox((0,0), text, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    draw.text(((256-w)/2, (256-h)/2), text, fill='white', font=font)
    
    img.save('assets/textures/stop_sign.png')

def create_light_red():
    size = (128, 256)
    img = Image.new('RGBA', size, (50, 50, 50, 255))
    draw = ImageDraw.Draw(img)
    
    # Lights
    draw.ellipse((34, 20, 94, 80), fill=(255, 0, 0)) # Red ON
    draw.ellipse((34, 98, 94, 158), fill=(50, 40, 0)) # Yellow OFF
    draw.ellipse((34, 176, 94, 236), fill=(0, 40, 0)) # Green OFF
    
    img.save('assets/textures/light_red.png')

create_speed_limit(30)
create_speed_limit(50)
create_speed_limit(80)
create_speed_limit(100)
create_speed_limit(120)
create_stop()
create_light_red()
print("Textures Generated.")
