#include <fstream>
#include <iostream>
#include <string>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

/*

  bazel run //src:nbt_viewer -- --json --ndl_filename=/path/to/ndl_file

*/


DEFINE_string(ndl_filename,"", "filepath of ndl");
DEFINE_bool(json, false, "output command in json if --json is set");

std::string ReadFile(const std::string& name) {
  // http://www.cplusplus.com/reference/istream/istream/read/
  std::ifstream is(name, std::ifstream::binary);
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

  std::ostringstream ss;
  Json::Value json;  

  for (size_t i = 0; i < nbt_content.size();) {
    int cur_nanobot_num = nanobot_num;
    Json::Value turn;

    for (int cur_nanobot_idx = 1; cur_nanobot_idx <= cur_nanobot_num; ++cur_nanobot_idx) {
      Json::Value command;
      command["bot_id"] = cur_nanobot_idx;

      ss <<"bot id " << cur_nanobot_idx << ": ";

      switch (nbt_content[i]) {
      case kHALT:
        ss << "Halt" << std::endl;
        command["command"] = "Halt";
        turn.append(command);
        ++i;
        continue;
      case kWAIT:
        ss << "Wait" << std::endl;
        command["command"] = "Wait";
        turn.append(command);
        ++i;
        continue;
      case kFLIP:
        ss << "Flip" << std::endl;
        command["command"] = "Flip";
        turn.append(command);
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
        LOG_IF(FATAL, !getlong(llda, lldi, &dx, &dy, &dz))
               << "encoding error";

        Json::Value smove;
        smove["dx"] = dx;
        smove["dy"] = dy;
        smove["dz"] = dz;
        command["command"]["SMove"] = std::move(smove);
        turn.append(command);
        ss << "SMove <" << dx << ", " << dy << ", " << dz << ">" << std::endl;
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
        LOG_IF(FATAL, !getshort(sid1a, sid1i, &dx1, &dy1, &dz1))
          << "encoding error";

        LOG_IF(FATAL, !getshort(sid2a, sid2i, &dx2, &dy2, &dz2))
          << "encoding error" << std::endl;

        Json::Value lmove;
        lmove["dx1"] = dx1;
        lmove["dy1"] = dy1;
        lmove["dz1"] = dz1;
        lmove["dx2"] = dx2;
        lmove["dy2"] = dy2;
        lmove["dz2"] = dz2;
        command["command"]["LMove"] = std::move(lmove);        
        turn.append(command);
        ss << "LMove <" << dx1 << ", " << dy1 << ", " << dz1 << "> <" 
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
      
        ss << "FusionP <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;
        ++i;

        Json::Value fusionp;
        fusionp["dx"] = dx;
        fusionp["dy"] = dy;
        fusionp["dz"] = dz;
        command["command"]["FusionP"] = std::move(fusionp);        
        turn.append(command);

        continue;
      }

      if ((nbt_content[i] & 0b111) == 0b110) {
        // FusionS
        --nanobot_num;
        int nd = (nbt_content[i] >> 3) & 0b11111;
        int dx, dy, dz;
        LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
          << "encoding error" << std::endl;

        ss << "FusionS <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;
      
        ++i;

        Json::Value fusions;
        fusions["dx"] = dx;
        fusions["dy"] = dy;
        fusions["dz"] = dz;
        command["command"]["FusionS"] = std::move(fusions);
        turn.append(command);

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

        ss << "Fission <" << dx << ", " << dy << ", " << dz << "> "  << m << std::endl;

        Json::Value fission;
        fission["dx"] = dx;
        fission["dy"] = dy;
        fission["dz"] = dz;
        command["command"]["Fission"] = std::move(fission);
        turn.append(command);

        i += 2;
        continue;
      }

      if ((nbt_content[i] & 0b111) == 0b011) {
        // Fill
        int nd = (nbt_content[i] >> 3) & 0b11111;
        int dx, dy, dz;
        LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
          << "encoding error" << std::endl;

        ss << "Fill <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;
        i += 1;

        Json::Value fill;
        fill["dx"] = dx;
        fill["dy"] = dy;
        fill["dz"] = dz;
        command["command"]["Fill"] = std::move(fill);
        turn.append(command);

        continue;
      }

      LOG(FATAL) << "unknown command? " << binary(nbt_content[i]);
    }

    json["turn"].append(std::move(turn));
  }


  if (FLAGS_json) {
    std::cout << json << std::endl;
  } else {
    std::cout << ss.str() << std::endl;
  }
}
