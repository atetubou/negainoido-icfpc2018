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
        "//src/base:flags",
    ],
)

cc_library(
    name = "simple_solve_lib",
    copts = ["-std=c++14"],
    srcs = ["simple_solve_lib.cc", "simple_solve.h"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",  
        "@jsoncpp_git//:jsoncpp",
        "@com_google_absl//absl/types:span",
        "//src:command_executer",
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
        "simple_solve_lib",
        "//src/base:flags",
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
        "//src/base:flags",
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
        "//src/base:flags",
    ],
)

cc_library(
    name = "square_delete",
    srcs = [
        "square_delete.cc",
        "square_delete.h",
    ],
    deps = [
        "//external:glog",
        "@jsoncpp_git//:jsoncpp",
        "@com_google_absl//absl/types:span",
        "//src/base",
        "//src:command_executer",
    ],
)

cc_binary(
    name = "square_deleter",
    srcs = [ "square_deleter.cc"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",  
        "@jsoncpp_git//:jsoncpp",
        "@com_google_absl//absl/types:span",
        "//src/base",
        "square_delete",
        "//src/base:flags",
    ],
)

cc_library(
    name = "oscar",
    copts = ["-std=c++14"],
    srcs = ["oscar.cc", "oscar.h"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",  
        "@jsoncpp_git//:jsoncpp",
        "@com_google_absl//absl/types:span",
        "//src:command_executer",
        "//src/base",
    ],
)

cc_binary(
    name = "oscar_ai",
    srcs = ["oscar_ai.cc",
            "AI.h",
           ],
    copts = ["-std=c++14"],
    deps = [
        "simple_solve_lib",
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        "//src:command_executer",
        "oscar",
        "//src/base:flags",
    ],
)
