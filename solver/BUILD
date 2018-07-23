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
           ],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        "//src:command_executer",
        "//src/base:flags",
        ":ai",
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
    srcs = ["square_deleter.cc", "AI.h"],
    copts = ["-std=c++14"],
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
    name = "crimea_ai",
    copts = ["-std=c++14"],
    srcs = ["crimea_ai.h", "oscar.cc", "oscar.h", "crimea_ai.cc", "AI.h"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "@com_google_absl//absl/types:span",
        "//src:command_executer",
        "//src/base",
    ],
)

cc_library(
    name = "crimea_ai_debug",
    copts = ["-std=c++14", "-DDCHECK_ALWAYS_ON"],
    srcs = ["crimea_ai.h", "oscar.cc", "oscar.h", "crimea_ai.cc", "AI.h"],
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
           ],
    copts = ["-std=c++14"],
    deps = [
        "simple_solve_lib",
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        "//src:command_executer",
        "crimea_ai",
        "//src/base:flags",
        ":ai",
    ],
)

cc_binary(
    name = "oscar_ai_debug",
    srcs = ["oscar_ai.cc"],
    copts = ["-std=c++14", "-DDCHECK_ALWAYS_ON"],
    deps = [
        "simple_solve_lib",
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        "//src:command_executer",
        "crimea_ai_debug",
        "//src/base:flags",
        ":ai",
    ],
)

cc_binary(
    name = "kevin_ai",
    srcs = ["kevin_ai.cc" ],
    copts = ["-std=c++14"],
    deps = [
        "simple_solve_lib",
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        "//src:command_executer",
        "crimea_ai",
        "//src/base:flags",
        ":ai",
    ],
)

cc_binary(
    name = "kevin_ai_debug",
    srcs = ["kevin_ai.cc" ],
    copts = ["-std=c++14", "-DDCHECK_ALWAYS_ON"],
    deps = [
        "simple_solve_lib",
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "//src/base",
        "//src:command_executer",
        "crimea_ai_debug",
        "//src/base:flags",
        ":ai",
    ],
)

cc_binary(
    name = "duskin_ai",
    srcs = ["duskin_ai.cc",
            "duskin_lib.h",
            "duskin_lib.cc",
           ],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "//src/base",
        "//src:command_executer",
        "//src/base:flags",
        ":ai",
    ],
)

cc_binary(
    name = "smarter_duskin_ai",
    srcs = ["smarter_duskin_ai.cc",
            "duskin_lib.h",
            "duskin_lib.cc",
           ],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "//src/base",
        "//src:command_executer",
        "//src/base:flags",
        ":ai",
    ],
)

cc_library(
    name = "ai",
    srcs = [
        "AI.h",
        "AI.cc",
    ],
    copts = ["-std=c++14"],
    deps =[
        "//src/base",
        "//src:command_executer",
    ],
)

cc_binary(
    name = "cube_eraser",
    srcs = ["cube_eraser.cc"],
    copts = ["-std=c++14"],
    deps = [
        "@com_google_absl//absl/types:span",
        "//external:glog",
        "//src/base",
        "//src:command_executer",
        "//src/base:flags",
        ":ai",
    ],
)

cc_binary(
    name = "udon_ai",
    srcs = ["udon_ai.cc",
           ],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "@com_google_absl//absl/types:span",
        "//src/base",
        "//src:command_executer",
        "//src/base:flags",
        ":ai",
    ],
)

cc_binary(
    name = "itigo",
    srcs = ["itigo.cc"],
    copts = ["-std=c++14"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
        "@com_google_absl//absl/types:span",
        "//src/base",
        "//src:command_executer",
        "//src/base:flags",
        ":ai",
    ],
)
