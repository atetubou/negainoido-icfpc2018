#include <iostream>
#include <fstream>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"

DEFINE_string(mdl_filename, "", "filepath of mdl");

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (!FLAGS_mdl_filename.empty()) {
    std::cout << "need to pass --mdl_filename=/path/to/mdl";
    exit(1);
  }

  vvv M = ReadMDL(FLAGS_mdl_filename);
  int R = M.size();
  std::cout << R << std::endl;

  for (int x=0;x<R;++x) {
    for (int y=0; y<R; ++y) {
      for (int z=0;z<R;++z) {
        std::cout << M[x][y][z];
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}
