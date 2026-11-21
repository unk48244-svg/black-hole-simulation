#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

struct V3 { float x = 0, y = 0, z = 0; };
struct Box { V3 min, max; };

struct MovingRay {
    V3 pos;       
    V3 dir{0, 0, 1};  
    float speed = 0;        
    bool active = false;

    void Launch(V3 p, V3 d, float s) {
        float l = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        pos = p; dir = {d.x / l, d.y / l, d.z / l}; speed = s; active = true;
    }
    // move ray
    bool Update(float dt, const std::vector<Box>& boxes) {
        if (!active) return false;
        float step = speed * dt, best = step;
        bool hit = false;
        float o[3] = {pos.x, pos.y, pos.z}, d[3] = {dir.x, dir.y, dir.z};

        for (auto& b : boxes) {
            float mn[3] = {b.min.x, b.min.y, b.min.z}, mx[3] = {b.max.x, b.max.y, b.max.z};
            float t0 = 0, t1 = best;
            bool miss = false;
            for (int i = 0; i < 3 && !miss; i++) {
                if (std::fabs(d[i]) < 1e-8f) { miss = o[i] < mn[i] || o[i] > mx[i]; continue; }
                float a = (mn[i] - o[i]) / d[i], c = (mx[i] - o[i]) / d[i];
                t0 = std::max(t0, std::min(a, c));
                t1 = std::min(t1, std::max(a, c));
                miss = t0 > t1;
            }
            if (!miss && t0 <= best) { best = t0; hit = true; }
        }

        pos = {pos.x + dir.x * best, pos.y + dir.y * best, pos.z + dir.z * best};
        if (hit) active = false;
        return hit;
    }
};