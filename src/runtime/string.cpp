#include <runtime/string.hpp>

extern "C" {

RuntimeString mn_strcat(RuntimeString, RuntimeString) { return {}; }
int32_t mn_strcmp(RuntimeString, RuntimeString) { return 0; }
}