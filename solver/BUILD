package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "default",
    copts = ["-std=c++14"],
    srcs = ["default.cc"],
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
        "//src:command_executer",
    ],
)

cc_binary(
    name = "skeleton",
    srcs = ["skeleton.cc"],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "@com_google_absl//absl/types:span",
        "//src/base",
        "//src:command_executer",
    ],
)

cc_binary(
    name = "sample_ai",
    srcs = ["sample_ai.cc",
            "AI.h",
           ],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        "//src:command_executer",
    ],
)
