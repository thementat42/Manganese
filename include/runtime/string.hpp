#ifndef MANGANESE_INCLUDE_RUNTIME_STRING_HPP
#define MANGANESE_INCLUDE_RUNTIME_STRING_HPP 1

#include <core.hpp>
#include <cstddef>
#include <cstdint>

#ifndef MN_STRCMP_LESS
#define MN_STRCMP_LESS (-1)
#endif

#ifndef MN_STRCMP_EQUAL
#define MN_STRCMP_EQUAL (0)
#endif

#ifndef MN_STRCMP_GREATER
#define MN_STRCMP_GREATER (1)
#endif

MN_BEGIN_C_LINKAGE
struct RuntimeString {
    const char* data;
    size_t length;
};

MN_RUNTIME_API_FUNCTION RuntimeString mn_strcat(RuntimeString, RuntimeString);
MN_RUNTIME_API_FUNCTION int32_t mn_strcmp(RuntimeString, RuntimeString);
MN_END_C_LINKAGE

#endif  // MANGANESE_INCLUDE_RUNTIME_STRING_HPP