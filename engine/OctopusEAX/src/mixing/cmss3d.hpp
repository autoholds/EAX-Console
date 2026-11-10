#pragma once

#include <cmath>
#include <algorithm>

namespace mix {

namespace cm {

struct Allpass {
    int n = 0, pos = 0; float g = 0.5f;
    float mem[1024];
    void prepare(int delay, float gain) { n = std::clamp(delay, 8, 1023); g = gain; pos = 0; std::fill(mem, mem + 1024, 0.f); }
    inline float process(float x) {
        float d = mem[pos];
        float y = d - g * x;
        mem[pos] = x + g * d;
        if (++pos >= n) pos = 0;
        return y;
    }
};

struct Decorrelator {
    Allpass a[3], b[3];
    void prepare(int fs) {
        const float k = fs / 48000.0f;
        const int da[3] = { 142, 107, 379 }, db[3] = { 167, 113, 421 };
        for (int i = 0; i < 3; ++i) { a[i].prepare((int)(da[i] * k), 0.52f); b[i].prepare((int)(db[i] * k), 0.52f); }
    }
    inline void process(float x, float& outA, float& outB) {
        float p = x, q = x;
        for (int i = 0; i < 3; ++i) { p = a[i].process(p); q = b[i].process(q); }
        outA = p; outB = q;
    }
};

struct OnePole {
    float a = 0.f, z = 0.f;
    void set(float hz, int fs) { a = expf(-2.0f * 3.14159265f * hz / fs); }
    inline float lp(float x) { z += (1.0f - a) * (x - z); return z; }
    inline float hp(float x) { return x - lp(x); }
};

}

struct CMSS3D {
    static const int ITD = 12, AMB = 640;

    float dl[ITD] = {0}; int pos = 0;
    float dS[AMB] = {0}; int posS = 0; int ambLen = 624;
    cm::OnePole lpX, lpS3, lpAmb3, hpDec3;

    cm::OnePole lpBass, hpDec2;
    cm::Decorrelator dec3, dec2;

    float sOn = 0.f, sMix = 0.f, sWidth = 0.f, sDepth = 1.f, rate = 0.001f;

    void prepare(int fs) {
        lpX.set(1400.f, fs); lpS3.set(200.f, fs); lpAmb3.set(5200.f, fs); hpDec3.set(260.f, fs);
        lpBass.set(170.f, fs); hpDec2.set(300.f, fs);
        dec3.prepare(fs); dec2.prepare(fs);
        ambLen = std::clamp((int)(0.013f * fs), 8, AMB - 1);
        std::fill(dl, dl + ITD, 0.f); std::fill(dS, dS + AMB, 0.f); pos = posS = 0;
        lpX.z = lpS3.z = lpAmb3.z = hpDec3.z = lpBass.z = hpDec2.z = 0.f;
        rate = 1.0f - expf(-1.0f / (0.015f * fs));
    }

    inline void process(float& l, float& r, bool on, int mode, float width, float depth) {
        sOn    += rate * ((on ? 1.f : 0.f) - sOn);
        if (sOn < 0.0005f && !on) { sOn = 0.f; return; }
        sMix   += rate * ((mode == 1 ? 1.f : 0.f) - sMix);
        sWidth += rate * (width - sWidth);
        sDepth += rate * (depth - sDepth);

        const float m = 0.5f * (l + r), s = 0.5f * (l - r);
        float o3l = l, o3r = r, o2l = l, o2r = r;

        if (sMix < 0.999f) {
            const float md = dl[pos]; dl[pos] = m; if (++pos >= ITD) pos = 0;
            const float x = 0.35f * lpX.lp(md) * sDepth;
            const float sh = s - lpS3.lp(s);
            float sd = dS[posS]; dS[posS] = s;
            if (++posS >= AMB) posS = 0;
            const int rd = (posS + AMB - ambLen) % AMB;
            sd = lpAmb3.lp(dS[rd]);
            float da, db; dec3.process(hpDec3.hp(m), da, db);
            const float amb = 0.30f * sd + 0.22f * 0.5f * (da - db);
            const float sw = s + 0.8f * sDepth * sh + sDepth * amb;
            const float G = 1.0f / (1.0f + 0.08f * sDepth);
            o3l = G * (m + x + sw);
            o3r = G * (m + x - sw);
        }
        if (sMix > 0.001f) {
            const float w = sWidth;
            float side;
            if (w >= 0.f) {
                const float lo = lpBass.lp(s), hi = s - lo;
                float da, db; dec2.process(hpDec2.hp(m), da, db);
                side = lo + hi * (1.0f + 1.25f * w) + 0.45f * w * 0.5f * (da - db);
                const float comp = 1.0f / (1.0f + 0.12f * w);
                o2l = m + comp * side; o2r = m - comp * side;
            } else {
                lpBass.lp(s); { float da, db; dec2.process(hpDec2.hp(m), da, db); }
                side = s * (1.0f + w);
                o2l = m + side; o2r = m - side;
            }
        }
        const float wl = o3l + sMix * (o2l - o3l), wr = o3r + sMix * (o2r - o3r);
        l += sOn * (wl - l); r += sOn * (wr - r);
    }
};

}
