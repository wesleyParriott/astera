#ifndef PLAT_H
#define PLAT_H

#include "config.h"
#include <stdint.h>

// --- SYSTEM STANDARDS ---
#define _POSIX_C_SOURCE 199309L

// Define POSIX features for C99
#if !defined(_XOPEN_SOURCE) && !defined(_POSIX_C_SOURCE)
#if defined(__cplusplus)
#define _XOPEN_SOURCE 700 /* SUS v4, POSIX 1003.1 2008/13 (POSIX 2008/13) */
#elif __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 700 /* SUS v4, POSIX 1003.1 2008/13 (POSIX 2008/13) */
#else
#define _XOPEN_SOURCE 500 /* SUS v2, POSIX 1003.1 1997 */
#endif                    /* __STDC_VERSION__ */
#endif                    /* !_XOPEN_SOURCE && !_POSIX_C_SOURCE */

// PLATFORM DETECTION
#if defined(_WIN32)
#define PLAT_MSFT
#if defined(_WIN64)
#define PLAT_MSFT_64
#endif
#elif defined(__APPLE__)
#define PLAT_APPLE
#if defined(TARGET_IPHONE_SIMULATOR)
#define PLAT_IOS_SIM
#elif defined(TARGET_OS_IPHONE)
#define PLAT_IOS
#elif defined(TARGET_OS_MAC)
#define PLAT_MAC
#else
#define PLAT_APPLE_UNK
#endif
#elif defined(__linux__)
#define PLAT_LINUX
#elif defined(__unix__)
#define PLAT_UNIX
#elif defined(_POSIX_VERSION)
#define PLAT_POSIX
#elif __FreeBSD__
#define PLAT_BSD
#else
#define PLAT_UNK
#endif

// ARCHITECTURE DETECTION
#if defined(__x86_64__) || defined(_M_AMD64) || defined(__x86_64) ||           \
    defined(__amd64__) || defined(__amd64)
#define ARCH_X86_64
#elif defined(__arm__) || defined(_M_ARM)
#define ARCH_ARM
#elif defined(__aarch64__)
#define ARCH_ARM64
#elif defined(i386) || defined(__i386) || defined(__i386__) ||                 \
    defined(_M_IX86) || defined(_X86_)
#define ARCH_i386
#else
#define ARCH_UNK
#endif

#ifndef DEBUG_OUTPUT
#if defined(PLAT_MSFT) || defined(PLAT_MSFT_64)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#endif

// NOTE: Temporary, will be refactoring back out to system types
// I should definitely factor these out lol
#define INCLUDE_TYPES
#if !defined(TYPES_INCLUDED)
#define TYPES_INCLUDED

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

#endif
#endif
