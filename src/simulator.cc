/*
 * Usage:
 * bazel run //src:simulator -- --verbose --mdl_filename $HOME/Downloads/c/problemsL/LA001_tgt.mdl --nbt_filename $HOME/Downloads/c/dfltTracesL/LA001.nbt
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

#include "src/base/base.h"
#include "src/base/flags.h"
#include "src/command.h"
#include "src/command_executer.h"
#include "src/nbt_loader.h"

DEFINE_string(nbt_filename, "", "filepath.nbt");
DEFINE_bool(from_json, false, ".nbt => .json");
DEFINE_bool(verbose, false, "verbose mode; can use when --from_json");


vvv empty(int R) {
    return vvv(R, vv(R, v(R, 0)));
}


int main(int argc, char* argv[]) {

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_nbt_filename.empty()) {
    std::cerr << "Specify --nbt_filename\n";
    exit(1);
  }
  if (FLAGS_src_filename == "-" and FLAGS_tgt_filename == "-") {
    std::cerr << "ERROR: - and -\n";
    exit(1);
  }

  LOG(INFO) << "load models";
  std::unique_ptr<CommandExecuter> ce;
  vvv M_tgt;
  int R;

  if (FLAGS_src_filename != "-" and FLAGS_tgt_filename == "-") {
      LOG(INFO) << "Disassembly";
      vvv M_src = ReadMDL(FLAGS_src_filename);
      R = M_src.size();
      M_tgt = empty(R);
      ce = std::make_unique<CommandExecuter>(M_src, false);
  } else if (FLAGS_src_filename == "-" and FLAGS_tgt_filename != "-") {
      LOG(INFO) << "Assembly";
      M_tgt = ReadMDL(FLAGS_tgt_filename);
      R = M_tgt.size();
      ce = std::make_unique<CommandExecuter>(R, false);
  } else {
      LOG(INFO) << "Reassemble";
      vvv M_src = ReadMDL(FLAGS_src_filename);
      M_tgt = ReadMDL(FLAGS_tgt_filename);
      R = M_tgt.size();
      ce = std::make_unique<CommandExecuter>(M_src, false);
      CHECK((int)M_src.size() == R);
  }

  LOG(INFO) << "start execution";

  Json::Value turns;
  std::string nbt_content;

  if (FLAGS_from_json) {
    LOG(INFO) << "read from json";
    std::ifstream is(std::string(FLAGS_nbt_filename), std::ifstream::binary);
    Json::Value root; is >> root;
    turns = std::move(root["turn"]);
  } else {
    LOG(INFO) << "read from binary";
    nbt_content = ReadFile(FLAGS_nbt_filename);
  }
  LOG(INFO) << "done read";

  if (FLAGS_from_json) {

    int n = turns.size();
    LOG(INFO) << "JSON length: " << n;
    for (int i = 0; i < n; ++i) {
      std::vector<Command> commands;
      int m = turns[i].size();
      if (FLAGS_verbose) {
        LOG(INFO) << turns[i];
      }
      for (int j = 0; j < m; ++j) {
        Command c = Command::JsonToCommand(turns[i][j]);
        commands.push_back(c);
      }
      ce->Execute(commands);
      if (FLAGS_verbose) {
        LOG(INFO) << "Energy: " << ce->GetSystemStatus().energy;
        LOG(INFO) << "Harmonics: " << ce->GetSystemStatus().harmonics;
      }
    }

  } else {

    std::ostringstream ss;

    int nanobot_num = 1;
    for (size_t i = 0; i < nbt_content.size();) {
        int cur_nanobot_num = nanobot_num;
        Json::Value turn;
        std::vector<Command> commands;
        for (int cur_nanobot_idx = 1; cur_nanobot_idx <= cur_nanobot_num; ++cur_nanobot_idx) {
            Json::Value command_json;
            command_json["bot_id"] = cur_nanobot_idx;
            int n = parse_command(nbt_content, i, &nanobot_num, &ss, &command_json);
            i += n;
            Command command =  Command::JsonToCommand(command_json);
            if (FLAGS_verbose) {
                LOG(INFO) << command_json;
            }
            commands.push_back(command);
        }
        ce->Execute(commands);
        if (FLAGS_verbose) {
            LOG(INFO) << "Energy: " << ce->GetSystemStatus().energy;
            LOG(INFO) << "Harmonics: " << ce->GetSystemStatus().harmonics;
        }
    }
  }

  LOG(INFO) << "done command execute";
  LOG(INFO) << "active bots num: " << ce->GetActiveBotsNum();
  LOG(INFO) << "Harmonics: " << ce->GetSystemStatus().harmonics;

  // Halt?
  if (ce->GetActiveBotsNum() > 0) {
      std::cout << -1 << std::endl;
      exit(1);
  }

  // Low?
  if (ce->GetSystemStatus().harmonics != LOW) {
      std::cout << -2 << std::endl;
      exit(1);
  }

  // N == M_tgt?
  auto&N = ce->GetSystemStatus().matrix;
  for (int x = 0; x < R; ++x) {
    for (int y = 0; y < R; ++y) {
      for (int z = 0; z < R; ++z) {
        if (M_tgt[x][y][z] != N[x][y][z]) {
          std::cout << -3 << std::endl;
          exit(1);
        }
      }
    }
  }

  std::cout << ce->GetSystemStatus().energy << std::endl;
  return 0;
}
