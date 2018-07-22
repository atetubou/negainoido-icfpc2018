#ifndef SRC_BASE_FLAGS_H_
#define SRC_BASE_FLAGS_H_

#include "gflags/gflags.h"

DEFINE_string(src_filename, "-", "filename for src mdl file e.g. shared/FD001_src.mdl");
DEFINE_string(tgt_filename, "-", "filename for src mdl file e.g. shared/FA001_src.mdl");

#endif  // SRC_BASE_FLAGS_H_
