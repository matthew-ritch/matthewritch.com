// spectral.cpp — Component-based audio-reactive filter engine
// Three base filters: f1 (temporal 2nd diff), f2 (spatial Y), f3 (spatial X)
// Combs combine these with modes: + - × ÷ exp log
// Single process_combs() call per animation frame.

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <emscripten.h>

// ── globals ──────────────────────────────────────────────────────────────────
static int gN = 0, gW = 0, gH = 0;
static int gFrameIdx = 0;

static float* vid   = nullptr;        // N*H*W*3  frame buffer (0-1 range)
static float* u_t   = nullptr;        // N*H*W*3  velocity field
static float* f1buf = nullptr;        // N*H*W*3  temporal component
static float* f2buf = nullptr;        // N*H*W*3  spatial-Y component
static float* f3buf = nullptr;        // N*H*W*3  spatial-X component
static float* delta = nullptr;        // N*H*W*3  accumulated comb delta
static float* comb_cfg = nullptr;     // MAX_COMBS*3 floats [comp, mode, coeff]
static uint8_t* in_buf  = nullptr;    // H*W*4    camera input (RGBA)
static uint8_t* out_buf = nullptr;    // H*W*4    display output (RGBA)

static const int MAX_COMBS = 32;

static inline int sz()      { return gN * gH * gW * 3; }
static inline int rgba_sz() { return gH * gW * 4; }

// ── memory management ─────────────────────────────────────────────────────────
static void free_all() {
    free(vid);      vid      = nullptr;
    free(u_t);      u_t      = nullptr;
    free(f1buf);    f1buf    = nullptr;
    free(f2buf);    f2buf    = nullptr;
    free(f3buf);    f3buf    = nullptr;
    free(delta);    delta    = nullptr;
    free(comb_cfg); comb_cfg = nullptr;
    free(in_buf);   in_buf   = nullptr;
    free(out_buf);  out_buf  = nullptr;
}

// ── convolution kernels [1, -2, 1] ───────────────────────────────────────────
// f1: temporal — along frame axis with wrap
static void conv_temporal(const float* src, float* dst) {
    for (int f = 0; f < gN; ++f) {
        int fp = (f - 1 + gN) % gN;
        int fn = (f + 1) % gN;
        for (int y = 0; y < gH; ++y)
        for (int x = 0; x < gW; ++x)
        for (int c = 0; c < 3; ++c) {
            int i  = ((f  * gH + y) * gW + x) * 3 + c;
            int ip = ((fp * gH + y) * gW + x) * 3 + c;
            int in_= ((fn * gH + y) * gW + x) * 3 + c;
            dst[i] = src[ip] - 2.0f * src[i] + src[in_];
        }
    }
}

// f2: spatial Y — along height, clamp boundary
static void conv_spatial_y(const float* src, float* dst) {
    for (int f = 0; f < gN; ++f)
    for (int y = 0; y < gH; ++y) {
        int yp = (y > 0)      ? y - 1 : 0;
        int yn = (y < gH - 1) ? y + 1 : gH - 1;
        for (int x = 0; x < gW; ++x)
        for (int c = 0; c < 3; ++c) {
            int base = ((f * gH + y) * gW + x) * 3 + c;
            dst[base] = src[((f * gH + yp) * gW + x) * 3 + c]
                      - 2.0f * src[base]
                      + src[((f * gH + yn) * gW + x) * 3 + c];
        }
    }
}

// f3: spatial X — along width, clamp boundary
static void conv_spatial_x(const float* src, float* dst) {
    for (int f = 0; f < gN; ++f)
    for (int y = 0; y < gH; ++y)
    for (int x = 0; x < gW; ++x) {
        int xp = (x > 0)      ? x - 1 : 0;
        int xn = (x < gW - 1) ? x + 1 : gW - 1;
        for (int c = 0; c < 3; ++c) {
            int base = ((f * gH + y) * gW + x) * 3 + c;
            dst[base] = src[((f * gH + y) * gW + xp) * 3 + c]
                      - 2.0f * src[base]
                      + src[((f * gH + y) * gW + xn) * 3 + c];
        }
    }
}

// ── exported API ──────────────────────────────────────────────────────────────
extern "C" {

EMSCRIPTEN_KEEPALIVE
int init(int N, int W, int H) {
    free_all();
    gN = N; gW = W; gH = H;
    int total = sz();
    int rgba  = rgba_sz();

    vid      = (float*)   calloc(total, sizeof(float));
    u_t      = (float*)   calloc(total, sizeof(float));
    f1buf    = (float*)   malloc(total * sizeof(float));
    f2buf    = (float*)   malloc(total * sizeof(float));
    f3buf    = (float*)   malloc(total * sizeof(float));
    delta    = (float*)   calloc(total, sizeof(float));
    comb_cfg = (float*)   malloc(MAX_COMBS * 3 * sizeof(float));
    in_buf   = (uint8_t*) malloc(rgba);
    out_buf  = (uint8_t*) malloc(rgba);

    if (!vid || !u_t || !f1buf || !f2buf || !f3buf ||
        !delta || !comb_cfg || !in_buf || !out_buf) {
        free_all();
        return -1;
    }
    gFrameIdx = 0;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
void reset() {
    if (vid) memset(vid, 0, sz() * sizeof(float));
    if (u_t) memset(u_t, 0, sz() * sizeof(float));
    gFrameIdx = 0;
}

EMSCRIPTEN_KEEPALIVE uint8_t* get_input_ptr()  { return in_buf; }
EMSCRIPTEN_KEEPALIVE uint8_t* get_output_ptr() { return out_buf; }
EMSCRIPTEN_KEEPALIVE float*   get_comb_ptr()   { return comb_cfg; }
EMSCRIPTEN_KEEPALIVE int      get_frame_idx()  { return gFrameIdx; }
EMSCRIPTEN_KEEPALIVE int      get_width()      { return gW; }
EMSCRIPTEN_KEEPALIVE int      get_height()     { return gH; }

// Main per-frame call.
// comb_cfg layout: [comp0, mode0, coeff0, comp1, mode1, coeff1, ...]
//   comp: 0=f1(temporal), 1=f2(spatialY), 2=f3(spatialX)
//   mode: 0=+, 1=-, 2=×, 3=÷, 4=exp, 5=log
//   coeff: float (audio_power * gain * component_scale)
EMSCRIPTEN_KEEPALIVE
void process_combs(int n_combs, int n_reps) {
    if (!vid || n_combs < 0) return;
    if (n_combs > MAX_COMBS) n_combs = MAX_COMBS;

    int total = sz();
    int f = gFrameIdx;

    // 1. Clip vid to [0, 1]
    for (int i = 0; i < total; ++i) {
        if      (vid[i] < 0.0f) vid[i] = 0.0f;
        else if (vid[i] > 1.0f) vid[i] = 1.0f;
    }

    // 2. Copy current frame to output (display before overwrite)
    for (int y = 0; y < gH; ++y)
    for (int x = 0; x < gW; ++x) {
        int vi = ((f * gH + y) * gW + x) * 3;
        int oi = (y * gW + x) * 4;
        out_buf[oi + 0] = (uint8_t)(vid[vi + 0] * 255.0f);
        out_buf[oi + 1] = (uint8_t)(vid[vi + 1] * 255.0f);
        out_buf[oi + 2] = (uint8_t)(vid[vi + 2] * 255.0f);
        out_buf[oi + 3] = 255;
    }

    // 3. Ingest camera frame (1.5x brightness + horizontal flip)
    for (int y = 0; y < gH; ++y)
    for (int x = 0; x < gW; ++x) {
        int fx = gW - 1 - x;
        int ii = (y * gW + fx) * 4;
        int vi = ((f * gH + y) * gW + x) * 3;
        vid[vi + 0] = 1.5f * in_buf[ii + 0] / 255.0f;
        vid[vi + 1] = 1.5f * in_buf[ii + 1] / 255.0f;
        vid[vi + 2] = 1.5f * in_buf[ii + 2] / 255.0f;
    }

    // 4. Clear u_t for this frame slot
    int slot_off = f * gH * gW * 3;
    memset(u_t + slot_off, 0, gH * gW * 3 * sizeof(float));

    // 5. Determine which components are needed
    bool need[3] = {false, false, false};
    for (int c = 0; c < n_combs; ++c) {
        int ci = (int)comb_cfg[c * 3];
        if (ci >= 0 && ci < 3) need[ci] = true;
    }

    // 6. Compute needed components
    if (need[0]) conv_temporal (vid, f1buf);
    if (need[1]) conv_spatial_y(vid, f2buf);
    if (need[2]) conv_spatial_x(vid, f3buf);

    float* comp_ptrs[3] = { f1buf, f2buf, f3buf };

    // 7. Accumulate delta — additive modes first, then multiplicative
    memset(delta, 0, total * sizeof(float));

    // Pass 1: additive (+, -, exp, log)
    for (int c = 0; c < n_combs; ++c) {
        int   ci    = (int)comb_cfg[c * 3 + 0];
        int   mode  = (int)comb_cfg[c * 3 + 1];
        float coeff = comb_cfg[c * 3 + 2];

        if (ci < 0 || ci > 2 || fabsf(coeff) < 1e-9f) continue;
        const float* cv = comp_ptrs[ci];

        switch (mode) {
            case 0: // +
                for (int i = 0; i < total; ++i) delta[i] += coeff * cv[i];
                break;
            case 1: // -
                for (int i = 0; i < total; ++i) delta[i] -= coeff * cv[i];
                break;
            case 4: // exp
                for (int i = 0; i < total; ++i)
                    delta[i] += coeff * (expf(cv[i]) - 1.0f);
                break;
            case 5: // log
                for (int i = 0; i < total; ++i) {
                    float v = cv[i];
                    delta[i] += coeff * copysignf(log1pf(fabsf(v)), v);
                }
                break;
        }
    }

    // Pass 2: multiplicative (×, ÷)
    for (int c = 0; c < n_combs; ++c) {
        int   ci    = (int)comb_cfg[c * 3 + 0];
        int   mode  = (int)comb_cfg[c * 3 + 1];
        float coeff = comb_cfg[c * 3 + 2];

        if (ci < 0 || ci > 2 || fabsf(coeff) < 1e-9f) continue;
        const float* cv = comp_ptrs[ci];

        if (mode == 2) { // ×
            for (int i = 0; i < total; ++i)
                delta[i] *= (1.0f + coeff * cv[i]);
        } else if (mode == 3) { // ÷
            for (int i = 0; i < total; ++i) {
                float d = 1.0f + coeff * cv[i];
                if (fabsf(d) < 1e-6f) d = copysignf(1e-6f, d);
                delta[i] /= d;
            }
        }
    }

    // 8. PDE update: n_reps iterations
    for (int rep = 0; rep < n_reps; ++rep) {
        for (int i = 0; i < total; ++i) {
            u_t[i] += delta[i];
            vid[i] += u_t[i];
        }
    }

    // 9. Advance frame
    gFrameIdx = (gFrameIdx + 1) % gN;
}

} // extern "C"
