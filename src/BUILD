cc_binary(
    name = "nbt_viewer",
    srcs = ["nbt_viewer.cc"],
    deps = [
        "//external:glog",
        "@com_github_gflags_gflags//:gflags",
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_library(
    name = "command_executer",
    srcs = ["command_executer.h",
            "command_executer.cc",
    ],
    copts = ["-std=c++14"],
    deps = ["//external:glog"],
)
