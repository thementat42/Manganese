#ifndef GLOBAL_MACROS_H
#define GLOBAL_MACROS_H

#define DEBUG 1

#ifdef __cplusplus
#define EXT_C_BEGIN extern "C" {
#define EXT_C_END }
#define MANG_BEGIN namespace Manganese {
#define MANG_END }
#else  // ^^ __cplusplus vv !__cplusplus
#define EXT_C_BEGIN
#define EXT_C_END
#define MANG_BEGIN
#define MANG_END
#endif  // __cplusplus

#endif  // GLOBAL_MACROS_H