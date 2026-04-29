"""PlatformIO pre-script: make `gcc`/`g++`/`ar`/`ranlib` resolve to clang.

The platform-native toolchain hardcodes those binary names, but this
Windows host has LLVM clang instead of MinGW. We:

 1. Prepend the in-repo `scripts/shims/` directory (containing thin
    .cmd wrappers that forward to clang/clang++/llvm-ar/llvm-ranlib)
    to PATH before SCons spawns any compile commands.
 2. Force-include the MQTTManager mock for C++ translation units only
    (NOT for Unity's C files), so when src/ScheduleManager.cpp later
    does #include "MQTTManager.h" the matching include guard fires
    and the real (PubSubClient/WiFi-dependent) header is skipped.

Only used by [env:native].
"""

import os

Import("env")  # type: ignore  # noqa: F821

PROJECT_DIR = env["PROJECT_DIR"]
SHIM_DIR = os.path.join(PROJECT_DIR, "scripts", "shims")
MOCK_MQTT = os.path.join(PROJECT_DIR, "test", "test_native", "mocks",
                         "MQTTManager.h")

if os.path.isdir(SHIM_DIR):
    current = os.environ.get("PATH", "")
    if SHIM_DIR not in current:
        os.environ["PATH"] = SHIM_DIR + os.pathsep + current
    env["ENV"] = env.get("ENV", {})
    env_path = env["ENV"].get("PATH", os.environ["PATH"])
    if SHIM_DIR not in env_path:
        env["ENV"]["PATH"] = SHIM_DIR + os.pathsep + env_path
    print(">> [native_use_clang] PATH prepended with %s" % SHIM_DIR)

# C++ only — Unity itself is C and would choke on <string> via -include.
env.Append(CXXFLAGS=["-include", MOCK_MQTT])
print(">> [native_use_clang] CXXFLAGS += -include %s" % MOCK_MQTT)
