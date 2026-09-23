#ifndef MANGANESE_INCLUDE_RUNTIME_STRING_HPP
#define MANGANESE_INCLUDE_RUNTIME_STRING_HPP 1

#include <cstddef>
#include <cstdint>

#define MN_STRCMP_LESS    -1
#define MN_STRCMP_EQUAL   0
#define MN_STRCMP_GREATER 1

extern "C" {
struct RuntimeString {
    const char* data;
    size_t size;
};

RuntimeString mn_strcat(RuntimeString, RuntimeString);
int32_t mn_strcmp(RuntimeString, RuntimeString);
}

#endif  // MANGANESE_INCLUDE_RUNTIME_STRING_HPP