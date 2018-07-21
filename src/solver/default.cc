#include <iostream>
#include <fstream>
#include <regex>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"


DEFINE_string(mdl_filename, "", "filepath of mdl");
DEFINE_string(nbt_dir, "/home/negainoido/icfpc2018/shared", "dir of mdl");
DEFINE_bool(json, false, "output command in json if --json is set");

void cat(absl::string_view filename) {
  std::ifstream is(std::string(filename), std::ifstream::binary);
  if (is.good()) {
      std::cout << is.rdbuf();
  } else {
      std::cout << "cannot open " << filename << std::endl;
      exit(1);
  }
}

int main(int argc, char* argv[]) {

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_mdl_filename.empty()) {
    std::cout << "need to pass --mdl_filename=/path/to/mdl\n";
    exit(1);
  }

  std::regex re(R"(.*(LA\d\d\d)_tgt.mdl$)");
  std::smatch match;

  if (regex_match(FLAGS_mdl_filename, match, re)) {
      std::stringstream ss;
      ss << FLAGS_nbt_dir << '/' << match[1] << ".nbt";
      std::string nbt_filename = ss.str();
      cat(nbt_filename);
  } else {
    std::cout << "invalid mdl_filename: " << FLAGS_mdl_filename << std::endl;
    exit(1);
  }

}
