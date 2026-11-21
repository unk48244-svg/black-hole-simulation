#include "Rprm.h"
//aaa l fsd
const double G = 6.67430e-20;
const double c = 300000; // 300 000 km/sec

struct phys_body {
    double mass, radius, SpinParameter; // kg, km, km
    Vec3 vel, angulMom, pos; // km/sec, kg*km, pos lol
    Quat rot;
    Mat3 inertia;
};
