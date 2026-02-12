import FreeCAD
import Part
import math

def create_car():
    doc = FreeCAD.newDocument("CarDesign")
    
    # Car Parameters
    chassis_length = 40.0
    chassis_width = 18.0
    chassis_height = 5.0
    
    cabin_length = 20.0
    cabin_width = 16.0
    cabin_height = 10.0
    cabin_offset_x = 5.0 # Shift cabin back a bit
    
    wheel_radius = 4.0
    wheel_thickness = 2.0
    wheelbase = 26.0
    track_width = 16.0 # Center to center of wheels? Or just outside chassis?
    
    # 1. Create Chassis
    # Centered at (0,0, chassis_height/2 + ground_clearance) roughly
    # Let's put bottom of chassis at Z=wheel_radius 
    chassis_z = wheel_radius
    
    chassis_box = Part.makeBox(chassis_length, chassis_width, chassis_height)
    # Center the chassis on X and Y, sit on wheels in Z
    chassis_box.translate(FreeCAD.Vector(-chassis_length/2, -chassis_width/2, chassis_z))
    
    # 2. Create Cabin
    cabin_box = Part.makeBox(cabin_length, cabin_width, cabin_height)
    # Center cabin on width, position on top of chassis
    cabin_box.translate(FreeCAD.Vector(-cabin_length/2 + cabin_offset_x, -cabin_width/2, chassis_z + chassis_height))
    
    # Fuse Chassis and Cabin
    body_shape = chassis_box.fuse(cabin_box)
    
    # Optional: Fillet the body for style
    try:
        # Select edges greater than some length to fillet main body edges
        # This is tricky without naming, so let's just leave it blocky or do a simple chamfer if easy
        pass
    except:
        pass

    # 3. Create Wheels
    wheels = []
    wheel_x_offsets = [wheelbase/2, -wheelbase/2]
    wheel_y_offsets = [track_width/2, -track_width/2]
    
    for x_off in wheel_x_offsets:
        for y_off in wheel_y_offsets:
            # Create cylinder
            # Cylinder is created height along Z by default. We need it along Y.
            wheel = Part.makeCylinder(wheel_radius, wheel_thickness)
            
            # Rotate to align axis with Y
            # default axis is Z (0,0,1). We want Y (0,1,0). Rotate -90 deg around X
            rot = FreeCAD.Rotation(FreeCAD.Vector(1,0,0), -90)
            wheel.rotate(FreeCAD.Vector(0,0,0), rot, FreeCAD.Vector(0,0,0))
            
            # Position
            # Y offset needs to account for thickness to center it or place it on side
            y_pos = y_off
            if y_off > 0:
                y_pos = (chassis_width/2) 
            else:
                y_pos = -(chassis_width/2) - wheel_thickness
                
            wheel.translate(FreeCAD.Vector(x_off, y_pos, wheel_radius))
            wheels.append(wheel)

    # 4. Cut Wheel Arches from Body (Optional, but looks better)
    # We'll make slightly larger cylinders to cut
    cut_radius = wheel_radius + 1.0
    cut_thickness = wheel_thickness + 2.0
    
    for x_off in wheel_x_offsets:
        for y_off in wheel_y_offsets:
             # Cut shape
            cutter = Part.makeCylinder(cut_radius, cut_thickness)
            rot = FreeCAD.Rotation(FreeCAD.Vector(1,0,0), -90)
            cutter.rotate(FreeCAD.Vector(0,0,0), rot, FreeCAD.Vector(0,0,0))
            
            y_pos = 0
            if y_off > 0:
                y_pos = (chassis_width/2) - 1.0 # Indent slightly
            else:
                y_pos = -(chassis_width/2) - cut_thickness + 1.0
                
            cutter.translate(FreeCAD.Vector(x_off, y_pos, wheel_radius))
            body_shape = body_shape.cut(cutter)

    # Add Body to Doc
    Part.show(body_shape, "CarBody")
    
    # Add Wheels to Doc
    for i, w in enumerate(wheels):
        Part.show(w, f"Wheel_{i}")

    # Recompute
    doc.recompute()
    
    # View setup
    Gui = FreeCAD.Gui
    if Gui:
        Gui.SendMsgToActiveView("ViewFit")
        Gui.activeDocument().activeView().viewAxonometric()

if __name__ == "__main__":
    create_car()
