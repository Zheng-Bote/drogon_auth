# GEMINI.md - Drogon Authentication Microservice

## C++23 Development Guidelines

This document outlines the coding standards, best practices, and toolchain configurations for C++23 development in this project.

## 1. Toolchain & Environment

- **Standard Version**: C++23
- **Compiler Flags**:
  - GCC/Clang: `-std=c++23 -Wall -Wextra -Wpedantic -Wshadow -Wconversion`
  - MSVC: `/std:c++latest /W4`
- **Build System**: CMake >=3.31+ (required for full module support and modern features)
- **Package Manager**: Conan (required for dependency management)

## 2. CMake Standards

All `CMakeLists.txt` must adhere to the following standards:

- **Minimum Version**: `cmake_minimum_required(VERSION 3.28)`
- **Project Declaration**: Always include `VERSION`, `DESCRIPTION`, and `HOMEPAGE_URL`.
  ```cmake
  project(photo_indexer
      VERSION 0.1.0
      DESCRIPTION "A high-performance C++23 photo indexing backend"
      HOMEPAGE_URL "https://github.com/zheng-bote/photo_indexer"
      LANGUAGES CXX
  )
  ```
- **Standard Enforcement**:
  ```cmake
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  ```
- **Dependency Management**: Use Conan for all external libraries.
- **Library Tracking**: Generate a `libs.txt` file for transparency in the build directory:
  ```cmake
  # libs.txt Generate (timing-safe via GENERATE)
  file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/libs.txt"
       CONTENT "$<TARGET_PROPERTY:${PROJECT_NAME},LINK_LIBRARIES>")
  ```

## 3. Key C++23 Features

### 3.1. Formatted Output (`<print>`)

Use `std::print` and `std::println` over `std::cout` for performance and cleaner syntax.

```cpp
#include <print>

void greet(std::string_view name) {
    std::println("Hello, {}!", name);
}
```

### 3.2. Error Handling (`std::expected`)

Prefer `std::expected<T, E>` for operations that can fail, instead of throwing exceptions or returning error codes/pointers.

```cpp
#include <expected>

std::expected<int, std::string> parse_int(std::string_view input) {
    if (valid(input)) return std::stoi(std::string(input));
    return std::unexpected("Invalid number");
}
```

### 3.3. Modules (`import std`)

Where compiler support permits, use C++ modules to improve build times and isolation. Fallback to headers if not fully stable.

### 3.4. Explicit Object Parameter ("Deducing This")

Use strict `this` deduction for recursive lambdas and reducing boilerplate in CRTP patterns.

## 4. Coding Style

- **Naming**:
  - Variables/Functions: `snake_case`
  - Classes/Structs: `PascalCase`
  - Constants: `SCREAMING_SNAKE_CASE` or `kCamelCase`
- **Modern Idioms**:
  - Always use `auto` when types are obvious.
  - Use `[[nodiscard]]` for pure functions.
  - Use `constexpr` and `consteval` by default for computation logic.

## 5. File Structure

- Headers: `.hpp` stored in `include/`, Sources: `.cpp` stored in `src/`.
- Type files: `*_type.hpp`
- Utilities: `*_util.hpp`
- Models: `*_model.hpp`
- Controllers: `*_ctrl.hpp`
- Services: `*_srv.hpp` / `*_srvpl.cpp` (Service / Service Impl)

## 6. Documentation & Licensing

- **License**: Apache-2.0
- **NOTICE**: Maintain a `NOTICE` file containing copyright and attribution notices.
- **CHANGELOG.md**: Always update `CHANGELOG.md` when changes are made.
- **SPDX Headers**: Every source file must contain the SPDX header (Apache-2.0).

### Source Code Documentation (Doxygen)

Always use Doxygen style comments in English for every public entity.

```cpp
/**
 * SPDX-FileComment: <Description of the file>
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: <Year> ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file <filename.hpp>
 * @brief <Description of the file>
 * @version <X.Y.Z>
 * @date <YYYY-MM-DD>
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) <Year> ZHENG Robert
 * @license Apache-2.0
 */
```

## 7. Architecture Documentation

- README.md must contain an architecture overview.
- Use Mermaid diagrams for all architectural views (Class, Sequence, Component, etc.) stored in `docs/architecture/`.
- Detailed documentation stays in `docs/documentation/`.
- Architecture model can be C4, Archimate, or UML.

## 8. Tooling & Quality

- **Formatting**: Use `.clang-format` based on the LLVM or Google style (configured in the project).
- **Static Analysis**: Integrate `clang-tidy` into the build process where possible.
- **Testing**: Use a modern testing framework (e.g., GTest or Catch2) managed via Conan.
- **Continuous Integration**: Ensure all PRs pass build, lint, and test stages.
