workspace(name = "icfpc2018")

# だいたいここからとってきた
# https://github.com/tensorflow/tensorflow/blob/master/tensorflow/workspace.bzl

# GoogleTest/GoogleMock framework. Used by most unit-tests.
http_archive(
    name = "com_google_googletest",
    urls = [
        "https://mirror.bazel.build/github.com/google/googletest/archive/9816b96a6ddc0430671693df90192bbee57108b6.zip",
        "https://github.com/google/googletest/archive/9816b96a6ddc0430671693df90192bbee57108b6.zip",
    ],
    sha256 = "9cbca84c4256bed17df2c8f4d00c912c19d247c11c9ba6647cd6dd5b5c996b8d",
    strip_prefix = "googletest-9816b96a6ddc0430671693df90192bbee57108b6",
)

http_archive(
    name = "com_google_absl",
    urls = [
        "https://mirror.bazel.build/github.com/abseil/abseil-cpp/archive/9613678332c976568272c8f4a78631a29159271d.tar.gz",
        "https://github.com/abseil/abseil-cpp/archive/9613678332c976568272c8f4a78631a29159271d.tar.gz",
    ],
    sha256 = "1273a1434ced93bc3e703a48c5dced058c95e995c8c009e9bdcb24a69e2180e9",
    strip_prefix = "abseil-cpp-9613678332c976568272c8f4a78631a29159271d",
)

http_archive(
    name = "com_github_gflags_gflags",
    urls = [
        "https://mirror.bazel.build/github.com/gflags/gflags/archive/v2.2.1.tar.gz",
        "https://github.com/gflags/gflags/archive/v2.2.1.tar.gz",
    ],
    sha256 = "ae27cdbcd6a2f935baa78e4f21f675649271634c092b1be01469440495609d0e",
    strip_prefix = "gflags-2.2.1",
)

new_http_archive(
    name = "jsoncpp_git",
    urls = [
        "https://mirror.bazel.build/github.com/open-source-parsers/jsoncpp/archive/1.8.4.tar.gz",
        "https://github.com/open-source-parsers/jsoncpp/archive/1.8.4.tar.gz",
    ],
    sha256 = "c49deac9e0933bcb7044f08516861a2d560988540b23de2ac1ad443b219afdb6",
    strip_prefix = "jsoncpp-1.8.4",
    build_file = "third_party/jsoncpp.BUILD",
)

git_repository(
    name = "com_github_google_glog",
    remote = "https://github.com/google/glog.git",
    commit = "028d37889a1e80e8a07da1b8945ac706259e5fd8",
)

bind(
    name = "glog",
    actual = "@com_github_google_glog//:glog",
)
