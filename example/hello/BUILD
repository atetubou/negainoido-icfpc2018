package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hello",
    srcs = ["hello.cc"],
    hdrs = ["hello.h"],
    copts = ["-std=c++14"]
)

cc_test(
    name = "hello_test",
    srcs = ["hello_test.cc"],
    deps = [
        ":hello",
        "@com_google_googletest//:gtest_main",
    ],
)
