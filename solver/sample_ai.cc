#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <deque>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "src/command_util.h"
#include "src/command_executer.h"

#include "solver/AI.h"

DEFINE_string(mdl_filename, "", "filepath of mdl");

class SampleAI : public AI {
  vvv model;

public:
  SampleAI(const vvv &model) : AI(model), model(model) {}
  ~SampleAI() override = default;
  void Run() override {}
};

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_mdl_filename.empty()) {
    std::cout << "need to pass --mdl_filename=/path/to/mdl";
    exit(1);
  }

  vvv M = ReadMDL(FLAGS_mdl_filename);
  WriteMDL(FLAGS_mdl_filename + "_dbl", M); // should be the same file
  //  OutputMDL(M);

  auto sample_ai = std::make_unique<SampleAI>(M);
  sample_ai->Run();
  sample_ai->Finalize();
}
