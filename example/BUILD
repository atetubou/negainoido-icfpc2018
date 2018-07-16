cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = ["//example/hello:hello"]
)

cc_test(
    name = "example_test",
    srcs = ["example_test.cc"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/strings",
        "@jsoncpp_git//:jsoncpp",
    ]
)
