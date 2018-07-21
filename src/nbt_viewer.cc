#include <fstream>
#include <iostream>
#include <string>
#include <memory>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

#include "src/base/base.h"
#include "src/nbt_loader.h"



/*

  bazel run //src:nbt_viewer -- --json --nbt_filename=/path/to/nbt_file

*/


DEFINE_string(nbt_filename, "", "filepath of nbt");
DEFINE_bool(json, false, "output command in json if --json is set");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  
  std::string nbt_content = ReadFile(FLAGS_nbt_filename);

  int nanobot_num = 1;

  std::ostringstream ss;
  Json::Value json;  

  for (size_t i = 0; i < nbt_content.size();) {
    int cur_nanobot_num = nanobot_num;
    Json::Value turn;

    for (int cur_nanobot_idx = 1; cur_nanobot_idx <= cur_nanobot_num; ++cur_nanobot_idx) {
      Json::Value command;
      command["bot_id"] = cur_nanobot_idx;

      ss <<"bot id " << cur_nanobot_idx << ": ";

      int n = parse_command(nbt_content, i, &nanobot_num, &ss, &command);
      i += n;

      turn.append(std::move(command));
    }

    json["turn"].append(std::move(turn));
  }


  if (FLAGS_json) {
    Json::StreamWriterBuilder builder;
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(json, &std::cout);
  } else {
    std::cout << ss.str() << std::endl;
  }
}
