#include "protocol.h"

// ============================================================

uint16_t convert_to_capacitance(float pf_val) {
    float val = pf_val * 100.0f;
    if (val < 0.0f) val = 0.0f;
    if (val > 65535.0f) val = 65535.0f;
    return (uint16_t)val;
}
