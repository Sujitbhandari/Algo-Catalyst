#pragma once

// Algo-Catalyst version information
// These are injected by CMake at compile time via target_compile_definitions.

#ifndef ALGOCATALYST_VERSION_MAJOR
#define ALGOCATALYST_VERSION_MAJOR 1
#endif

#ifndef ALGOCATALYST_VERSION_MINOR
#define ALGOCATALYST_VERSION_MINOR 2
#endif

#ifndef ALGOCATALYST_VERSION_PATCH
#define ALGOCATALYST_VERSION_PATCH 0
#endif

#define ALGOCATALYST_VERSION_STRING        \
    std::to_string(ALGOCATALYST_VERSION_MAJOR) + "." + \
    std::to_string(ALGOCATALYST_VERSION_MINOR) + "." + \
    std::to_string(ALGOCATALYST_VERSION_PATCH)

namespace AlgoCatalyst {

struct Version {
    static constexpr int major = ALGOCATALYST_VERSION_MAJOR;
    static constexpr int minor = ALGOCATALYST_VERSION_MINOR;
    static constexpr int patch = ALGOCATALYST_VERSION_PATCH;

    static constexpr const char* name    = "AlgoCatalyst";
    static constexpr const char* tagline = "High-Performance Event-Driven Backtesting Engine";
};

} // namespace AlgoCatalyst
