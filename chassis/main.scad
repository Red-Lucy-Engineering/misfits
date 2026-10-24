// misfits 3D printable chassis
// part = "assembly" | "body" | "caseback" | "pusher"

part = "assembly";
$fn  = 48;


// modules for placeholders
module display_placeholder(display_loc, display_dim, fpc_loc, fpc_dim) {
    translate(display_loc) cube(display_dim);
    //translate(fpc_loc) cube(fpc_dim);
}

module battery_placeholder(battery_loc, battery_dim) {
    translate(battery_loc) cube(battery_dim);
}

// measurements
display_loc = [-18.5,-15.75,4.9];
display_dim = [37.32, 31.8, 1];
fpc_loc = [-35,-13.9/2,display_loc[2]];
fpc_dim = [20, 13.9, 1];


// render placeholders as transparent
if (part == "assembly") {
    %import("/home/lucy/projects/misfits/misfits-rev3.stl");
    %display_placeholder(display_loc, display_dim, fpc_loc, fpc_dim);
}

// cassis parameters
// PCB
pcb_hx   = 21.355;     // half length of PCB, x axis
usb_w    = 9.2;        // USB-C cutout width
usb_h    = 3.9;        // USB-C cutout height
usb_z    = 3.25;       // USB-C receptacle middle
fun_w    = 13;
fun_h    = 7.4;
fun_depth= 1.2;

// battery lies on top of the inductor
inductor_bottom = -2.2;
batt_t          = 3.0;	// battery thickness

// walls
wall_side = 2.0;       // long side
wall_end  = 2.4;       // short side where the buttons are
outer_r   = 4.5;       // outer corner radius
bezel_t   = 1.6;       // front bezel thickness
caseback_t= 1.6;       // caseback thickness (bottom of chassis)

// carveout
cav_hx = 21.1;                 // half length
cav_hy = 16.0;                 // half width
cy     = -0.15;                // y-middle fo inner room

// display window GDEY0154D67
aa_dim     = [27.6, 27.6];
aa_off     = [0, 0];
win_margin = 0.5;
lens_size  = [31.0, 31.0]; 		// add an acrylic glass at the front
lens_t     = 0.75;             // thickness for acrylic glass, 0 = none

// buttons
btn_y   = [-9, 8.8];
btn_z   = 1.0;
act_x   = 20.3;                // outer surface buttons at PCB
flange_t= 1.15;
pocket_front = act_x + flange_t;
push_out = 1.2;                // how far the button pushes out

// wristband (standard wristwatch wristband)
strap_w  = 18;
lug_gap  = strap_w + 0.4;
lug_w    = 4.0;                // width per lug
bar_dx   = 3.4;                // distance to outer wall for wristband bar
bar_z    = -3.0;               // height
bar_hole = 1.2;
bar_hole_depth = 1.6;

// misc
outer_hx  = cav_hx + wall_end;
outer_hy  = cav_hy + wall_side;
z_bot     = inductor_bottom - batt_t - 0.3;
z_cav_top = display_loc[2] + display_dim[2] + 0.1;
z_top     = z_cav_top + bezel_t;
disp_c    = [display_loc[0] + display_dim[0]/2, display_loc[1] + display_dim[1]/2];
stem_len  = outer_hx + push_out - pocket_front + 0.01;

boss_pos = [for (sx=[-1,1], sy=[-1,1]) [sx*20.3, cy + sy*15.2]];


// module helpers
module rrect(sx, sy, r, h) {
    linear_extrude(h) offset(r) square([sx-2*r, sy-2*r], center=true);
}

module outer_shape() {
    ch = 0.8;
    hull() {
        translate([0, cy, z_bot])
            rrect(2*outer_hx, 2*outer_hy, outer_r, z_top - ch - z_bot);
        translate([0, cy, z_top - ch - 0.01])
            rrect(2*outer_hx - 2*ch, 2*outer_hy - 2*ch, outer_r - ch, ch + 0.01);
    }
}

module cavity() {
    translate([0, cy, z_bot - 0.01])
        rrect(2*cav_hx, 2*cav_hy, 1.0, z_cav_top - z_bot + 0.01);
}

module window_cut() {
    translate([disp_c[0] + aa_off[0], disp_c[1] + aa_off[1], z_cav_top - 0.1])
        rrect(aa_dim[0] + 2*win_margin, aa_dim[1] + 2*win_margin, 1.0, bezel_t + 0.5);
    if (lens_t > 0)
        translate([disp_c[0] + aa_off[0], disp_c[1] + aa_off[1], z_top - lens_t])
            rrect(lens_size[0] + 0.3, lens_size[1] + 0.3, 2.0, lens_t + 0.5);
}

module usb_cut() {
    yface = cy - cav_hy;
    
    hull() for (sx=[-1,1])
        translate([sx*(usb_w/2 - usb_h/2), yface + 0.01, usb_z])
            rotate([90,0,0]) cylinder(d=usb_h, h=wall_side + 1);

    hull() for (sx=[-1,1])
        translate([sx*(fun_w/2 - fun_h/2), cy - outer_hy - 0.01, usb_z])
            rotate([-90,0,0]) cylinder(d=fun_h, h=fun_depth + 0.01);
}

module pusher_cuts() {
    for (mx=[0,1]) mirror([mx,0,0]) for (yc=btn_y) translate([0, yc, btn_z]) {
        translate([cav_hx - 0.6, -2.5, -1.7])
            cube([pocket_front - (cav_hx - 0.6), 5.0, 3.4]);
        translate([pocket_front - 0.01, -1.25, -0.85])
            cube([outer_hx - pocket_front + 3, 2.5, 1.7]);
        translate([outer_hx, -1.7, -1.3])
            cube([2.05, 3.4, 10]);
    }
}

module lug_pos() {
    yin = lug_gap/2;
    difference() {
        hull() {
            translate([outer_hx - 1, yin, z_bot + 1]) cube([1.5, lug_w, 2.8 - (z_bot + 1)]);
            translate([outer_hx + bar_dx, yin, bar_z])
                rotate([-90,0,0]) cylinder(r=2.7, h=lug_w);
        }
        translate([outer_hx + bar_dx, yin - 0.01, bar_z])
            rotate([-90,0,0]) cylinder(d=bar_hole, h=bar_hole_depth);
    }
}

module lugs() {
    for (mx=[0,1]) mirror([mx,0,0])
        for (my=[0,1]) translate([0, cy, 0]) mirror([0,my,0]) lug_pos();
}

module bosses() {
    for (p=boss_pos) translate([p[0], p[1], z_bot]) cylinder(d=4, h=-z_bot - 2.5);
}

module boss_holes() {
    for (p=boss_pos) translate([p[0], p[1], z_bot - 0.01]) cylinder(d=1.7, h=8);
}


// parts
module body() {
    difference() {
        union() {
            difference() { outer_shape(); cavity(); }
            bosses();
            lugs();
        }
        boss_holes();
        window_cut();
        usb_cut();
        pusher_cuts();
    }
}

module caseback() {
    difference() {
        translate([0, cy, z_bot - caseback_t])
            rrect(2*outer_hx, 2*outer_hy, outer_r, caseback_t);
        for (p=boss_pos) {
            translate([p[0], p[1], z_bot - caseback_t - 0.01]) cylinder(d=2.3, h=caseback_t + 0.02);
            translate([p[0], p[1], z_bot - 1.0])               cylinder(d=4.2, h=1.1);
        }
    }
}

module pusher() {
    translate([0, -2.3, -1.5]) cube([flange_t, 4.6, 3.0]);
    translate([flange_t - 0.01, -1.1, -0.7]) cube([stem_len, 1.2, 0.7]);
}

module pushers_assembled() {
    for (mx=[0,1]) mirror([mx,0,0]) for (yc=btn_y)
        translate([act_x, yc, btn_z]) pusher();
}


// render output
if (part == "assembly") {
    color("dimgray") body();
    color("silver")  caseback();
    color("crimson") pushers_assembled();
} else if (part == "body") {
    // front side at the bottom
    translate([0, 0, z_top]) rotate([180,0,0]) body();
} else if (part == "caseback") {
    translate([0, 0, -(z_bot - caseback_t)]) caseback();
} else if (part == "pusher") {
    // 4 pieces, thicker side at the bottom
    for (i=[0:3]) translate([i*8, 0, 0]) rotate([0,-90,0]) pusher();
}
