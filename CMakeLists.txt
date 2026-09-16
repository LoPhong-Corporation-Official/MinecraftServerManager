cmake_minimum_required(VERSION 3.21)

project(MinecraftServerManager
    LANGUAGES CXX
    DESCRIPTION "Windows Minecraft Server Management Application (Qt6 UI + native Win32 process backend)"
)

if (NOT WIN32)
    message(FATAL_ERROR
        "MinecraftServerManager's process backend uses the native Win32 API "
        "(CreateProcessW, Job Objects) and cannot be built for this platform.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt's moc needs to process any header containing Q_OBJECT (MainWindow.hpp,
# AddServerDialog.hpp). AUTOMOC finds those automatically from the sources
# listed below - no manual moc_*.cpp bookkeeping needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(MSM_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(cmake/CompilerOptions.cmake)

# If CMake cannot find Qt6, point it at your install, e.g.:
#   cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64"
set(Qt6_DIR
    "C:/Qt/6.11.2/msvc2022_64/lib/cmake/Qt6"
)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(MinecraftServerManager WIN32
    src/main.cpp

    src/app/Application.hpp
    src/app/Application.cpp

    src/core/Types.hpp
    src/core/Event.hpp
    src/core/EventDispatcher.hpp
    src/core/EventDispatcher.cpp
    src/core/Logger.hpp
    src/core/Logger.cpp
    src/core/Json.hpp
    src/core/Json.cpp

    src/process/Process.hpp
    src/process/Process.cpp
    src/process/ProcessPipes.hpp
    src/process/ProcessPipes.cpp

    src/monitor/ProcessMonitor.hpp
    src/monitor/ProcessMonitor.cpp

    src/console/ConsoleBuffer.hpp
    src/console/ConsoleController.hpp
    src/console/ConsoleController.cpp

    src/server/MinecraftServer.hpp
    src/server/MinecraftServer.cpp
    src/server/ServerManager.hpp
    src/server/ServerManager.cpp

    src/config/ConfigManager.hpp
    src/config/ConfigManager.cpp

    src/ui/MainWindow.hpp
    src/ui/MainWindow.cpp
    src/ui/AddServerDialog.hpp
    src/ui/AddServerDialog.cpp
)

target_include_directories(MinecraftServerManager PRIVATE src)

# UNICODE / _UNICODE select the *W (wide) Win32 API overloads used by the
# process/console/config backend (CreateProcessW, etc.) - unrelated to Qt,
# which always talks to Windows in UTF-16 regardless of this macro.
# NOMINMAX avoids <windows.h>'s min/max macros clashing with std::min/max.
target_compile_definitions(MinecraftServerManager PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(MinecraftServerManager PRIVATE
    Qt6::Core
    Qt6::Widgets
)

msm_apply_compiler_options(MinecraftServerManager)

set_target_properties(MinecraftServerManager PROPERTIES
    VS_DPI_AWARE "PerMonitor"
)

# Copies the Qt DLLs (Qt6Core.dll, Qt6Widgets.dll, platforms/qwindows.dll,
# ...) next to the .exe after each build, so it runs without manually
# fiddling with PATH. Requires windeployqt.exe to be reachable, which it
# is automatically when Qt6 was found via find_package() above.
find_program(MSM_WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")
if (MSM_WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET MinecraftServerManager POST_BUILD
        COMMAND "${MSM_WINDEPLOYQT_EXECUTABLE}" --no-translations "$<TARGET_FILE:MinecraftServerManager>"
        COMMENT "Running windeployqt to stage Qt runtime DLLs next to the .exe"
    )
endif()

# --- Tests (spec section 46) -------------------------------------------
# Kept minimal for the Phase 1 MVP: a hand-rolled smoke-test executable for
# the pieces that do not require a live java.exe/Windows process/Qt event
# loop to be meaningful (JSON round-trip, config load/save).
option(MSM_BUILD_TESTS "Build the smoke-test executable" OFF)
if (MSM_BUILD_TESTS)
    add_executable(MinecraftServerManagerTests
        tests/ConfigTests.cpp

        src/core/Json.cpp
        src/core/Logger.cpp
        src/config/ConfigManager.cpp
    )
    target_include_directories(MinecraftServerManagerTests PRIVATE src)
    target_compile_definitions(MinecraftServerManagerTests PRIVATE
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    msm_apply_compiler_options(MinecraftServerManagerTests)
endif()

