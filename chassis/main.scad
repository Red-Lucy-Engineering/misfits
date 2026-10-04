// modules
module display_placeholder(display_loc, display_dim, fpc_loc, fpc_dim) {
    translate(display_loc) cube(display_dim);
    translate(fpc_loc) cube(fpc_dim);
}

module housing_skel(display_loc, display_dim, fpc_loc, fpc_dim) {
        difference() {
        translate([display_loc[0]-0.5,display_loc[1]-0.5,display_loc[2]-0.5]) {
            cube([display_dim[0]+1,display_dim[1]+1,display_dim[2]+1]);
        }
        translate(display_loc) cube(display_dim);
        translate([display_loc[0]+0.5,display_loc[1]+0.5,display_loc[2]+0.5]) {
            cube([display_dim[0]-1,display_dim[1]-1,display_dim[2]+5]);
        }
        translate(fpc_loc) cube(fpc_dim);
        translate(display_loc) cube(display_dim);
        translate(display_loc) {
            cube([display_dim[0]+20,display_dim[1],display_dim[2]+10]);
        }
    }
}

// dimensions
display_loc = [-17.3,-15.75,5.5];
display_dim = [37.32, 31.8, 1];
fpc_loc = [-35,-13.9/2,display_loc[2]];
fpc_dim = [20, 13.9, 1];

// placeholders
%import("/home/lucy/projects/misfits/misfits-rev3.stl");
%display_placeholder(display_loc, display_dim, fpc_loc, fpc_dim);

housing_skel(display_loc, display_dim, fpc_loc, fpc_dim);

