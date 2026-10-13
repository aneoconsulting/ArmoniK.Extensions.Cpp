#pragma once

/**
 * @file Diagnostics.h
 * @brief Portable macros to locally silence compiler diagnostics
 *
 * ARMONIK_SDK_SUPPRESS_DEPRECATED_BEGIN / ARMONIK_SDK_SUPPRESS_DEPRECATED_END bracket code that uses
 * [[deprecated]] entities on purpose, such as the legacy TaskPayload path.
 */

#if defined(_MSC_VER) && !defined(__clang__)
#define ARMONIK_SDK_SUPPRESS_DEPRECATED_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4996))
#define ARMONIK_SDK_SUPPRESS_DEPRECATED_END __pragma(warning(pop))
#elif defined(__GNUC__) // GCC and Clang
#define ARMONIK_SDK_SUPPRESS_DEPRECATED_BEGIN                                                                          \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define ARMONIK_SDK_SUPPRESS_DEPRECATED_END _Pragma("GCC diagnostic pop")
#else
#define ARMONIK_SDK_SUPPRESS_DEPRECATED_BEGIN
#define ARMONIK_SDK_SUPPRESS_DEPRECATED_END
#endif
