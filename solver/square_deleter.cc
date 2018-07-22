#include <iostream>
#include <memory>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

#include "solver/square_delete.h"
#include "src/base/flags.h"

DEFINE_bool(flip, true, "do flip?");
DEFINE_bool(json, false, "output json");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);


  const vvv voxels = ReadMDL(FLAGS_src_filename);
  const Json::Value json = SquareDelete(voxels, FLAGS_flip);

  if (FLAGS_json) {
    Json::StreamWriterBuilder builder;
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(json, &std::cout);
    std::cout << std::endl;
  } else {
    std::string nbt_content = Json2Binary(json);
    std::cout << nbt_content;
  }
}
