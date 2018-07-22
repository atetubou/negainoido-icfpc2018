#include "gflags/gflags.h"
#include "src/nbt_loader.h"
#include "src/base/base.h"
#include "glog/logging.h"
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

DEFINE_string(assemble_nbt, "", "filepath of nbt emitted from assembler");
DEFINE_string(deassemble_nbt, "", "filepath of nbt emitted from deassembler");

vector<Json::Value> read_commands(const string &nbt_filename) {
  const string nbt_content = ReadFile(nbt_filename);
  vector<Json::Value> commands;

  int nanobot_num = 1;
  std::ostringstream ss;
  for (size_t i = 0; i < nbt_content.size();) {
    Json::Value command_json;
    i += parse_command(nbt_content, i, &nanobot_num, &ss, &command_json);
    commands.push_back(command_json);
  }
  return commands;
}
  

int main(int argc, char **argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::ostringstream ss;
  vector<Json::Value> deassemble_command_json = read_commands(FLAGS_deassemble_nbt);
  vector<Json::Value> assemble_command_json = read_commands(FLAGS_assemble_nbt);

  const string deassemble_last_command = deassemble_command_json.back()["command"].asString();
  LOG_IF(FATAL, deassemble_last_command != "Halt") << "Last command must be Halt but it was " << deassemble_last_command;

  
  deassemble_command_json.pop_back();
  deassemble_command_json.insert(deassemble_command_json.end(), assemble_command_json.begin(), assemble_command_json.end());
  
  string res = "";
  for (const auto &command: deassemble_command_json) {
    res += encodecommand(command);
  }
  cout << res;
}
