#ifndef MANGANESE_INCLUDE_UTILS_RESOLUTION_STATUS_HPP
#define MANGANESE_INCLUDE_UTILS_RESOLUTION_STATUS_HPP 1


namespace Manganese {

/**
* Used for cycle detection
*/
enum class ResolutionStatus : std::int8_t {
    Failure = -1,
    InProgress = 0,
    Success = 1,
    NotStarted = 2,
};

} // namespace Manganese


#endif // MANGANESE_INCLUDE_UTILS_RESOLUTION_STATUS_HPP