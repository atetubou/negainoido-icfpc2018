cc_binary(
    name = "nbt_viewer",
    srcs = ["nbt_viewer.cc"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "//external:glog",
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
