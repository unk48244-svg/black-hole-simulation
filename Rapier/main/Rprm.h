#pragma once
#include <algorithm>
#include <cmath>

inline constexpr double PI = 3.14159265358979323846;
inline double radians(double deg) { return deg * PI / 180.0; }
inline double degrees(double rad) { return rad * 180.0 / PI; }
inline double clamp(double v, double lo, double hi) { return std::max(lo, std::min(hi, v)); }
inline double lerp(double a, double b, double t) { return a + (b - a) * t; }

// ---------------------------------------------------------------- Vec2
struct Vec2 {
    double x = 0, y = 0;
    Vec2() = default;
    Vec2(double x, double y) : x(x), y(y) {}
    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(double s) const { return {x * s, y * s}; }
    Vec2 operator/(double s) const { return {x / s, y / s}; }
};
inline double dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline double length(Vec2 v) { return std::sqrt(dot(v, v)); }

// ---------------------------------------------------------------- Vec3
struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
    Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
    Vec3& operator+=(Vec3 o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(Vec3 o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(double s) { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(double s) { x /= s; y /= s; z /= s; return *this; }
};
inline Vec3 operator*(double s, Vec3 v) { return v * s; }
inline double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double lengthSq(Vec3 v) { return dot(v, v); }
inline double length(Vec3 v) { return std::sqrt(dot(v, v)); }
inline double distance(Vec3 a, Vec3 b) { return length(a - b); }
inline Vec3 normalize(Vec3 v) { double l = length(v); return l > 0 ? v / l : Vec3(); }
inline Vec3 lerp(Vec3 a, Vec3 b, double t) { return a + (b - a) * t; }
inline Vec3 reflect(Vec3 v, Vec3 n) { return v - n * (2 * dot(v, n)); }  // n must have length 1
// Compare with a small error allowed (== is not safe for double)
inline bool nearlyEqual(Vec3 a, Vec3 b, double eps = 1e-9) { return length(a - b) <= eps; }

// ---------------------------------------------------------------- Vec4
struct Vec4 {
    double x = 0, y = 0, z = 0, w = 0;
    Vec4() = default;
    Vec4(double x, double y, double z, double w) : x(x), y(y), z(z), w(w) {}
    Vec4(Vec3 v, double w) : x(v.x), y(v.y), z(v.z), w(w) {}
    Vec3 xyz() const { return {x, y, z}; }
};

// ---------------------------------------------------------------- Mat3
// Stored by columns: m[c*3 + r]. Used for inertia and rotation.
struct Mat3 {
    double m[9] = {1, 0, 0,  0, 1, 0,  0, 0, 1};

    double& at(int r, int c) { return m[c * 3 + r]; }
    double at(int r, int c) const { return m[c * 3 + r]; }

    static Mat3 diagonal(Vec3 d) {   // example, sphere inertia: diagonal(Vec3(1,1,1) * 0.4*m*r*r)
        Mat3 o;
        o.at(0, 0) = d.x; o.at(1, 1) = d.y; o.at(2, 2) = d.z;
        return o;
    }

    Mat3 operator*(const Mat3& b) const {
        Mat3 o;
        for (int c = 0; c < 3; ++c)
            for (int r = 0; r < 3; ++r) {
                double s = 0;
                for (int k = 0; k < 3; ++k) s += at(r, k) * b.at(k, c);
                o.at(r, c) = s;
            }
        return o;
    }
    Vec3 operator*(Vec3 v) const {
        return {at(0, 0) * v.x + at(0, 1) * v.y + at(0, 2) * v.z,
                at(1, 0) * v.x + at(1, 1) * v.y + at(1, 2) * v.z,
                at(2, 0) * v.x + at(2, 1) * v.y + at(2, 2) * v.z};
    }
    Mat3 transposed() const {
        Mat3 o;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c) o.at(r, c) = at(c, r);
        return o;
    }
    double determinant() const {
        return at(0, 0) * (at(1, 1) * at(2, 2) - at(1, 2) * at(2, 1))
             - at(0, 1) * (at(1, 0) * at(2, 2) - at(1, 2) * at(2, 0))
             + at(0, 2) * (at(1, 0) * at(2, 1) - at(1, 1) * at(2, 0));
    }
    Mat3 inversed() const {   // if it cannot be inverted, returns identity
        double det = determinant();
        if (std::fabs(det) < 1e-300) return Mat3();
        double id = 1.0 / det;
        Mat3 o;
        o.at(0, 0) =  (at(1, 1) * at(2, 2) - at(1, 2) * at(2, 1)) * id;
        o.at(0, 1) = -(at(0, 1) * at(2, 2) - at(0, 2) * at(2, 1)) * id;
        o.at(0, 2) =  (at(0, 1) * at(1, 2) - at(0, 2) * at(1, 1)) * id;
        o.at(1, 0) = -(at(1, 0) * at(2, 2) - at(1, 2) * at(2, 0)) * id;
        o.at(1, 1) =  (at(0, 0) * at(2, 2) - at(0, 2) * at(2, 0)) * id;
        o.at(1, 2) = -(at(0, 0) * at(1, 2) - at(0, 2) * at(1, 0)) * id;
        o.at(2, 0) =  (at(1, 0) * at(2, 1) - at(1, 1) * at(2, 0)) * id;
        o.at(2, 1) = -(at(0, 0) * at(2, 1) - at(0, 1) * at(2, 0)) * id;
        o.at(2, 2) =  (at(0, 0) * at(1, 1) - at(0, 1) * at(1, 0)) * id;
        return o;
    }
};

// ---------------------------------------------------------------- Mat4
// Stored by columns (like OpenGL): row r, column c = m[c*4 + r].
struct Mat4 {
    double m[16] = {1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1};

    double& at(int r, int c) { return m[c * 4 + r]; }
    double at(int r, int c) const { return m[c * 4 + r]; }

    // A * B means: do B first, then A.
    Mat4 operator*(const Mat4& b) const {
        Mat4 o;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r) {
                double s = 0;
                for (int k = 0; k < 4; ++k) s += at(r, k) * b.at(k, c);
                o.at(r, c) = s;
            }
        return o;
    }
    Vec4 operator*(Vec4 v) const {
        return {at(0, 0) * v.x + at(0, 1) * v.y + at(0, 2) * v.z + at(0, 3) * v.w,
                at(1, 0) * v.x + at(1, 1) * v.y + at(1, 2) * v.z + at(1, 3) * v.w,
                at(2, 0) * v.x + at(2, 1) * v.y + at(2, 2) * v.z + at(2, 3) * v.w,
                at(3, 0) * v.x + at(3, 1) * v.y + at(3, 2) * v.z + at(3, 3) * v.w};
    }
    Vec3 point(Vec3 p) const {   // point: uses move, divides by w (for projection)
        Vec4 r = *this * Vec4(p, 1);
        return (r.w != 0 && r.w != 1) ? r.xyz() / r.w : r.xyz();
    }
    Vec3 dir(Vec3 d) const { return (*this * Vec4(d, 0)).xyz(); }   // direction: no move

    Mat4 transposed() const {
        Mat4 o;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c) o.at(r, c) = at(c, r);
        return o;
    }

    // Inverse matrix. If it cannot be inverted, returns identity.
    Mat4 inversed() const {
        const double* a = m;
        double s0 = a[0] * a[5] - a[4] * a[1],   s1 = a[0] * a[9] - a[8] * a[1];
        double s2 = a[0] * a[13] - a[12] * a[1], s3 = a[4] * a[9] - a[8] * a[5];
        double s4 = a[4] * a[13] - a[12] * a[5], s5 = a[8] * a[13] - a[12] * a[9];
        double c5 = a[10] * a[15] - a[14] * a[11], c4 = a[6] * a[15] - a[14] * a[7];
        double c3 = a[6] * a[11] - a[10] * a[7],   c2 = a[2] * a[15] - a[14] * a[3];
        double c1 = a[2] * a[11] - a[10] * a[3],   c0 = a[2] * a[7] - a[6] * a[3];
        double det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
        if (std::fabs(det) < 1e-300) return Mat4();
        double id = 1.0 / det;
        Mat4 o;
        double* b = o.m;
        b[0]  = ( a[5] * c5 - a[9] * c4 + a[13] * c3) * id;
        b[1]  = (-a[1] * c5 + a[9] * c2 - a[13] * c1) * id;
        b[2]  = ( a[1] * c4 - a[5] * c2 + a[13] * c0) * id;
        b[3]  = (-a[1] * c3 + a[5] * c1 - a[9]  * c0) * id;
        b[4]  = (-a[4] * c5 + a[8] * c4 - a[12] * c3) * id;
        b[5]  = ( a[0] * c5 - a[8] * c2 + a[12] * c1) * id;
        b[6]  = (-a[0] * c4 + a[4] * c2 - a[12] * c0) * id;
        b[7]  = ( a[0] * c3 - a[4] * c1 + a[8]  * c0) * id;
        b[8]  = ( a[7] * s5 - a[11] * s4 + a[15] * s3) * id;
        b[9]  = (-a[3] * s5 + a[11] * s2 - a[15] * s1) * id;
        b[10] = ( a[3] * s4 - a[7]  * s2 + a[15] * s0) * id;
        b[11] = (-a[3] * s3 + a[7]  * s1 - a[11] * s0) * id;
        b[12] = (-a[6] * s5 + a[10] * s4 - a[14] * s3) * id;
        b[13] = ( a[2] * s5 - a[10] * s2 + a[14] * s1) * id;
        b[14] = (-a[2] * s4 + a[6]  * s2 - a[14] * s0) * id;
        b[15] = ( a[2] * s3 - a[6]  * s1 + a[10] * s0) * id;
        return o;
    }

    // OpenGL wants float, so convert before glUniformMatrix4fv:
    //   float f[16]; mvp.toFloat(f); glUniformMatrix4fv(loc, 1, GL_FALSE, f);
    void toFloat(float out[16]) const {
        for (int i = 0; i < 16; ++i) out[i] = (float)m[i];
    }
};

inline Mat4 translate(Vec3 t) {
    Mat4 o;
    o.at(0, 3) = t.x; o.at(1, 3) = t.y; o.at(2, 3) = t.z;
    return o;
}
inline Mat4 scale(Vec3 s) {
    Mat4 o;
    o.at(0, 0) = s.x; o.at(1, 1) = s.y; o.at(2, 2) = s.z;
    return o;
}
inline Mat4 scale(double s) { return scale(Vec3(s, s, s)); }

// Rotate around an axis by angle (in radians)
inline Mat4 rotate(Vec3 axis, double angle) {
    Vec3 a = normalize(axis);
    double c = std::cos(angle), s = std::sin(angle), t = 1 - c;
    Mat4 o;
    o.at(0, 0) = t * a.x * a.x + c;       o.at(0, 1) = t * a.x * a.y - s * a.z; o.at(0, 2) = t * a.x * a.z + s * a.y;
    o.at(1, 0) = t * a.x * a.y + s * a.z; o.at(1, 1) = t * a.y * a.y + c;       o.at(1, 2) = t * a.y * a.z - s * a.x;
    o.at(2, 0) = t * a.x * a.z - s * a.y; o.at(2, 1) = t * a.y * a.z + s * a.x; o.at(2, 2) = t * a.z * a.z + c;
    return o;
}

// OpenGL perspective (depth -1..1). fovY is the vertical angle in radians.
inline Mat4 perspective(double fovY, double aspect, double nearZ, double farZ) {
    double f = 1.0 / std::tan(fovY / 2);
    Mat4 o;
    for (double& v : o.m) v = 0;
    o.at(0, 0) = f / aspect;
    o.at(1, 1) = f;
    o.at(2, 2) = (farZ + nearZ) / (nearZ - farZ);
    o.at(2, 3) = 2 * farZ * nearZ / (nearZ - farZ);
    o.at(3, 2) = -1;
    return o;
}

inline Mat4 ortho(double l, double r, double b, double t, double n, double f) {
    Mat4 o;
    o.at(0, 0) = 2 / (r - l);  o.at(0, 3) = -(r + l) / (r - l);
    o.at(1, 1) = 2 / (t - b);  o.at(1, 3) = -(t + b) / (t - b);
    o.at(2, 2) = -2 / (f - n); o.at(2, 3) = -(f + n) / (f - n);
    return o;
}

// Camera at eye looks at center (camera looks along -Z)
inline Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = normalize(center - eye);
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);
    Mat4 o;
    o.at(0, 0) =  s.x; o.at(0, 1) =  s.y; o.at(0, 2) =  s.z; o.at(0, 3) = -dot(s, eye);
    o.at(1, 0) =  u.x; o.at(1, 1) =  u.y; o.at(1, 2) =  u.z; o.at(1, 3) = -dot(u, eye);
    o.at(2, 0) = -f.x; o.at(2, 1) = -f.y; o.at(2, 2) = -f.z; o.at(2, 3) =  dot(f, eye);
    return o;
}

// ---------------------------------------------------------------- Quat
// Quaternion = a rotation. It has no gimbal lock and turns smoothly.
struct Quat {
    double x = 0, y = 0, z = 0, w = 1;
    Quat() = default;
    Quat(double x, double y, double z, double w) : x(x), y(y), z(z), w(w) {}

    static Quat fromAxisAngle(Vec3 axis, double angle) {
        Vec3 a = normalize(axis);
        double s = std::sin(angle / 2);
        return {a.x * s, a.y * s, a.z * s, std::cos(angle / 2)};
    }

    // a * b: rotate by b first, then by a
    Quat operator*(const Quat& b) const {
        return {w * b.x + x * b.w + y * b.z - z * b.y,
                w * b.y - x * b.z + y * b.w + z * b.x,
                w * b.z + x * b.y - y * b.x + z * b.w,
                w * b.w - x * b.x - y * b.y - z * b.z};
    }
    Quat conjugate() const { return {-x, -y, -z, w}; }   // opposite rotation (when length is 1)
    Quat normalized() const {
        double l = std::sqrt(x * x + y * y + z * z + w * w);
        return l > 0 ? Quat(x / l, y / l, z / l, w / l) : Quat();
    }

    Vec3 rotate(Vec3 v) const {
        Vec3 q(x, y, z);
        Vec3 t = cross(q, v) * 2;
        return v + t * w + cross(q, t);
    }

    Mat3 toMat3() const {
        double xx = x * x, yy = y * y, zz = z * z, xy = x * y, xz = x * z, yz = y * z;
        double wx = w * x, wy = w * y, wz = w * z;
        Mat3 o;
        o.at(0, 0) = 1 - 2 * (yy + zz); o.at(0, 1) = 2 * (xy - wz);     o.at(0, 2) = 2 * (xz + wy);
        o.at(1, 0) = 2 * (xy + wz);     o.at(1, 1) = 1 - 2 * (xx + zz); o.at(1, 2) = 2 * (yz - wx);
        o.at(2, 0) = 2 * (xz - wy);     o.at(2, 1) = 2 * (yz + wx);     o.at(2, 2) = 1 - 2 * (xx + yy);
        return o;
    }
    Mat4 toMat4() const {
        Mat3 r = toMat3();
        Mat4 o;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) o.at(i, j) = r.at(i, j);
        return o;
    }

    // New rotation after spinning with speed omega (rad/s, world axes) for time dt.
    // Use a small dt. The result is fixed to length 1.
    Quat integrate(Vec3 omega, double dt) const {
        Quat qDot = Quat(omega.x, omega.y, omega.z, 0) * (*this);
        return Quat(x + 0.5 * qDot.x * dt, y + 0.5 * qDot.y * dt,
                    z + 0.5 * qDot.z * dt, w + 0.5 * qDot.w * dt).normalized();
    }
};

// Smooth move from rotation a to rotation b, t from 0 to 1
inline Quat slerp(Quat a, Quat b, double t) {
    double d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (d < 0) { b = Quat(-b.x, -b.y, -b.z, -b.w); d = -d; }   // take the short way
    if (d > 0.9995)   // almost the same - simple blend is enough
        return Quat(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t), lerp(a.w, b.w, t)).normalized();
    double th = std::acos(d), s = std::sin(th);
    double wa = std::sin((1 - t) * th) / s, wb = std::sin(t * th) / s;
    return Quat(a.x * wa + b.x * wb, a.y * wa + b.y * wb, a.z * wa + b.z * wb, a.w * wa + b.w * wb).normalized();
}
