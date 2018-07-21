#include <iostream>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"

DEFINE_string(json_filename, "", "filepath of nbt json");

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_json_filename.empty()) {
    std::cerr << "need to pass --json_filename=/path/to/json" << std::endl;
    exit(1);
  }
  
  std::string jsonfile = ReadFile(FLAGS_json_filename);

  Json::CharReaderBuilder builder;
  Json::Value json;
  std::string err;
  std::istringstream ss(jsonfile);
  LOG_IF(FATAL, !Json::parseFromStream(builder, ss, &json, &err)) << err;

  std::cout << Json2Binary(json);
}
