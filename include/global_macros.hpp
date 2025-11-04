/**
 * @file global_macros.hpp
 * @brief Global macros and utility definitions for the project.
 */

#ifndef MANGANESE_INCLUDE_GLOBAL_MACROS_HPP
#define MANGANESE_INCLUDE_GLOBAL_MACROS_HPP

#include <stdint.h>
#include <utils/compiler_config.h>

#include <utils/debug_utils.hpp>


// Some common includes that are used frequently

/**
 * Indicates that a function will not throw in release builds, to enable more optimizations.
 * This is a no-op in debug builds, where more exceptions are expected.
 * Note: This macro should not be used on functions that will actually never throw (in those cases, use `noexcept`
 * directly).
 */

#if DEBUG
#define noexcept_if_release  // In debug mode, these functions are more likely to throw -- don't use noexcept
#else  // ^^ DEBUG vv !DEBUG
#define noexcept_if_release noexcept  // In release builds, optimize these functions more
#endif

#define DISCARD(value) (void)(value)  // Explicitly discard a value

#endif  // MANGANESE_INCLUDE_GLOBAL_MACROS_HPP