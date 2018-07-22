#include <iostream>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

#include "solver/square_delete.h"

DEFINE_string(mdl_filename, "", "filepath of mdl");
DEFINE_bool(flip, false, "do flip?");


int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);


  const vvv voxels = ReadMDL(FLAGS_mdl_filename);
  const Json::Value json = SquareDelete(voxels, FLAGS_flip);

  std::string nbt_content = Json2Binary(json);
  std::cout << nbt_content;
}
