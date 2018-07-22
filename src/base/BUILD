package(default_visibility = ["//visibility:public"])

cc_library(
    name = "base",
    hdrs = ["base.h"],
    srcs = ["base.cc"],
    deps = [
        "//external:glog",
        "@com_google_absl//absl/strings",
        "@jsoncpp_git//:jsoncpp",
    ]
)

cc_library(
    name = "flags",
    hdrs = ["flags.h"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
    ]
)
