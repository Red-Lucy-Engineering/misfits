module display_placeholder(display_loc, display_dim, fpc_loc, fpc_dim) {
    translate(display_loc) cube(display_dim);
    translate(fpc_loc) cube(fpc_dim);
}

module battery_placeholder(battery_loc, battery_dim) {
    translate(battery_loc) cube(battery_dim);
}

module housing_skel_prototype(display_loc, display_dim, fpc_loc, fpc_dim) {
    disp_margin = 1.25;
    bat_margin  = [2, 1.2];
    bat_clip_up = 5;
    z_trim      = 2;
    wire_corner = [-1, 1];
    wire_dir    = "none";
    zt         = 0.1;
    w          = 1.0;
    arm        = 3;
    pr         = 2.4;
    poff       = 0.6;
    top_t      = 0.8;
    ring_t     = 1.2;
    ring_inset = 2.0;
    border     = [1.2, 1.2, 1.2, 1.2];
    shelf_t    = 0.6;
    shelf_lip  = 1.2;
    scr_pilot = 1.7;  scr_hole = 2.2;  scr_head = 3.8;  scr_depth = 5;
    strap_w = 18;  strap_clear = 0.2;
    bridge_t = 1.6;  bridge_h = 4;
    lug_t = 2.5;  lug_r = 2;  lug_off = 2;
    bar_hole = 1.0;  bar_depth = 1.5;
    ring_offset = [0, -50, 0];

    bl = battery_loc;  bd = battery_dim;
    dm = disp_margin;  bm = bat_margin;
    pcb_z = [bl[2] + bd[2], display_loc[2] - shelf_t];

    pcb_tol = 0.6;
    pcb_r   = 3;
    pcb_dim = [40, 30];
    pcb_loc = [bl[0] + bd[0]/2 - pcb_dim[0]/2,
               display_loc[1] + display_dim[1]/2 - pcb_dim[1]/2];
    pf = [pcb_loc[0]-pcb_tol, pcb_loc[1]-pcb_tol,
          pcb_loc[0]+pcb_dim[0]+pcb_tol, pcb_loc[1]+pcb_dim[1]+pcb_tol];

    L_disp = [display_loc[0]-dm, display_loc[1]-dm, display_loc[2],
              display_loc[0]+display_dim[0]+dm, display_loc[1]+display_dim[1]+dm,
              display_loc[2]+display_dim[2]+zt];
    L_pcb  = [pf[0], pf[1], pcb_z[0], pf[2], pf[3], pcb_z[1]];
    L_bat  = [bl[0]-bm[0], bl[1]-bm[1], bl[2]-zt+z_trim,
              bl[0]+bd[0]+bm[0], bl[1]+bd[1]+bm[1], bl[2]+bd[2]];
    L_batc = [L_bat[0], L_bat[1], L_bat[2], L_bat[3], L_bat[4], L_bat[5] + bat_clip_up];
    layers      = [L_disp, L_bat];
    clip_layers = [L_disp, L_batc];

    bx0 = min([L_disp[0], L_pcb[0], L_bat[0]]);
    bx1 = max([L_disp[3], L_pcb[3], L_bat[3]]);
    by0 = min([L_disp[1], L_pcb[1], L_bat[1]]);
    by1 = max([L_disp[4], L_pcb[4], L_bat[4]]);
    zr1 = L_bat[2];
    zr0 = zr1 - ring_t;
    z_ceil = L_disp[5];
    z_top  = z_ceil + top_t;
    bar_z  = (zr0 + z_top) / 2;
    yc     = (by0 + by1) / 2;
    gap    = strap_w + strap_clear;
    cy0 = by0 - poff;  cy1 = by1 + poff;

    centers = [for (sx = [-1,1], sy = [-1,1])
        [(sx > 0 ? bx1 : bx0) + sx*poff, (sy > 0 ? by1 : by0) + sy*poff, sx, sy]];

    module box(a, b)
        translate([min(a[0],b[0]), min(a[1],b[1]), min(a[2],b[2])])
            cube([abs(a[0]-b[0]), abs(a[1]-b[1]), abs(a[2]-b[2])]);

    module pillar_slice(c, z0, z1)
        translate([c[0], c[1], z0]) cylinder(r=pr, h=z1-z0, $fn=40);

    module outline2d()
        hull() for (c = centers) translate([c[0], c[1]]) circle(r=pr, $fn=40);

    module rounded_rect2d(p, s, r=1)
        translate(p) offset(r=r) offset(delta=-r) square(s);

    module corner_clips() {
        for (c = centers) {
            sx = c[2];  sy = c[3];
            pillar_slice(c, zr1, z_top);
            for (L = clip_layers) {
                px = sx > 0 ? L[3] : L[0];
                py = sy > 0 ? L[4] : L[1];
                is_wire = (L == L_batc) && (sx == wire_corner[0]) && (sy == wire_corner[1]);
                if (!(is_wire && wire_dir == "y"))
                    hull() {
                        pillar_slice(c, L[2], L[5]);
                        box([px - sx*arm, py, L[2]], [px + sx*w, py + sy*w, L[5]]);
                    }
                if (!(is_wire && wire_dir == "x"))
                    hull() {
                        pillar_slice(c, L[2], L[5]);
                        box([px, py - sy*arm, L[2]], [px + sx*w, py + sy*w, L[5]]);
                    }
            }
            ppx = sx > 0 ? pf[2] : pf[0];
            ppy = sy > 0 ? pf[3] : pf[1];
            hull() {
                pillar_slice(c, L_bat[5], L_batc[5]);
                box([ppx - sx*arm, ppy - sy*arm, L_bat[5]],
                    [ppx + sx*w,   ppy + sy*w,   L_batc[5]]);
            }
            px = sx > 0 ? L_disp[3] : L_disp[0];
            py = sy > 0 ? L_disp[4] : L_disp[1];
            hull() {
                pillar_slice(c, L_disp[2] - shelf_t, L_disp[2]);
                box([px - sx*shelf_lip, py - sy*shelf_lip, L_disp[2] - shelf_t],
                    [px + sx*w, py + sy*w, L_disp[2]]);
            }
        }
    }

    module lug(side) {
        xb = side > 0 ? bx1 : bx0;
        xo = xb + side*bridge_t;
        for (s = [-1, 1]) {
            ly = yc + s * (gap/2 + lug_t/2);
            hull() {
                translate([xo + side*lug_off, ly, bar_z])
                    rotate([90,0,0]) cylinder(r=lug_r, h=lug_t, center=true, $fn=40);
                box([xb, ly - lug_t/2, bar_z - lug_r], [xo, ly + lug_t/2, bar_z + lug_r]);
            }
        }
    }

    module lug_holes(side) {
        xo = (side > 0 ? bx1 : bx0) + side*bridge_t;
        for (s = [-1, 1])
            translate([xo + side*lug_off, yc + s*(gap/2 - 0.01), bar_z])
                rotate([-s*90,0,0]) cylinder(d=bar_hole, h=bar_depth, $fn=20);
    }

    union() {
        difference() {
            corner_clips();
            for (L = layers) box([L[0],L[1],L[2]], [L[3],L[4],L[5]]);
            translate([0, 0, pcb_z[0]]) linear_extrude(pcb_z[1] - pcb_z[0])
                rounded_rect2d([pf[0], pf[1]], [pf[2]-pf[0], pf[3]-pf[1]], pcb_r);
            for (c = centers)
                translate([c[0], c[1], zr1 - 1]) cylinder(d=scr_pilot, h=scr_depth+1, $fn=24);
        }

        translate([0, 0, z_ceil]) linear_extrude(top_t)
            difference() {
                outline2d();
                rounded_rect2d([display_loc[0] + border[0], display_loc[1] + border[2]],
                               [display_dim[0] - border[0] - border[1],
                                display_dim[1] - border[2] - border[3]]);
            }

        difference() {
            union() {
                for (side = [-1, 1]) {
                    xb = side > 0 ? bx1 : bx0;
                    box([xb, cy0, bar_z - bridge_h/2], [xb + side*bridge_t, cy1, bar_z + bridge_h/2]);
                    lug(side);
                }
            }
            lug_holes(-1);
            lug_holes(+1);
        }
    }
}

display_loc = [-17.3,-15.75,5.5];
display_dim = [37.32, 31.8, 1];
fpc_loc = [-35,-13.9/2,display_loc[2]];
fpc_dim = [20, 13.9, 1];
battery_loc = [-20,-12.5,-10.3];
battery_dim = [40, 25, 8];

%import("/home/lucy/projects/misfits/misfits-rev3.stl");
%display_placeholder(display_loc, display_dim, fpc_loc, fpc_dim);
%battery_placeholder(battery_loc, battery_dim);

housing_skel_prototype(display_loc, display_dim, fpc_loc, fpc_dim);