package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "nbt_viewer",
    srcs = ["nbt_viewer.cc"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        ":nbt_loader",
    ],
)

cc_library(
    name = "nbt_loader",
    srcs = ["nbt_loader.cc", "nbt_loader.h"],
    deps = [
        "//external:glog",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
    ],
)

cc_library(
    name = "command_util",
    srcs = ["command_util.h",
            "command_util.cc",
    ],
    copts = ["-std=c++14"],
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
            "@com_google_absl//absl/types:span",
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
    srcs = ["simulator.cc"],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        ":command_executer",
        ":nbt_loader",
    ],
)

cc_test(
    name = "command_executer_test",
    srcs = ["command_executer_test.cc"],
    copts = ["-std=c++14"],
    deps = [
        ":command_executer",
        "@com_google_googletest//:gtest_main",
    ],
)
