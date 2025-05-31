#ifndef GLOBAL_MACROS_H
#define GLOBAL_MACROS_H

#include <stdint.h>

#define DEBUG 1

#if DEBUG
#define DEBUG_FUNC  // In debug mode, leave functions as is
#else               // ^ DEBUG ^ | v !DEBUG v
// In release mode, inline the debug functions (since they do nothing)
#ifdef _MSC_VER
#define DEBUG_FUNC inline __forceinline
#else  // ^ _MSC_VER ^ | v ! _MSC_VER v
// GCC/Clang
#define DEBUG_FUNC inline __attribute__((always_inline))
#endif  // _MSC_VER
#endif  // DEBUG

#ifdef __cplusplus
#define EXT_C_BEGIN extern "C" {
#define EXT_C_END }
#else  // ^^ __cplusplus vv !__cplusplus
#define EXT_C_BEGIN
#define EXT_C_END
#endif  // __cplusplus

#endif  // GLOBAL_MACROS_H