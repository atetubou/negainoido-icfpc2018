#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include "src/base/base.h"
#include "gflags/gflags.h"
using namespace std;

template <typename T> ostream &operator<<(ostream &out, const vector<T> &v) {
  out << "[";
  for (size_t i = 0; i < v.size(); i++) {
    if (i > 0) out << ", ";
    out << v[i];
  }
  out << "]";
  return out;
}

// Example
//     ./bazel-bin/src/model_sample/sample_generator --mdltxtdir src/model_sample/examples --mdlbindir model_examples
// it dies when model_examples doesn't exist

DEFINE_string(mdltxtdir, "", "");
DEFINE_string(mdlbindir, "", "");
int main(int argc, char **argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(FLAGS_mdltxtdir.c_str())) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL) {
      if (ent->d_type == DT_REG) {
        string input_file = FLAGS_mdltxtdir + "/" + ent->d_name;
        string output_file = FLAGS_mdlbindir + "/" + ent->d_name + ".mdl";
        cout << "Convert " << input_file << " into " << output_file << endl;

        ifstream ifs(input_file);
        int R;
        ifs >> R;
        vvv M(R, vv(R, v(R)));
        char c;
        for (int x = 0; x < R; x++) {
          for (int y = 0; y < R; y++) {
            for (int z = 0; z < R; z++) {
              ifs >> c;
              M[x][y][z] = c - '0';
            }
          }
        }
        WriteMDL(output_file, M);
      }
    }
    closedir (dir);
  } else {
    /* could not open directory */
    perror ("");
    return EXIT_FAILURE;
  }
}
