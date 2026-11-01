#include "../core/app.h"
#include "grngame/dev/logging.h"

const char *InputTextGet()
{
    InputManager *im = &g_app.input_manager;
    if (kv_size(im->text_input) == 0)
    {
        LOG_INFO("Input text is empty");
        return "";
    }
    kv_push(char, im->text_input, '\0');
    kv_size(im->text_input)--;
    return im->text_input.a;
}

void InputTextClear()
{
    kv_size(g_app.input_manager.text_input) = 0;
    LOG_INFO("Text input clear");
}

void InputTextPutChar(const char text)
{
    InputManager *im = &g_app.input_manager;
    kv_push(char, im->text_input, text);
}

// todo o(n) -> o(1)
void InputTextPutText(const char *text)
{
    for (int32 i = 0; text[i] != '\0'; i++)
    {
        InputTextPutChar(text[i]);
    }
}

const char *InputDropFile()
{
    return g_app.input_manager.drop_file;
}

void InputTextRemoveTextInt(uint32 size)
{
    InputManager *im = &g_app.input_manager;
    if ((uint32)size >= (uint32)kv_size(im->text_input))
    {
        kv_size(im->text_input) = 0;
    }
    else
    {
        kv_size(im->text_input) -= size;
    }
}

void InputTextRemoveTextChar(const char *pattern)
{
    InputManager *im = &g_app.input_manager;
    if (kv_size(im->text_input) == 0 || pattern == NULL || pattern[0] == '\0')
        return;

    kv_push(char, im->text_input, '\0');
    kv_size(im->text_input)--;

    char *start = im->text_input.a;
    uint32 pat_len = (uint32)strlen(pattern);
    char *match;

    while ((match = strstr(start, pattern)) != NULL)
    {
        uint32 match_pos = match - im->text_input.a;
        uint32 rem_len = kv_size(im->text_input) - (match_pos + pat_len);

        memmove(match, match + pat_len, rem_len);
        kv_size(im->text_input) -= pat_len;

        im->text_input.a[kv_size(im->text_input)] = '\0';
        start = match;
    }
}

void InputTextReplace(const char *pattern, const char *replace)
{
    InputManager *im = &g_app.input_manager;
    if (kv_size(im->text_input) == 0 || pattern == NULL || pattern[0] == '\0' || replace == NULL)
        return;

    kv_push(char, im->text_input, '\0');
    kv_size(im->text_input)--;

    uint32 pat_len = (uint32)strlen(pattern);
    uint32 rep_len = (uint32)strlen(replace);

    char *match;
    uint32 search_pos = 0;

    while ((match = strstr(im->text_input.a + search_pos, pattern)) != NULL)
    {
        uint32 match_pos = match - im->text_input.a;

        if (rep_len > pat_len)
        {
            uint32 diff = rep_len - pat_len;
            for (uint32 i = 0; i < diff; i++)
                kv_push(char, im->text_input, '\0');
            kv_size(im->text_input) -= diff;

            match = im->text_input.a + match_pos;
            uint32 rem_len = kv_size(im->text_input) - (match_pos + pat_len);
            memmove(match + rep_len, match + pat_len, rem_len + 1);
        }
        else if (rep_len < pat_len)
        {
            uint32 rem_len = kv_size(im->text_input) - (match_pos + pat_len);
            memmove(match + rep_len, match + pat_len, rem_len + 1);
        }

        memcpy(match, replace, rep_len);
        kv_size(im->text_input) = kv_size(im->text_input) - pat_len + rep_len;

        search_pos = match_pos + rep_len;
    }
}