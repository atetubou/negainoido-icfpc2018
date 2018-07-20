#include <fstream>
#include <iostream>
#include <string>

#include "gflags/gflags.h"
#include "glog/logging.h"

/*

  bazel run //src:nbt_viewer -- --ndl_filename=/path/to/ndl_file

*/


DEFINE_string(ndl_filename,"", "filepath of ndl");

std::string ReadFile(const std::string& name) {
  // http://www.cplusplus.com/reference/istream/istream/read/
  std::ifstream is(FLAGS_ndl_filename, std::ifstream::binary);
  std::string buffer;
  
  LOG_IF(FATAL, !is) << "failed to read file " << FLAGS_ndl_filename;

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

const char kHALT = 0b11111111;
const char kWAIT = 0b11111110;
const char kFLIP = 0b11111101;

bool getshort(int a, int i, int* dx, int* dy, int* dz) {
  *dx = *dy = *dz = 0;
  
  if (a == 0b01) *dx = i - 5;
  else if (a == 0b10) *dy = i - 5;
  else if (a == 0b11) *dz = i - 5;
  else return false;

  return true;
}

bool getlong(int a, int i, int* dx, int* dy, int* dz) {
  *dx = *dy = *dz = 0;
  
  if (a == 0b01) *dx = i - 15;
  else if (a == 0b10) *dy = i - 15;
  else if (a == 0b11) *dz = i - 15;
  else return false;

  return true;
}

bool getnearcoordinate(int nd, int* dx, int* dy, int* dz) {
  for (*dx = -1; *dx <= 1; ++*dx) {
    for (*dy = -1; *dy <= 1; ++*dy) {
      for (*dz = -1; *dz <= 1; ++*dz) {
        if ((*dx + 1) * 9 + (*dy + 1) * 3 + (*dz + 1) == nd) {
          return true;
        }
      }
    }
  }

  return false;
}

std::string binary(int x) {
  std::string a;
  for (int i = 7; i >= 0; --i) {
    a += ((x >> i) & 1) + '0';
  }
  return a;
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  
  std::string nbt_content = ReadFile(FLAGS_ndl_filename);

  int nanobot_num = 1;

  for (size_t i = 0; i < nbt_content.size();) {
    int cur_nanobot_num = nanobot_num;
    for (int cur_nanobot_idx = 0; cur_nanobot_idx < cur_nanobot_num; ++cur_nanobot_idx) {
      std::cout <<"bot id " << cur_nanobot_idx << ": ";

      switch (nbt_content[i]) {
      case kHALT:
        std::cout << "Halt" << std::endl;
        ++i;
        continue;
      case kWAIT:
        std::cout << "Wait" << std::endl;
        ++i;
        continue;
      case kFLIP:
        std::cout << "Flip" << std::endl;
        ++i;
        continue;
      default:
        //
        break;
      }

      if ((nbt_content[i] & 0b1111) == 0b0100) {
        // SMove
        int llda = (nbt_content[i] >> 4) & 0b11;
        int lldi = nbt_content[i + 1] & 0b11111;
        int dx = 0, dy = 0, dz = 0;
        LOG_IF(FATAL, !getshort(llda, lldi, &dx, &dy, &dz))
               << "encoding error";

        std::cout << "SMove <" << dx << ", " << dy << ", " << dz << ">" << std::endl;
        i += 2;
        continue;
      }

      if ((nbt_content[i] & 0b1111) == 0b1100) {
        // LMove
        int sid2a = (nbt_content[i] >> 6) & 0b11;
        int sid1a = (nbt_content[i] >> 4) & 0b11;
        int sid2i = (nbt_content[i + 1] >> 4) & 0b1111;
        int sid1i = nbt_content[i + 1] & 0b1111;
      
        int dx1 = 0, dy1 = 0, dz1 = 0;
        int dx2 = 0, dy2 = 0, dz2 = 0;
        LOG_IF(FATAL, !getlong(sid1a, sid1i, &dx1, &dy1, &dz1))
          << "encoding error";

        LOG_IF(FATAL, !getlong(sid2a, sid2i, &dx2, &dy2, &dz2))
          << "encoding error" << std::endl;
        
        std::cout << "LMove <" << dx1 << ", " << dy1 << ", " << dz1 << "> <" 
                  << dx2 << ", " << dy2 << ", " << dz2 << ">"
                  << std::endl;
        i += 2;
        continue;
      }
    
      if ((nbt_content[i] & 0b111) == 0b111) {
        // FusionP
        --nanobot_num;
        int nd = (nbt_content[i] >> 3) & 0b11111;
        int dx, dy, dz;

        LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
          << "encoding error";
      
        std::cout << "FusionP <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;
        ++i;
        continue;
      }

      if ((nbt_content[i] & 0b111) == 0b110) {
        // FusionS
        --nanobot_num;
        int nd = (nbt_content[i] >> 3) & 0b11111;
        int dx, dy, dz;
        LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
          << "encoding error" << std::endl;

        std::cout << "FusionS <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;
      
        ++i;
        continue;
      }

      if ((nbt_content[i] & 0b111) == 0b101) {
        // Fission
        ++nanobot_num;
        int nd = (nbt_content[i] >> 3) & 0b11111;
        int m = nbt_content[i + 1];
        int dx, dy, dz;
        LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
          << "encoding error";

        std::cout << "Fission <" << dx << ", " << dy << ", " << dz << "> "  << m << std::endl;
        i += 2;
        continue;
      }

      if ((nbt_content[i] & 0b111) == 0b011) {
        // Fill
        int nd = (nbt_content[i] >> 3) & 0b11111;
        int dx, dy, dz;
        LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
          << "encoding error" << std::endl;

        std::cout << "Fill <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;
        i += 1;
        continue;
      }

      LOG(FATAL) << "unknown command? " << binary(nbt_content[i]);
    }
  }
}
