#include <core.hpp>
#include <runtime/string.hpp>

MN_BEGIN_C_LINKAGE

RuntimeString mn_strcat(RuntimeString, RuntimeString) { return {}; }
int32_t mn_strcmp(RuntimeString, RuntimeString) { return 0; }

MN_END_C_LINKAGE