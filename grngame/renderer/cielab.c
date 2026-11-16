#include "cielab.h"
#include "SDL3/SDL_pixels.h"
#include "grngame/core/app.h"
#include "grngame/math/math.h"
#include "kvec.h"
#include <math.h>
#include <cglm/cglm.h>

static float32 srgb_linear_lut[256];

void InitLinearLut()
{
    for (int32 i = 0; i < 256; i++)
    {
        float32 v = i / 255.0;
        if (v <= 0.04045)
            srgb_linear_lut[i] = v / 12.92;
        else
            srgb_linear_lut[i] = pow((v + 0.055) / 1.055, 2.4);
    }
}

static float32 PivotXYZ(float32 n)
{
    float32 epsilon = 216.0 / 24389.0;
    float32 kappa = 24389.0 / 27.0;

    if (n > epsilon)
    {
        return cbrt(n);
    }
    else
    {
        return (kappa * n + 16.0) / 116.0;
    }
}

static HOT ColorXYZ RgbToXyz(SDL_Color *rgb)
{
    float32 r = srgb_linear_lut[rgb->r];
    float32 g = srgb_linear_lut[rgb->g];
    float32 b = srgb_linear_lut[rgb->b];
    ColorXYZ xyz;
    xyz.x = (r * 0.4124564 + g * 0.3575761 + b * 0.1804375) * 100.0;
    xyz.y = (r * 0.2126729 + g * 0.7151522 + b * 0.0721750) * 100.0;
    xyz.z = (r * 0.0193339 + g * 0.1191920 + b * 0.9503041) * 100.0;

    return xyz;
}

static ColorLAB XyzToLab(ColorXYZ *xyz)
{
    float32 ref_X = 95.047;
    float32 ref_Y = 100.000;
    float32 ref_Z = 108.883;

    float32 x = PivotXYZ(xyz->x / ref_X);
    float32 y = PivotXYZ(xyz->y / ref_Y);
    float32 z = PivotXYZ(xyz->z / ref_Z);

    ColorLAB lab;
    lab.l = fmax(0.0, 116.0 * y - 16.0);
    lab.a = 500.0 * (x - y);
    lab.b = 200.0 * (y - z);

    return lab;
}

ColorLAB HOT RgbToLab(SDL_Color *color)
{
    ColorXYZ temp_xyz = RgbToXyz(color);
    return XyzToLab(&temp_xyz);
}

HOT static float32 CalculateCIEDE2000(const ColorLAB *color1, const ColorLAB *color2)
{
    float32 kL = 1.0;
    float32 kC = 1.0;
    float32 kH = 1.0;

    float32 l1 = color1->l;
    float32 a1 = color1->a;
    float32 b1 = color1->b;

    float32 l2 = color2->l;
    float32 a2 = color2->a;
    float32 b2 = color2->b;

    float32 c1 = sqrt(a1 * a1 + b1 * b1);
    float32 c2 = sqrt(a2 * a2 + b2 * b2);
    float32 c_avg = (c1 + c2) / 2.0;

    float32 c_avg7 = pow(c_avg, 7.0);
    float32 g = 0.5 * (1.0 - sqrt(c_avg7 / (c_avg7 + 6103515625.0)));

    float32 a1_prime = a1 * (1.0 + g);
    float32 a2_prime = a2 * (1.0 + g);

    float32 c1_prime = sqrt(a1_prime * a1_prime + b1 * b1);
    float32 c2_prime = sqrt(a2_prime * a2_prime + b2 * b2);

    float32 h1_prime = 0.0;
    if (b1 != 0 || a1_prime != 0)
    {
        h1_prime = glm_deg(atan2(b1, a1_prime));
        if (h1_prime < 0.0)
            h1_prime += 360.0;
    }

    float32 h2_prime = 0.0;
    if (b2 != 0 || a2_prime != 0)
    {
        h2_prime = glm_deg(atan2(b2, a2_prime));
        if (h2_prime < 0.0)
            h2_prime += 360.0;
    }

    float32 delta_L_prime = l2 - l1;
    float32 delta_C_prime = c2_prime - c1_prime;

    float32 delta_h_prime = 0.0;
    if (c1_prime * c2_prime != 0.0)
    {
        if (fabs(h2_prime - h1_prime) <= 180.0)
        {
            delta_h_prime = h2_prime - h1_prime;
        }
        else if (h2_prime - h1_prime > 180.0)
        {
            delta_h_prime = h2_prime - h1_prime - 360.0;
        }
        else
        {
            delta_h_prime = h2_prime - h1_prime + 360.0;
        }
    }

    float32 delta_H_prime = 2.0 * sqrt(c1_prime * c2_prime) * sin(glm_deg(delta_h_prime) / 2.0);

    float32 L_prime_avg = (l1 + l2) / 2.0;
    float32 C_prime_avg = (c1_prime + c2_prime) / 2.0;

    float32 h_prime_avg = 0.0;
    if (c1_prime * c2_prime != 0.0)
    {
        if (fabs(h1_prime - h2_prime) <= 180.0)
        {
            h_prime_avg = (h1_prime + h2_prime) / 2.0;
        }
        else if (h1_prime + h2_prime < 360.0)
        {
            h_prime_avg = (h1_prime + h2_prime + 360.0) / 2.0;
        }
        else
        {
            h_prime_avg = (h1_prime + h2_prime - 360.0) / 2.0;
        }
    }
    float32 T = 1.0 - 0.17 * cos(glm_deg(h_prime_avg - 30.0)) + 0.24 * cos(glm_deg(2.0 * h_prime_avg)) +
                0.32 * cos(glm_deg(3.0 * h_prime_avg + 6.0)) - 0.20 * cos(glm_deg(4.0 * h_prime_avg - 63.0));

    float32 delta_theta = 30.0 * exp(-pow((h_prime_avg - 275.0) / 25.0, 2.0));
    float32 C_prime_avg7 = pow(C_prime_avg, 7.0);
    float32 R_c = 2.0 * sqrt(C_prime_avg7 / (C_prime_avg7 + 6103515625.0));
    float32 R_t = -sin(glm_deg(2.0 * delta_theta)) * R_c;
    float32 L_minus_50_sq = pow(L_prime_avg - 50.0, 2.0);
    float32 S_L = 1.0 + ((0.015 * L_minus_50_sq) / sqrt(20.0 + L_minus_50_sq));
    float32 S_C = 1.0 + 0.045 * C_prime_avg;
    float32 S_H = 1.0 + 0.015 * C_prime_avg * T;
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
