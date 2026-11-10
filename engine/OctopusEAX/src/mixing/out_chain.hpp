#pragma once
#include "../equalizer/graphic_equalizer.hpp"
#include "cmss3d.hpp"
#include "soft_clip.hpp"

namespace mix {

struct CmssParams { bool on = false; int mode = 0; float width = 0.f, depth = 1.f; };

struct OutChain {
    eqb::GraphicEQ eq;
    CMSS3D cmss;
    SoftClip clip;

    void prepare(int fs) {
        eq.prepare(fs);
        cmss.prepare(fs);
    }
    inline void process(float& l, float& r, bool eqOn, const CmssParams& c) {
        if (eqOn) eq.process(l, r);
        cmss.process(l, r, c.on, c.mode, c.width, c.depth);
        clip.process(l, r);
    }
};

}
