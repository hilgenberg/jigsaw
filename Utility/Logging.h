#pragma once

#if defined(LINUX)

#include <cstdlib>
#define LOG_PRINT(fmt, ...) fprintf(stderr, fmt " (at %s:%d in %s)\n", __VA_ARGS__, __FILE__, __LINE__, __func__)
#define LOG_ERROR LOG_PRINT
#ifdef DEBUG
#define LOG_DEBUG LOG_PRINT
#else
#define LOG_DEBUG(...)
#endif

#elif defined(ANDROID)

#include <android/log.h>
#define LOG_PRINT(fmt, ...) __android_log_print(ANDROID_LOG_INFO,  "JIGSAW", fmt " (at %s:%d in %s)\n", __VA_ARGS__, __FILE__, __LINE__, __func__)
#define LOG_ERROR(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "JIGSAW", fmt " (at %s:%d in %s)\n", __VA_ARGS__, __FILE__, __LINE__, __func__)
#ifdef DEBUG
#define LOG_DEBUG(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "JIGSAW", fmt " (at %s:%d in %s)\n", __VA_ARGS__, __FILE__, __LINE__, __func__)
#else
#define LOG_DEBUG(...)
#endif

#else
#error "add logging macros"
#endif
