#pragma once
// B3: Shared utility functions to avoid duplication across plugins
#include <cmath>

namespace Asmrtop {

// Hermite 4-point interpolation for sample-rate conversion
inline float hermite_interp(float v0, float v1, float v2, float v3, float f) {
    float c0 = v1;
    float c1 = 0.5f * (v2 - v0);
    float c2 = v0 - 2.5f * v1 + 2.0f * v2 - 0.5f * v3;
    float c3 = 0.5f * (v3 - v0) + 1.5f * (v1 - v2);
    return ((c3 * f + c2) * f + c1) * f + c0;
}

} // namespace Asmrtop
