#include <iostream>

#include "gflags/gflags.h"
#include "json/json.h"

#include "absl/types/span.h"

#include "src/base/base.h"
#include "src/command.h"

DEFINE_string(mdl_filename, "", "filepath of mdl");

Json::Value SMove(int bot_id, int dx, int dy, int dz) {
  Json::Value json;
  json["Command"] = "SMoeve";
  json["bot_id"] = bot_id;
  json["dx"] = dx;
  json["dy"] = dy;
  json["dz"] = dz;
  return json;
}

Json::Value FusionP(int bot_id, int dx, int dy, int dz) {
  Json::Value json;
  json["Command"] = "FusionP";
  json["bot_id"] = bot_id;
  json["dx"] = dx;
  json["dy"] = dy;
  json["dz"] = dz;
  return json;
}

Json::Value FusionS(int bot_id, int dx, int dy, int dz) {
  Json::Value json;
  json["Command"] = "FusionS";
  json["bot_id"] = bot_id;
  json["dx"] = dx;
  json["dy"] = dy;
  json["dz"] = dz;
  return json;
}


Json::Value Wait(int bot_id) {
  Json::Value json;
  json["Command"] = "Wait";
  json["bot_id"] = bot_id;
  return json;
}

Json::Value Halt(int bot_id) {
  Json::Value json;
  json["Command"] = "Halt";
  json["bot_id"] = bot_id;
  return json;
}

Json::Value Flip(int bot_id) {
  Json::Value json;
  json["Command"] = "Flip";
  json["bot_id"] = bot_id;
  return json;
}

Json::Value Fission(int bot_id, int dx, int dy, int dz, int m) {
  Json::Value json;
  json["Command"] = "Fission";
  json["bot_id"] = bot_id;
  json["dx"] = dx;
  json["dy"] = dy;
  json["dz"] = dz;
  json["m"] = m;
  return json;
}

Json::Value ToArray(absl::Span<const Json::Value> values) {
  Json::Value json;
  for (const auto& v : values) {
    json.append(v);
  }
  return json;
}

Json::Value GVoid(int bot_id, int dx1, int dy1, int dz1,
                  int dx2, int dy2, int dz2) {
  Json::Value json;
  json["Command"] = "GVoid";
  json["bot_id"] = bot_id;

  // nd
  json["dx1"] = dx1;
  json["dy1"] = dy1;
  json["dz1"] = dz1;

  // fd
  json["dx2"] = dx2;
  json["dy2"] = dy2;
  json["dz2"] = dz2;

  return json;
}

Json::Value GVoids(int n) {
  return ToArray({
        GVoid(1, 0, -1, 0,
              n, 0, n),

        GVoid(2, 0, -1, 0,
              -n, 0, n),

        GVoid(3, 0, -1, 0,
              -n, 0, -n),

        GVoid(4, 0, -1, 0,
              n, 0, -n),
          });
}

Json::Value SMoves(int dx, int dy, int dz) {
  return ToArray({
        SMove(1, dx, dy, dz),
          SMove(2, dx, dy, dz),
          SMove(3, dx, dy, dz),
          SMove(4, dx, dy, dz),
          });
}

std::vector<std::pair<int, int>> Getdxzs(int R, int n) {
  auto checkRange = [R](int lx, int hx, int lz, int hz) {
    return
    0 <= lx && hx < R &&
    0 <= lz && hz < R;
  };

  std::vector<std::pair<int, int>> dxzs;
  int lx = 0;
  int hx = n + 1;
  int lz = 0;
  int hz = n + 1;
  
  int dx[] = {1, 0, -1, 0};
  int dz[] = {0, 1, 0, 1};
  int didx = 0;

  bool end = false;

  while (true) {
    int i = 1;
    for (; i <= n; ++i) {
      if (!checkRange(lx + dx[didx] * i, hx + dx[didx] * i,
                      lz + dz[didx] * i, hz + dz[didx] * i)) {
        break;
      }
    }
    
    if (i == 1 && end) {
      break;
    }

    if (i == 1) {
      didx = (didx + 1) % 4;
      end = true;
      continue;
    }

    end = false;
    
    --i;
    dxzs.emplace_back(dx[didx] * i, dz[didx] * i);
    
    lx += dx[didx] * i;
    hx += dx[didx] * i;

    lz += dz[didx] * i;
    hz += dz[didx] * i;
  }

  return dxzs;
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  
  const vvv voxels = ReadMDL(FLAGS_mdl_filename);
  const int R = voxels.size();
  
  // TODO(tikuta): replace commands
  Json::Value json;
  
  for (int y = 0; y < R - 1; ) {
    int ny = std::min(y + 15, R - 1);
    Json::Value smove = SMove(1, 0, ny - y, 0);

    json["turn"].append(ToArray({smove}));
    y = ny;
  }

  // Up done.

  // Fission 1
  json["turn"].append(ToArray({Fission(1, 1, 0, 0, 1)}));

  int n = std::min(14, (R - 1) - 1);

  json["turn"].append(ToArray({
        Wait(1),
          SMove(2, n, 0, 0),
      }));
  
  // Fission2
  json["turn"].append(ToArray({
        Fission(1, 0, 0, 1, 1),        
          Fission(2, 0, 0, 1, 1),        
      }));  
  
  json["turn"].append(ToArray({
        Wait(1),
        Wait(2),
          SMove(3, 0, 0, n),
          SMove(4, 0, 0, n),
      }));  
  
  
  // Flip
  json["turn"].append(ToArray({
        Flip(1),
          Wait(2),
          Wait(3),
          Wait(4),
      }));

  
  // TODO: 30x30

  auto dxzs = Getdxzs(R, n);  

  int lx = 0;
  int hx = n + 1;
  int lz = 0;
  int hz = n + 1;
  
  {
    for (int y = R - 1; y >= 1; --y) {
      // Flip
      if (y == 1) {
        json["turn"].append(ToArray({
              Flip(1),
                Wait(2),
                Wait(3),
                Wait(4),
                }));
      }


      for (const auto& xz : dxzs) {
        json["turn"].append(GVoids(n + 1));
        json["turn"].append(SMoves(xz.first, 0, xz.second));
        lx += xz.first;
        hx += xz.first;
        lz += xz.second;
        hz += xz.second;
      }
      json["turn"].append(GVoids(n + 1));

      json["turn"].append(SMoves(0, -1, 0));

      std::reverse(dxzs.begin(), dxzs.end());
      for (auto& xz : dxzs) {
        xz.first *= -1;
        xz.second *= -1;
      }
    }
  }


  // Fusion 1
  json["turn"].append(ToArray({
        Wait(1),
        Wait(2),
          SMove(3, 0, 0, -n),
          SMove(4, 0, 0, -n),
      }));    
  
  json["turn"].append(ToArray({
        FusionP(1, 0, 0, 1),
          FusionP(2, 0, 0, 1),
          FusionS(3, 0, 0, -1),
          FusionS(4, 0, 0, -1),
          }));

  json["turn"].append(ToArray({
        Wait(1),
          SMove(2, -n, 0, 0),
          }));
  
  // Fusion 2
  json["turn"].append(ToArray({
        FusionP(1, 1, 0, 0),
          FusionS(2, -1, 0, 0),
          }));

  
  {
    std::vector<Command> moves;
    for (int i = 0; i < lx; ++i) {
      moves.push_back(Command::make_smove(1, Point(-1, 0, 0)));
    }

    for (int i = 0; i < lz; ++i) {
      moves.push_back(Command::make_smove(1, Point(0, 0, -1)));
    }
    
    moves = MergeSMove(moves);
    
    for (const auto& c : moves) {
      json["turn"].append(Command::CommandsToJson({c}));
    }
  }
  
  json["turn"].append(ToArray({Halt(1)}));

  std::cout << Json2Binary(json);
}