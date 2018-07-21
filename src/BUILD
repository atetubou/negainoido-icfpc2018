cc_binary(
    name = "nbt_viewer",
    srcs = ["nbt_viewer.cc"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
    ],
)

cc_library(
    name = "command_executer",
    srcs = ["command.h",
            "command.cc",
            "command_executer.h",
            "command_executer.cc",
            "command_util.h",
            "command_util.cc",
    ],
    copts = ["-std=c++14"],
    deps = ["//external:glog",
            "@jsoncpp_git//:jsoncpp",
    ],
)

cc_binary(
    name = "mdl2txt",
    srcs = ["mdl2txt.cc"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
    ],
)

cc_binary(
    name = "mdl2txt_test",
    srcs = ["mdl2txt_test.cc"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
    ],
)

cc_binary(
    name = "simple_solve",
    srcs = ["simple_solve.cc"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        "command_executer",
    ],
)

cc_binary(
    name = "json2binary",
    srcs = ["json2binary.cc"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
    ],
)

cc_binary(
    name = "simulator",
    srcs = ["simulator.cc",
            "command.h",
            "command.cc",
            "command_executer.h",
            "command_executer.cc",
            "command_util.h",
            "command_util.cc"],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
    ],
)

cc_binary(
    name = "skeleton",
    srcs = ["skeleton.cc",
            "command.h",
            "command.cc",
            "command_executer.h",
            "command_executer.cc",
            "command_util.h",
            "command_util.cc",
    ],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
    ],
)
