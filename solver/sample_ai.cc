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
#include "src/base/flags.h"
#include "src/command_util.h"
#include "src/command_executer.h"

#include "solver/AI.h"

class SampleAI : public AI {
  vvv model;

public:
  SampleAI(const vvv &model) : AI(model), model(model) {}
  ~SampleAI() override = default;
  void Run() override {}
};

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_tgt_filename.empty()) {
    std::cout << "need to pass --tgt_filename=/path/to/mdl";
    exit(1);
  }

  vvv M = ReadMDL(FLAGS_tgt_filename);
  WriteMDL(FLAGS_tgt_filename + "_dbl", M); // should be the same file
  //  OutputMDL(M);

  auto sample_ai = std::make_unique<SampleAI>(M);
  sample_ai->Run();
  sample_ai->Finalize();
}
