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
