licenses(["unencumbered"])  # Public Domain or MIT

exports_files(["LICENSE"])

cc_import(
    name = "z3",
    hdrs = [
        "include/z3++.h",
        "include/z3_algebraic.h",
        "include/z3_ast_containers.h", 
        "include/z3_fpa.h",
        "include/z3_macros.h",
        "include/z3_polynomial.h",
        "include/z3_spacer.h",
        "include/z3.h",
        "include/z3_api.h",
        "include/z3_fixedpoint.h",
        "include/z3_interp.h",
        "include/z3_optimization.h",
        "include/z3_rcf.h",
        "include/z3_v1.h"
    ],
    static_library = select({ 
              "@bazel_tools//src/conditions:darwin": "libz3-darwin.a",
              "//conditions:default": "libz3-x64-ubuntu.a"
            }),
    visibility = ["//visibility:public"],
)

