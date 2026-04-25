cmake_minimum_required(VERSION 3.20)
project(AlgoCatalyst VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling (clangd, clang-tidy, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
set(SOURCES
    src/Engine.cpp
    src/Indicators.cpp
    src/Strategy.cpp
    src/AI_Regime.cpp
    src/main.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Target-based include directories (preferred over global include_directories)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Per-config compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -DNDEBUG>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
)

# Pass version info as preprocessor defines
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ALGOCATALYST_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ALGOCATALYST_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ALGOCATALYST_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# ── Unit Tests ────────────────────────────────────────────────────────────────
set(TEST_SOURCES
    tests/main.cpp
    tests/test_indicators.cpp
    tests/test_performance.cpp
    src/Indicators.cpp
    src/AI_Regime.cpp
)

add_executable(AlgoCatalystTests ${TEST_SOURCES})
target_include_directories(AlgoCatalystTests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)
target_compile_options(AlgoCatalystTests PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -Wpedantic -Wno-unused-parameter>
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
)
target_compile_definitions(AlgoCatalystTests PRIVATE
    ALGOCATALYST_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ALGOCATALYST_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ALGOCATALYST_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

enable_testing()
add_test(NAME IndicatorAndPerformanceTests COMMAND AlgoCatalystTests)

# Install rules
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/AlgoCatalyst/data OPTIONAL)

# Link necessary libraries
target_link_libraries(${PROJECT_NAME} PRIVATE)

