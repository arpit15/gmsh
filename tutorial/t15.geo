/*********************************************************************
 *
 *  Gmsh tutorial 15
 *
 *  Embedded points, lines and surfaces
 *
 *********************************************************************/

// We start one again by including the first tutorial:
Include "t1.geo";

// We define a new point
Point(5) = {0.02, 0.02, 0, lc};

// One can force this point to be included ("embedded") in the 2D mesh, using
// the "Point In Surface" command:
Point{5} In Surface{6};

// In the same way, one can force a curve to be embedded in the 2D mesh using
// the "Line in Surface" command:
Point(6) = {0.02, 0.12, 0, lc};
Point(7) = {0.04, 0.18, 0, lc};
Line(5) = {6, 7};
Line{5} In Surface{6};

// One can also embed a surface in a volume using the "Surface In Volume"
// command:
Extrude {0, 0, 0.1}{ Surface {6}; }
p = newp;
Point(p) = {0.02, 0.12, 0.05, lc};
Point(p+1) = {0.04, 0.12, 0.05, lc};
Point(p+2) = {0.04, 0.18, 0.05, lc};
Point(p+3) = {0.02, 0.18, 0.05, lc};
l = newl;
Line(l) = {p, p+1};
Line(l+1) = {p+1, p+2};
Line(l+2) = {p+2, p+3};
Line(l+3) = {p+3, p};
ll = newll;
Line Loop(ll) = {l:l+3};
s = news;
Plane Surface(s) = {ll};
Surface{s} In Volume{1};

// Finally, one can also embed a point or a line in a volume using the
// "Line/Point In Volume" command:

p = newp;

Point(p) = {0.025, 0.15, 0.025, lc};
l = newl;
Line(l) = {7, 22};
Line{l} In Volume {1};

Point(p+1) = {0.07, 0.15, 0.025, lc/3};
Point{p+1} In Volume {1};
