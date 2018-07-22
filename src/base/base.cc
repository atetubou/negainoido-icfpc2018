#include "src/base/base.h"

#include <fstream>

#include "glog/logging.h"
#include <iostream>

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
  const std::string buffer = ReadFile(filename);
  int R = static_cast<unsigned char>(buffer[0]);
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
  if (i > 0) {
    os.write((char *)&b, 1);
  }
}

// Ouptut MDL file to stdout from bottom to top
void OutputMDL(const vvv &M) {
  int R = (int)M.size();

  for (int y=0; y<R; ++y) {
    for (int x=0;x<R;++x) {
      for (int z=0;z<R;++z) {
        std::cout << M[x][y][z] << " ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}

std::string encodecommand(const Json::Value& command) {
  CHECK(command.isMember("command")) << command;
  CHECK(command["command"].isString()) << command;

  const std::string command_name = command["command"].asString();

  if (command_name == "Halt") {
    return {static_cast<char>(0b11111111)};
  }

  if (command_name == "Wait") {
    return {static_cast<char>(0b11111110)};
  }

  if (command_name == "Flip") {
    return {static_cast<char>(0b11111101)};
  }

  auto getshort = [&](int dx,int dy, int dz) -> std::pair<int, int> {
    if (dx != 0) return {0b01, dx + 5};
    if (dy != 0) return {0b10, dy + 5};
    if (dz != 0) return {0b11, dz + 5};

    LOG(FATAL) << command;
    return {0, 0};
  };

  auto getlong = [&](int dx,int dy, int dz) -> std::pair<int, int> {
    if (dx != 0) return {0b01, dx + 15};
    if (dy != 0) return {0b10, dy + 15};
    if (dz != 0) return {0b11, dz + 15};

    LOG(FATAL) << command;
    return {0, 0};
  };


  if (command_name == "SMove") {
    CHECK(command.isMember("dx") && command["dx"].isInt()) << command;
    CHECK(command.isMember("dy") && command["dy"].isInt()) << command;
    CHECK(command.isMember("dz") && command["dz"].isInt()) << command;

    auto lld = getlong(command["dx"].asInt(), command["dy"].asInt(), command["dz"].asInt());
    return {static_cast<char>((lld.first << 4) | 0b0100), static_cast<char>(lld.second)};
  }

  if (command_name == "LMove") {
    CHECK(command.isMember("dx1") && command["dx1"].isInt()) << command;
    CHECK(command.isMember("dy1") && command["dy1"].isInt()) << command;
    CHECK(command.isMember("dz1") && command["dz1"].isInt()) << command;

    CHECK(command.isMember("dx2") && command["dx2"].isInt()) << command;
    CHECK(command.isMember("dy2") && command["dy2"].isInt()) << command;
    CHECK(command.isMember("dz2") && command["dz2"].isInt()) << command;

    auto sid1 = getshort(command["dx1"].asInt(), command["dy1"].asInt(), command["dz1"].asInt());
    auto sid2 = getshort(command["dx2"].asInt(), command["dy2"].asInt(), command["dz2"].asInt());

    return {
      static_cast<char>((sid2.first << 6) | (sid1.first << 4)| 0b1100),
        static_cast<char>((sid2.second << 4) | sid1.second),
        };
  }

  auto getnd = [&](int dx, int dy, int dz) -> int {
    CHECK(-1 <= dx && dx <= 1);
    CHECK(-1 <= dy && dy <= 1);
    CHECK(-1 <= dz && dz <= 1);

    CHECK(std::abs(dx) + std::abs(dy) + std::abs(dz) == 1);

    return (dx + 1) * 9 + (dy + 1) * 3 + (dz + 1);
  };

  if (command_name == "Fill") {
    CHECK(command.isMember("dx") && command["dx"].isInt()) << command;
    CHECK(command.isMember("dy") && command["dy"].isInt()) << command;
    CHECK(command.isMember("dz") && command["dz"].isInt()) << command;

    int nd = getnd(command["dx"].asInt(), command["dy"].asInt(), command["dz"].asInt());
    return {static_cast<char>((nd << 3) | 0b011)};
  }

  if (command_name == "FusionP") {
    CHECK(command.isMember("dx") && command["dx"].isInt()) << command;
    CHECK(command.isMember("dy") && command["dy"].isInt()) << command;
    CHECK(command.isMember("dz") && command["dz"].isInt()) << command;

    int nd = getnd(command["dx"].asInt(), command["dy"].asInt(), command["dz"].asInt());
    return {static_cast<char>((nd << 3) | 0b111)};
  }

  if (command_name == "FusionS") {
    CHECK(command.isMember("dx") && command["dx"].isInt()) << command;
    CHECK(command.isMember("dy") && command["dy"].isInt()) << command;
    CHECK(command.isMember("dz") && command["dz"].isInt()) << command;

    int nd = getnd(command["dx"].asInt(), command["dy"].asInt(), command["dz"].asInt());
    return {static_cast<char>((nd << 3) | 0b110)};
  }

  if (command_name == "Fission") {
    CHECK(command.isMember("dx") && command["dx"].isInt()) << command;
    CHECK(command.isMember("dy") && command["dy"].isInt()) << command;
    CHECK(command.isMember("dz") && command["dz"].isInt()) << command;

    CHECK(command.isMember("m") && command["m"].isInt()) << command;

    int nd = getnd(command["dx"].asInt(), command["dy"].asInt(), command["dz"].asInt());
    return {static_cast<char>((nd << 3) | 0b101), static_cast<char>(command["m"].asInt())};
  }

  if (command_name == "Void") {
    CHECK(command.isMember("dx") && command["dx"].isInt()) << command;
    CHECK(command.isMember("dy") && command["dy"].isInt()) << command;
    CHECK(command.isMember("dz") && command["dz"].isInt()) << command;

    int nd = getnd(command["dx"].asInt(), command["dy"].asInt(), command["dz"].asInt());
    return {static_cast<char>((nd << 3) | 0b010)};
  }

  if (command_name == "GFill") {
    CHECK(command.isMember("dx1") && command["dx1"].isInt()) << command;
    CHECK(command.isMember("dy1") && command["dy1"].isInt()) << command;
    CHECK(command.isMember("dz1") && command["dz1"].isInt()) << command;

    CHECK(command.isMember("dx2") && command["dx2"].isInt()) << command;
    CHECK(command.isMember("dy2") && command["dy2"].isInt()) << command;
    CHECK(command.isMember("dz2") && command["dz2"].isInt()) << command;

    int nd = getnd(command["dx1"].asInt(), command["dy1"].asInt(), command["dz1"].asInt());
    return {static_cast<char>((nd << 3) | 0b001),
        static_cast<char>(command["dx2"].asInt() + 30),
        static_cast<char>(command["dy2"].asInt() + 30),
        static_cast<char>(command["dz2"].asInt() + 30)};
  }

  if (command_name == "GVoid") {
    CHECK(command.isMember("dx1") && command["dx1"].isInt()) << command;
    CHECK(command.isMember("dy1") && command["dy1"].isInt()) << command;
    CHECK(command.isMember("dz1") && command["dz1"].isInt()) << command;

    CHECK(command.isMember("dx2") && command["dx2"].isInt()) << command;
    CHECK(command.isMember("dy2") && command["dy2"].isInt()) << command;
    CHECK(command.isMember("dz2") && command["dz2"].isInt()) << command;

    int nd = getnd(command["dx1"].asInt(), command["dy1"].asInt(), command["dz1"].asInt());
    return {static_cast<char>((nd << 3) | 0b000),
        static_cast<char>(command["dx2"].asInt() + 30),
        static_cast<char>(command["dy2"].asInt() + 30),
        static_cast<char>(command["dz2"].asInt() + 30)};
  }


  LOG(FATAL) << "unsupported json command " << command;
  return "";
}

std::string Json2Binary(const Json::Value& json) {
  CHECK(json.isMember("turn"));
  CHECK(json["turn"].isArray());
  std::string ret;

  for (const auto& turn : json["turn"]) {
    CHECK(turn.isArray());
    std::vector<std::pair<int, std::string>> commands;
    for (const auto& command : turn) {
      CHECK(command.isMember("bot_id") && command["bot_id"].isInt()) << command;
      commands.emplace_back(command["bot_id"].asInt(), encodecommand(command));
    }

    std::sort(commands.begin(), commands.end());
    for (const auto& c : commands) {
      ret += c.second;
    }
  }

  return ret;
}
