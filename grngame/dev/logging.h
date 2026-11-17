#pragma once

#include "grngame/utils/c_cpp.h"

#include <stdbool.h>

#if defined(GRNGAME_DESKTOP)

#include "haclog/haclog.h"

#define LOG_DEBUG(fmt, ...) HACLOG_DEBUG(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) HACLOG_INFO(fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) HACLOG_WARNING(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) HACLOG_ERROR(fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) HACLOG_FATAL(fmt, ##__VA_ARGS__)

#elif defined(GRNGAME_ANDROID)

#include <android/log.h>

#define LOG_DEBUG(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "GrnGame", fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "GrnGame", fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) __android_log_print(ANDROID_LOG_WARN, "GrnGame", fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "GrnGame", fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) __android_log_print(ANDROID_LOG_FATAL, "GrnGame", fmt, ##__VA_ARGS__)

#elif defined(GRNGAME_IOS)

#include <os/log.h>
#include <stdio.h>

#define GRN_OSLOG(fn, fmt, ...)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        char grn_log_buf_[512];                                                                                        \
        snprintf(grn_log_buf_, sizeof(grn_log_buf_), fmt, ##__VA_ARGS__);                                              \
        fn(OS_LOG_DEFAULT, "GrnGame: %{public}s", grn_log_buf_);                                                       \
    } while (0)

#define LOG_DEBUG(fmt, ...) GRN_OSLOG(os_log, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) GRN_OSLOG(os_log, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) GRN_OSLOG(os_log, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) GRN_OSLOG(os_log_error, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) GRN_OSLOG(os_log_fault, fmt, ##__VA_ARGS__)

#elif defined(GRNGAME_WASM)

#include <emscripten/emscripten.h>

#define LOG_DEBUG(fmt, ...) emscripten_log(EM_LOG_CONSOLE, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) emscripten_log(EM_LOG_CONSOLE, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) emscripten_log(EM_LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) emscripten_log(EM_LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) emscripten_log(EM_LOG_ERROR, fmt, ##__VA_ARGS__)

#endif

BEGIN_DECLARATIONS

typedef struct AppInfo AppInfo;

typedef enum
{
    LOG_TO_CONSOLE,
    LOG_TO_FILE,
} LogDestination;

bool LogInit(LogDestination log_destination);
void LogDestroy(void);

END_DECLARATIONS