/*
 * bazel run //src:nbt_viewer -- --nbt_filename=$HOME/Downloads/c/dfltTracesL/LA001.nbt --json | jq . > ~/Downloads/c/dfltTracesL/LA001.json
 * bazel run //src:simulator -- --verbose --mdl_filename $HOME/Downloads/c/problemsL/LA001_tgt.mdl --nbt_json $HOME/Downloads/c/dfltTracesL/LA001.json
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

#include "src/base/base.h"
#include "src/command.h"
#include "src/command_executer.h"

DEFINE_string(mdl_filename, "", "filepath of mdl (.mdl)");
DEFINE_string(nbt_json, "", "file path of nbt (.json)");
DEFINE_bool(verbose, false, "verbose");

int main(int argc, char* argv[]) {

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_mdl_filename.empty() or FLAGS_nbt_json.empty()) {
    exit(1);
  }

  vvv M = ReadMDL(FLAGS_mdl_filename);
  int R = M.size();

  std::ifstream is(std::string(FLAGS_nbt_json), std::ifstream::binary);
  Json::Value root;
  is >> root;

  auto ce = std::make_unique<CommandExecuter>(R, false);

  Json::Value turns = root["turn"];
  int n = turns.size();
  for (int i = 0; i < n; ++i) {
      std::vector<Command> commands;
      int m = turns[i].size();
      if (FLAGS_verbose) {
          std::cout << turns[i] << std::endl;
      }
      for (int j = 0; j < m; ++j) {
          Command c = Command::JsonToCommand(turns[i][j]);
          commands.push_back(c);
      }
      ce->Execute(commands);
      if (FLAGS_verbose) {
          std::cout << "Energy: " << ce->GetSystemStatus().energy << std::endl;
      }
  }
  std::cout << ce->GetSystemStatus().energy << std::endl;

  return 0;
}
