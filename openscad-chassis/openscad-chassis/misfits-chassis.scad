%import("/home/lucy/projects/misfits/misfits-rev3.stl");


display_thickness = 0.3;
battery_thickness = 8;

pcb_comp_height = 4.906;
pcb_comp_width = 41;
pcb_comp_length = 30;

pcb_comp_underneath = -5.885;

//translate([-20.5, -15, pcb_comp_underneath]) cube([pcb_comp_width, pcb_comp_length, 1]);

// battery placeholder
translate([-20.5, -15, pcb_comp_underneath*2-1.115]) cube([pcb_comp_width, pcb_comp_length, battery_thickness]);

// display placeholder
translate([-20.5, -15, pcb_comp_height]) cube([pcb_comp_width, pcb_comp_length, display_thickness]);