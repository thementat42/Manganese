#ifndef GLOBAL_MACROS_H
#define GLOBAL_MACROS_H

#include <stdint.h>

#define DEBUG 1

#define ASSERT_CRITICAL(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "\033[31mCritical assertion failed: %s\n\033[0m", message); \
            abort(); \
        } \
    } while (0)

#if DEBUG
#define DEBUG_FUNC  // In debug mode, leave functions as is
#define ASSERT_DEBUG(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "033[33mDebug assertion failed: %s\n\033[0m", message); \
        } \
    } while (0)
#else               // ^ DEBUG ^ | v !DEBUG v
// In release mode, inline the debug functions (since they do nothing)
#ifdef _MSC_VER
#define DEBUG_FUNC inline __forceinline
#else  // ^ _MSC_VER ^ | v ! _MSC_VER v
// GCC/Clang
#define DEBUG_FUNC inline __attribute__((always_inline))
#endif  // _MSC_VER
#define ASSERT_DEBUG(condition, message)  // no-op in release mode
#endif  // DEBUG

#ifdef __cplusplus
#define EXT_C_BEGIN extern "C" {
#define EXT_C_END }
#else  // ^^ __cplusplus vv !__cplusplus
#define EXT_C_BEGIN
#define EXT_C_END
#endif  // __cplusplus

#endif  // GLOBAL_MACROS_H