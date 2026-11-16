#include "cielab.h"
#include "SDL3/SDL_pixels.h"
#include "grngame/core/app.h"
#include "kvec.h"
#include <math.h>
#include <cglm/cglm.h>

static float32 srgb_linear_lut[256];

void InitLinearLut()
{
    for (int32 i = 0; i < 256; i++)
    {
        float32 v = (float32)i / 255.0f;
        if (v <= 0.04045)
            srgb_linear_lut[i] = v / 12.92f;
        else
            srgb_linear_lut[i] = powf((v + 0.055f) / 1.055f, 2.4f);
    }
}

static float32 PivotXYZ(float32 n)
{
    float32 epsilon = 216.0f / 24389.0f;
    float32 kappa = 24389.0f/ 27.0f;

    if (n > epsilon)
    {
        return cbrtf(n);
    }
    else
    {
        return (kappa * n + 16.0f) / 116.0f;
    }
}

static HOT ColorXYZ RgbToXyz(SDL_Color *rgb)
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

static ColorLAB XyzToLab(ColorXYZ *xyz)
{
    float32 ref_X = 95.047f;
    float32 ref_Y = 100.000f;
    float32 ref_Z = 108.883f;

    float32 x = PivotXYZ(xyz->x / ref_X);
    float32 y = PivotXYZ(xyz->y / ref_Y);
    float32 z = PivotXYZ(xyz->z / ref_Z);

    ColorLAB lab;
    lab.l = fmaxf(0.0f, 116.0f * y - 16.0f);
    lab.a = 500.0f * (x - y);
    lab.b = 200.0f * (y - z);

    return lab;
}

ColorLAB HOT RgbToLab(SDL_Color *color)
{
    ColorXYZ temp_xyz = RgbToXyz(color);
    return XyzToLab(&temp_xyz);
}

HOT static float32 CalculateCIEDE2000(const ColorLAB *color1, const ColorLAB *color2)
{
    float32 kL = 1.0f;
    float32 kC = 1.0f;
    float32 kH = 1.0f;

    float32 l1 = color1->l;
    float32 a1 = color1->a;
    float32 b1 = color1->b;

    float32 l2 = color2->l;
    float32 a2 = color2->a;
    float32 b2 = color2->b;

    float32 c1 = sqrtf(a1 * a1 + b1 * b1);
    float32 c2 = sqrtf(a2 * a2 + b2 * b2);
    float32 c_avg = (c1 + c2) / 2.0f;

    float32 c_avg7 = powf(c_avg, 7.0f);
    float32 g = 0.5f * (1.0f - sqrtf(c_avg7 / (c_avg7 + 6103515625.0f)));

    float32 a1_prime = a1 * (1.0f + g);
    float32 a2_prime = a2 * (1.0f + g);

    float32 c1_prime = sqrtf(a1_prime * a1_prime + b1 * b1);
    float32 c2_prime = sqrtf(a2_prime * a2_prime + b2 * b2);

    float32 h1_prime = 0.0f;
    if (b1 != 0 || a1_prime != 0)
    {
        h1_prime = glm_deg(atan2f(b1, a1_prime));
        if (h1_prime < 0.0)
            h1_prime += 360.0f;
    }

    float32 h2_prime = 0.0f;
    if (b2 != 0 || a2_prime != 0)
    {
        h2_prime = glm_deg(atan2f(b2, a2_prime));
        if (h2_prime < 0.0)
            h2_prime += 360.0f;
    }

    float32 delta_L_prime = l2 - l1;
    float32 delta_C_prime = c2_prime - c1_prime;

    float32 delta_h_prime = 0.0f;
    if (c1_prime * c2_prime != 0.0f)
    {
        if (fabs(h2_prime - h1_prime) <= 180.0f)
        {
            delta_h_prime = h2_prime - h1_prime;
        }
        else if (h2_prime - h1_prime > 180.0f)
        {
            delta_h_prime = h2_prime - h1_prime - 360.0f;
        }
        else
        {
            delta_h_prime = h2_prime - h1_prime + 360.0f;
        }
    }

    float32 delta_H_prime = 2.0f * sqrtf(c1_prime * c2_prime) * sinf(glm_deg(delta_h_prime) / 2.0f);

    float32 L_prime_avg = (l1 + l2) / 2.0f;
    float32 C_prime_avg = (c1_prime + c2_prime) / 2.0f;

    float32 h_prime_avg = 0.0f;
    if (c1_prime * c2_prime != 0.0)
    {
        if (fabs(h1_prime - h2_prime) <= 180.0f)
        {
            h_prime_avg = (h1_prime + h2_prime) / 2.0f;
        }
        else if (h1_prime + h2_prime < 360.0)
        {
            h_prime_avg = (h1_prime + h2_prime + 360.0f) / 2.0f;
        }
        else
        {
            h_prime_avg = (h1_prime + h2_prime - 360.0f) / 2.0f;
        }
    }
    float32 T = 1.0f - 0.17f * cosf(glm_deg(h_prime_avg - 30.0f)) + 0.24f * cosf(glm_deg(2.0f * h_prime_avg)) +
                0.32f * cosf(glm_deg(3.0f * h_prime_avg + 6.0f)) - 0.20f * cosf(glm_deg(4.0f * h_prime_avg - 63.0f));

    float32 delta_theta = 30.0f * expf(-powf((h_prime_avg - 275.0f) / 25.0f, 2.0f));
    float32 C_prime_avg7 = powf(C_prime_avg, 7.0f);
    float32 R_c = 2.0f * sqrtf(C_prime_avg7 / (C_prime_avg7 + 6103515625.0f));
    float32 R_t = -sinf(glm_deg(2.0f* delta_theta)) * R_c;
    float32 L_minus_50_sq = powf(L_prime_avg - 50.0f, 2.0f);
    float32 S_L = 1.0f + ((0.015f * L_minus_50_sq) / sqrtf(20.0f + L_minus_50_sq));
    float32 S_C = 1.0f + 0.045f * C_prime_avg;
    float32 S_H = 1.0f + 0.015f * C_prime_avg * T;
    float32 diff_L = delta_L_prime / (kL * S_L);
    float32 diff_C = delta_C_prime / (kC * S_C);
    float32 diff_H = delta_H_prime / (kH * S_H);

    return diff_L * diff_L + diff_C * diff_C + diff_H * diff_H + R_t * diff_C * diff_H;
}

int32 FindBestPaletteColorCIEDE2000(ColorLAB *target_pixel)
{
    int32 size = (int32)kv_size(g_app.palette_manager.palette_elements_lab);

    int32 best_index = 0;
    float32 best_score = 9999999.0f;
    for (int32 i = 0; i < size; i++)
    {
        ColorLAB *candidate_lab = &kv_A(g_app.palette_manager.palette_elements_lab, i);
        float32 current_score = CalculateCIEDE2000(target_pixel, candidate_lab);
        if (current_score < best_score)
        {
            best_score = current_score;
            best_index = (int32)i;
        }
    }

    return best_index;
}
