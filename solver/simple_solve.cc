#include <vector>
#include <iostream>
#include <utility>

#include "json/json.h"
#include "gflags/gflags.h"

#include "src/base/base.h"
#include "src/command_util.h"
#include "src/command.h"
#include "simple_solve.h"

using namespace std;

DEFINE_string(mdl_filename, "", "filepath of mdl");

void flush_commands(vector<Command> &results) {
  Json::Value json;
  for (const auto& c : results) {
    std::vector<Command> commands = {c};
    if (c.type == Command::LMOVE) {
    }
    json["turn"].append(Command::CommandsToJson(commands));
  }

  cout << Json2Binary(json);
  results.clear();
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const vvv voxels = ReadMDL(FLAGS_mdl_filename);

  vector<Command> result_buff = SimpleSolve(voxels);
  flush_commands(result_buff);

  return 0;
}
