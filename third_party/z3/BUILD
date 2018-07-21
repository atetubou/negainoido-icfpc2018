licenses(["unencumbered"])  # Public Domain or MIT

exports_files(["LICENSE"])

cc_library(
    name = "z3",
    hdrs = [
        "z3++.h",
        "z3_algebraic.h",
        "z3_ast_containers.h", 
        "z3_fpa.h",
        "z3_macros.h",
        "z3_polynomial.h",
        "z3_spacer.h",
        "z3.h",
        "z3_api.h",
        "z3_fixedpoint.h",
        "z3_interp.h",
        "z3_optimization.h",
        "z3_rcf.h",
        "z3_v1.h"
    ],
    srcs = select({ 
              "@bazel_tools//src/conditions:darwin": ["libz3.dylib"],
              "//conditions:default": ["libz3.so"]
            }),
    visibility = ["//visibility:public"],
)

