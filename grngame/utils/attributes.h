#pragma once

#include "grngame/dev/logging.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * LIKELY: This branch is likely to happen
 * UNLIKELY: This branch is unlikely to happen
 * UNREACHABLE: The execution of the code cannot reach this place
 * HOT: This function is called often (rather useless nowadays)
 * COLD: This function is not called often (rather useless nowadays)
 * FORCE_INLINE: This function should always be inlined whenever it is called
 * NO_INLINE: This function should never be inlined whenever it is called
 * PREFETCH: Load the cacheline that starts here before it's really accesses
 * PREFETCH_WRITE: Prefetch but allows writing
 * ASSUME: Tell the compiler something is true
 * FLATTEN: All calls inside this function should be inlined
 * RESTRICT: pointers never overlap
 * CONSTRUCTOR: Declares a function that runs automatically before
 * main(), at program/module load time, without needing to be called
 * explicitly anywhere.
 * THREAD_LOCAL: variable local to a thread
 */

#if defined(__clang__) || defined(__GNUC__)
#define RESTRICT __restrict__
#elif defined(_MSC_VER)
#define RESTRICT __restrict
#else
#define RESTRICT
#endif

#if defined(__clang__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define UNREACHABLE() __builtin_unreachable()
#define HOT __attribute__((hot))
#define COLD __attribute__((cold))
#define FORCE_INLINE __attribute__((always_inline)) inline
#define NO_INLINE __attribute__((noinline))
#define PREFETCH(ptr) __builtin_prefetch((ptr), 0, 1)
#define PREFETCH_WRITE(ptr) __builtin_prefetch((ptr), 1, 1)
#define ASSUME(x) __builtin_assume(x)
#define FLATTEN __attribute__((flatten))

#elif defined(__GNUC__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define UNREACHABLE() __builtin_unreachable()
#define HOT __attribute__((hot))
#define COLD __attribute__((cold))
#define FORCE_INLINE __attribute__((always_inline)) inline
#define NO_INLINE __attribute__((noinline))
#define PREFETCH(ptr) __builtin_prefetch((ptr), 0, 1)
#define PREFETCH_WRITE(ptr) __builtin_prefetch((ptr), 1, 1)
#define ASSUME(x)                                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
            __builtin_unreachable();                                                                                   \
    } while (0)
#define FLATTEN __attribute__((flatten))

#elif defined(_MSC_VER)
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define UNREACHABLE() __assume(0)
#define HOT __pragma(optimize("gt", on))
#define COLD __pragma(optimize("s", on))
#define FORCE_INLINE __forceinline
#define NO_INLINE __declspec(noinline)
#define PREFETCH(ptr) _mm_prefetch((const char *)(ptr), _MM_HINT_T0)
#define PREFETCH_WRITE(ptr) _mm_prefetch((const char *)(ptr), _MM_HINT_T1)
#define ASSUME(x) __assume(x)
#define FLATTEN

#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define UNREACHABLE() ((void)0)
#define HOT
#define COLD
#define FORCE_INLINE inline
#define NO_INLINE
#define PREFETCH(ptr) ((void)(ptr))
#define PREFETCH_WRITE(ptr) ((void)(ptr))
#define ASSUME(x) ((void)0)
#define FLATTEN

#endif

/* STACK_ALLOC : stack allocation */
#ifdef _MSC_VER
#include <malloc.h>
#define STACK_ALLOC(type, n) ((type *)_alloca(sizeof(type) * (n)))
#else
#include <alloca.h>
#define STACK_ALLOC(type, n) ((type *)alloca(sizeof(type) * (n)))
#endif

#if defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
#define CONSTRUCTOR(fn)                                                                                                \
    static void fn(void);                                                                                              \
    __declspec(allocate(".CRT$XCU")) void(__cdecl * fn##_)(void) = (void(__cdecl *)(void))fn;                          \
    static void fn(void)
#else
#define CONSTRUCTOR(fn) __attribute__((constructor)) static void fn(void)
#endif

#if defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif
