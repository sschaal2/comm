# Every SL directory has a symbolic link to config/bazel to access the config files as local path.
# While not pretty, this allows BUILD files to be independt of the SL_ROOT workspace path, and only
# SL.bzl needs to be adjusted
load(":bazel/SL.bzl", "SL_ROOT", "SL_ROOT_WS", "SL_VISIBILITY")

package(default_visibility = SL_VISIBILITY)

licenses(["notice"])

exports_files(["LICENSE"])

# Every SL directory has a symbolic link to config/bazel to access the config files as local path.
# While not pretty, this allows BUILD files to be independt of the SL_ROOT workspace path, and only
# SL.bzl needs to be adjusted

load(":bazel/SL.bzl", "SL_ROOT")

# the comm packages has various simple communication libarieess

# a simple udp communication library
cc_library(
    name = "udp_communication",
    srcs = [
        "src/udp_communication.cpp",
    ],
    includes = [
        "include",
    ],
    textual_hdrs = [
        "include/udp_communication.h",
    ],
    deps = [SL_ROOT + "utilities:utility"],
)

# a test for udp communiction
cc_binary(
    name = "xudpTest",
    srcs = [
        "src/udp_main.cpp",
    ],
    includes = [
        "-Iinclude",
        "-Iutilities/include",
    ],
    deps = [
        ":udp_communication",
        SL_ROOT + "utilities:utility",
    ],
)


# a simple serial communication library
cc_library(
    name = "serial_communication",
    srcs = [
        "src/serial_communication.cpp",
    ],
    includes = [
        "include",
    ],
    textual_hdrs = [
        "include/serial_communication.h",
    ],
    deps = [SL_ROOT + "utilities:utility"],
)

