#include "text.h"
#include "../math/types.h"
#include "cglm/types-struct.h"
#include "grngame/assets/asset_manager.h"
#include "grngame/dev/logging.h"
#include "grngame/renderer/sprite.h"
#include "grngame/utils/string_compat.h"

static void LayoutNewLine(TextLayout *l)
{
    if (l->current_line_width > l->max_line_width)
        l->max_line_width = l->current_line_width;

    l->cursor_x = l->start_x;
    l->cursor_y += l->line_height;
    l->current_line_width = 0.0f;
    l->line_start = true;
}

static bool LayoutOverflows(const TextLayout *l, float32 width, int32 max_width)
{
    return !l->line_start && (l->cursor_x - l->start_x + width) > (float32)max_width;
}

static void SetSizeOut(vec2s *size_out, float32 x, float32 y)
{
    if (size_out)
        *size_out = (vec2s){.x = x, .y = y};
}

static int32 FindPatternLetter(const Font *font, char c)
{
    // todo o(1)
    for (int32 i = 0; i < FONT_PATTERN_MAX && font->pattern[i].letter != '\0'; i++)
    {
        if (font->pattern[i].letter == c)
            return i;
    }
    return -1;
}

static float32 GlyphWidth(const Font *font, char c, int32 idx, uint8 coeff)
{
    // space is not a texture
    if (c == ' ')
        return (float32)(font->space_size * coeff);

    float32 w = font->size_of_case.x;
    if (idx >= 0)
        w += font->pattern[idx].size.x + font->pattern[idx].size.y;
    return w * (float32)coeff;
}

static float32 GetWordWidth(const Font *font, const char *word, int32 space_between_letters, uint8 coeff)
{
    float32 width = 0.0f;
    int32 count = 0;

    for (uint32 i = 0; word[i] != '\0' && word[i] != ' ' && word[i] != '\n'; i++)
    {
        width += GlyphWidth(font, word[i], FindPatternLetter(font, word[i]), coeff) + (float32)space_between_letters;
        count++;
    }

    if (count > 0)
        width -= (float32)space_between_letters;
    return width;
}

bool TextDraw(const Font *font, const char *text, vec2s position, int32 space_between_letters, int32 max_width,
              uint8 coeff, uint8 alpha, vec2s *size_out)
{
    SetSizeOut(size_out, 0.0f, 0.0f);

    bool find = false;
    Texture tex = FindImage(font->name, &find);
    if (!find)
    {
        LOG_WARNING("Impossible to get font texture: %s", font->name);
        return false;
    }

    Sprite sprite = {
        .w = (uint16)font->size_of_case.x, .h = (uint16)font->size_of_case.y, .tex = tex, .find = true, .name = font->name};

    TextLayout l = {
        .start_x = position.x,
        .start_y = position.y,
        .cursor_x = position.x,
        .cursor_y = position.y,
        .line_height = font->size_of_case.y * (float32)coeff,
        .line_start = true,
    };

    const float32 spacing = (float32)space_between_letters;
    bool all_drawn = true;

    for (uint32 i = 0; text[i] != '\0'; i++)
    {
        const char ch = text[i];

        if (ch == '\n')
        {
            LayoutNewLine(&l);
            continue;
        }
        const bool word_start = (ch != ' ') && (i == 0 || text[i - 1] == ' ' || text[i - 1] == '\n');
        if (max_width > 0 && word_start &&
            LayoutOverflows(&l, GetWordWidth(font, &text[i], space_between_letters, coeff), max_width))
        {
            LayoutNewLine(&l);
        }

        const int32 idx = (ch == ' ') ? -1 : FindPatternLetter(font, ch);
        const float32 glyph_w = GlyphWidth(font, ch, idx, coeff);

        if (max_width > 0 && LayoutOverflows(&l, glyph_w, max_width))
        {
            LayoutNewLine(&l);
            if (ch == ' ')
                continue;
        }

        if (ch == ' ')
        {
            l.cursor_x += glyph_w + spacing;
            l.line_start = false;
            continue;
        }

        if (idx < 0)
        {
            LOG_DEBUG("Character '%c' not found in pattern for font %s", ch, font->name);
            l.current_line_width = (l.cursor_x - l.start_x) + glyph_w;
            l.cursor_x += glyph_w + spacing;
            l.line_start = false;
            continue;
        }

        const PatternLetter *glyph = &font->pattern[idx];
        const float32 cell_w = (font->size_of_case.x + glyph->size.y) * (float32)coeff;

        l.cursor_x += glyph->size.x * (float32)coeff;
        if (!SpriteDraw(sprite, (uint16)idx, l.cursor_x, l.cursor_y + (float32)(glyph->y_offset * coeff), coeff, 0,
                        alpha))
        {
            all_drawn = false;
        }

        l.current_line_width = (l.cursor_x - l.start_x) + cell_w;
        l.cursor_x += cell_w + spacing;
        l.line_start = false;
    }

    if (l.current_line_width > l.max_line_width)
        l.max_line_width = l.current_line_width;

    if (text[0] != '\0')
        SetSizeOut(size_out, l.max_line_width, (l.cursor_y - l.start_y) + l.line_height);

    return all_drawn;
}