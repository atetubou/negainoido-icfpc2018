#include "src/base/base.h"

#include <fstream>

#include "glog/logging.h"

std::string ReadFile(absl::string_view filename) {
  // http://www.cplusplus.com/reference/istream/istream/read/
  std::ifstream is(std::string(filename), std::ifstream::binary);
  std::string buffer;
  
  LOG_IF(FATAL, !is) << "failed to read file " << filename;

  // get length of file:
  is.seekg (0, is.end);
  int length = is.tellg();
  is.seekg (0, is.beg);

  buffer.resize(length);

  // read data as a block:
  is.read(&buffer[0], length);
    
  LOG_IF(FATAL, !is) << "error: only " << is.gcount() << " could be read";

  return buffer;
}


vvv ReadMDL(absl::string_view filename) {
  std::string buffer = ReadFile(filename);
  int R = (unsigned int)buffer[0];
  int idx = 0;
  
  vvv M(R, vv(R, v(R, 0)));
  int i = 8;
  unsigned b = 0;
  for (int x=0; x<R; ++x) {
    for (int y=0; y<R; ++y) {
      for (int z=0; z<R; ++z) {
        if (i >= 8) {
          ++idx;
          b = buffer[idx];
          i = 0;
        }
        if (b & (1 << i)) {
          M[x][y][z] = 1;
        }
        i += 1;
      }
    }
  }
  return M;
}


void WriteMDL(absl::string_view filename, const vvv &M) {
	int R = (int)M.size();
  std::ofstream os(std::string(filename), std::ios::binary | std::ios::out | std::ios::trunc);

  LOG_IF(FATAL, !os) << "failed to read file " << filename;

  os.write((char*)&R, 1);
  int i = 0;
  unsigned b = 0;
  for (int x=0; x<R; ++x) {
    for (int y=0; y<R; ++y) {
      for (int z=0; z<R; ++z) {
        b = b | (M[x][y][z]<<i);
        i += 1;
        if (i >= 8) {
          os.write((char *)&b, 1);
          i = 0;
          b = 0;
        }
      }
    }
  }
}
