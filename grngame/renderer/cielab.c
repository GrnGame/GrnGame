#include "cielab.h"
#include "SDL3/SDL_pixels.h"
#include "grngame/core/app.h"
#include "grngame/utils/attributes.h"
#include "kvec.h"
#include <cglm/cglm.h>
#include <float.h>
#include <math.h>

#define POW25_7 6103515625.0f // 25^7

static float32 srgb_linear_lut[256];
static bool lut_ready = false;

void InitLinearLut(void)
{
    if (lut_ready)
        return;

    for (int32 i = 0; i < 256; i++)
    {
        float32 v = (float32)i / 255.0f;
        if (v <= 0.04045f)
            srgb_linear_lut[i] = v / 12.92f;
        else
            srgb_linear_lut[i] = powf((v + 0.055f) / 1.055f, 2.4f);
    }
    lut_ready = true;
}

static float32 PivotXYZ(float32 n)
{
    const float32 epsilon = 216.0f / 24389.0f;
    const float32 kappa = 24389.0f / 27.0f;

    if (n > epsilon)
        return cbrtf(n);

    return (kappa * n + 16.0f) / 116.0f;
}

static HOT ColorXYZ RgbToXyz(const SDL_Color *rgb)
{
    float32 r = srgb_linear_lut[rgb->r];
    float32 g = srgb_linear_lut[rgb->g];
    float32 b = srgb_linear_lut[rgb->b];

    ColorXYZ xyz;
    xyz.x = (r * 0.4124564f + g * 0.3575761f + b * 0.1804375f) * 100.0f;
    xyz.y = (r * 0.2126729f + g * 0.7151522f + b * 0.0721750f) * 100.0f;
    xyz.z = (r * 0.0193339f + g * 0.1191920f + b * 0.9503041f) * 100.0f;
    return xyz;
}

static ColorLAB XyzToLab(const ColorXYZ *xyz)
{
    const float32 ref_x = 95.047f;
    const float32 ref_y = 100.000f;
    const float32 ref_z = 108.883f;

    float32 x = PivotXYZ(xyz->x / ref_x);
    float32 y = PivotXYZ(xyz->y / ref_y);
    float32 z = PivotXYZ(xyz->z / ref_z);

    ColorLAB lab;
    lab.l = fmaxf(0.0f, 116.0f * y - 16.0f);
    lab.a = 500.0f * (x - y);
    lab.b = 200.0f * (y - z);
    return lab;
}

ColorLAB HOT RgbToLab(SDL_Color *color)
{
    ColorXYZ xyz = RgbToXyz(color);
    return XyzToLab(&xyz);
}

static float32 Pow7(float32 x)
{
    float32 x2 = x * x;
    return x2 * x2 * x2 * x;
}

static float32 HueDegrees(float32 b, float32 a_prime)
{
    if (b == 0.0f && a_prime == 0.0f)
        return 0.0f;

    float32 h = glm_deg(atan2f(b, a_prime));
    return (h < 0.0f) ? h + 360.0f : h;
}

HOT static float32 CalculateCIEDE2000(const ColorLAB *color1, const ColorLAB *color2)
{
    const float32 kL = 1.0f;
    const float32 kC = 1.0f;
    const float32 kH = 1.0f;

    float32 l1 = color1->l;
    float32 a1 = color1->a;
    float32 b1 = color1->b;

    float32 l2 = color2->l;
    float32 a2 = color2->a;
    float32 b2 = color2->b;

    float32 c1 = sqrtf(a1 * a1 + b1 * b1);
    float32 c2 = sqrtf(a2 * a2 + b2 * b2);
    float32 c_avg7 = Pow7((c1 + c2) / 2.0f);
    float32 g = 0.5f * (1.0f - sqrtf(c_avg7 / (c_avg7 + POW25_7)));

    float32 a1_prime = a1 * (1.0f + g);
    float32 a2_prime = a2 * (1.0f + g);

    float32 c1_prime = sqrtf(a1_prime * a1_prime + b1 * b1);
    float32 c2_prime = sqrtf(a2_prime * a2_prime + b2 * b2);

    float32 h1_prime = HueDegrees(b1, a1_prime);
    float32 h2_prime = HueDegrees(b2, a2_prime);

    float32 delta_L_prime = l2 - l1;
    float32 delta_C_prime = c2_prime - c1_prime;

    bool chromatic = (c1_prime * c2_prime) != 0.0f;

    float32 delta_h_prime = 0.0f;
    if (chromatic)
    {
        delta_h_prime = h2_prime - h1_prime;
        if (delta_h_prime > 180.0f)
            delta_h_prime -= 360.0f;
        else if (delta_h_prime < -180.0f)
            delta_h_prime += 360.0f;
    }

    float32 delta_H_prime = 2.0f * sqrtf(c1_prime * c2_prime) * sinf(glm_rad(delta_h_prime / 2.0f));

    float32 L_prime_avg = (l1 + l2) / 2.0f;
    float32 C_prime_avg = (c1_prime + c2_prime) / 2.0f;

    float32 h_prime_avg = h1_prime + h2_prime;
    if (chromatic)
    {
        if (fabsf(h1_prime - h2_prime) <= 180.0f)
            h_prime_avg = (h1_prime + h2_prime) / 2.0f;
        else if (h1_prime + h2_prime < 360.0f)
            h_prime_avg = (h1_prime + h2_prime + 360.0f) / 2.0f;
        else
            h_prime_avg = (h1_prime + h2_prime - 360.0f) / 2.0f;
    }

    float32 T = 1.0f - 0.17f * cosf(glm_rad(h_prime_avg - 30.0f)) + 0.24f * cosf(glm_rad(2.0f * h_prime_avg)) +
                0.32f * cosf(glm_rad(3.0f * h_prime_avg + 6.0f)) - 0.20f * cosf(glm_rad(4.0f * h_prime_avg - 63.0f));

    float32 hue_term = (h_prime_avg - 275.0f) / 25.0f;
    float32 delta_theta = 30.0f * expf(-(hue_term * hue_term));

    float32 C_prime_avg7 = Pow7(C_prime_avg);
    float32 R_c = 2.0f * sqrtf(C_prime_avg7 / (C_prime_avg7 + POW25_7));
    float32 R_t = -sinf(glm_rad(2.0f * delta_theta)) * R_c;

    float32 L_minus_50_sq = (L_prime_avg - 50.0f) * (L_prime_avg - 50.0f);
    float32 S_L = 1.0f + (0.015f * L_minus_50_sq) / sqrtf(20.0f + L_minus_50_sq);
    float32 S_C = 1.0f + 0.045f * C_prime_avg;
    float32 S_H = 1.0f + 0.015f * C_prime_avg * T;

    float32 diff_L = delta_L_prime / (kL * S_L);
    float32 diff_C = delta_C_prime / (kC * S_C);
    float32 diff_H = delta_H_prime / (kH * S_H);

    return diff_L * diff_L + diff_C * diff_C + diff_H * diff_H + R_t * diff_C * diff_H;
}

int32 FindBestPaletteColorCIEDE2000(PaletteManager *manager, ColorLAB *target_pixel)
{
    int32 size = (int32)kv_size(manager->palette_elements_lab);

    int32 best_index = -1;
    float32 best_score = FLT_MAX;
    for (int32 i = 0; i < size; i++)
    {
        const ColorLAB *candidate = &kv_A(manager->palette_elements_lab, i);
        float32 score = CalculateCIEDE2000(target_pixel, candidate);
        if (score < best_score)
        {
            best_score = score;
            best_index = i;
        }
    }

    return best_index;
}