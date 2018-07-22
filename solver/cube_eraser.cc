#include <iostream>
#include <memory>

#include "absl/types/span.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

#include "src/command.h"
#include "src/base/flags.h"
#include "solver/AI.h"


DEFINE_bool(json, false, "output json");


class CubeEraser : public AI {

  int get_height(const vvv& model) {
    for (int y=R-1; y>=0; --y) {
      for (int x=0; x<R; ++x) {
        for (int z=0; z<R; ++z) {
          if (model[x][y][z]) return y;
        }
      }
    }
    return -1;
  }

  void exec(absl::Span<const Command> commands) {
    std::vector<Command> cs;
    for (auto&c : commands) cs.push_back(c);
    ce->Execute(cs);
  }

  Command command_by(Command c, int id) {
    c.id = id;
    return c;
  }

  bool Imcomplete() {
    auto& model = ce->GetSystemStatus().matrix;
    for (int y=R-1; y>=0; --y) {
      for (int x=0; x<R; ++x) {
        for (int z=0; z<R; ++z) {
          if (model[x][y][z]) return true;
        }
      }
    }
    return false;
  }

  void GotoTop() {
    int goal_y = height + 1;
    while (ce->GetBotStatus()[1].pos.y < goal_y) {
      int dy = std::min(15, goal_y - ce->GetBotStatus()[1].pos.y);
      exec({ Command::make_smove(1, Point(0, dy, 0)) });
    }
  }

  std::vector<Command> MassuguGo(int id, const Point& from, const Point& goal) {
    std::vector<Command> ret;
    int x = from.x;
    int y = from.y;
    int z = from.z;
    while (x != goal.x) {
      int dx = std::min(15, abs(x - goal.x));
      if (x > goal.x) dx *= -1;
      x += dx;
      ret.push_back(Command::make_smove(id, Point(dx, 0, 0)));
    }
    while (y != goal.y) {
      int dy = std::min(15, abs(y - goal.y));
      if (y > goal.y) dy *= -1;
      y += dy;
      ret.push_back(Command::make_smove(id, Point(0, dy, 0)));
    }
    while (z != goal.z) {
      int dz = std::min(15, abs(z - goal.z));
      if (z > goal.z) dz *= -1;
      z += dz;
      ret.push_back(Command::make_smove(id, Point(0, 0, dz)));
    }
    return ret;
  }

  void MakeSquare() {

    int x1 = ce->GetBotStatus()[1].pos.x;
    // int y1 = ce->GetBotStatus()[1].pos.y;
    int z1 = ce->GetBotStatus()[1].pos.z;

    { // Fission: 2 from 1
      exec({ Command::make_fission(1, Point(1,0,0), 1) });
    }
    { // Move: 2 from 1
      int goal_dx = std::min(x1 + 29, R - 1) - x1 - 1;
      auto commands = MassuguGo(
          2,
          ce->GetBotStatus()[2].pos,
          ce->GetBotStatus()[2].pos + Point(goal_dx, 0, 0));
      for (auto& c : commands) {
        exec({ Command::make_wait(1), c });
      }
    }
    { // Fission: 3 from 2, 4 from 1
      exec({
          Command::make_fission(1, Point(0,0,1), 0),
          Command::make_fission(2, Point(0,0,1), 0)
          });
    }
    { // Move: 3 from 2, 4 from 1
      int goal_dz = std::min(z1 + 29, R - 1) - z1 - 1;
      auto commands = MassuguGo(
          4,
          ce->GetBotStatus()[4].pos,
          ce->GetBotStatus()[4].pos + Point(0, 0, goal_dz));
      for (size_t i = 0; i < commands.size(); ++i) {
        exec({
            Command::make_wait(1),
            Command::make_wait(2),
            command_by(commands[i], 3),
            command_by(commands[i], 4),
            });
      }
    }
  }

  bool IsEmpty(int y) {
    auto& model = ce->GetSystemStatus().matrix;
    for (int x=0; x<R; ++x) {
      for (int z=0; z<R; ++z) {
        if (model[x][y][z]) return false;
      }
    }
    return true;
  }

  std::pair<bool, std::pair<Point, Point> > GetGoodCube() {
    return std::make_pair(false, std::make_pair(Point(), Point()));
  }

  Point GetGoodSquare(int len, int y) {
    auto& model = ce->GetSystemStatus().matrix;
    Point p;
    int mx = 0;
    for (int x = 0; x < R - len + 1; ++x) {
      for (int z = 0; z < R - len + 1; ++z) {

        int n = 0;
        for (int i = 0; i < len; ++i) {
          for (int j = 0; j < len; ++j) {
            if (model[x+i][y][z+j]) n += 1;
          }
        }
        if (n > mx) {
          mx = n;
          p = Point(x, y, z);
        }
      }
    }
    return p;
  }

  void Work() {

    LOG(INFO) << ce->GetBotStatus()[1].pos;
    LOG(INFO) << ce->GetBotStatus()[2].pos;
    LOG(INFO) << ce->GetBotStatus()[3].pos;
    LOG(INFO) << ce->GetBotStatus()[4].pos;

    int bot_y = ce->GetBotStatus()[1].pos.y;
    CHECK(bot_y > 0);
    int len = ce->GetBotStatus()[2].pos.x - ce->GetBotStatus()[1].pos.x + 1;

    if (IsEmpty(bot_y - 1)) {
      LOG(INFO) << "Layer " << (bot_y-1) << " is empty.";
      exec({
          Command::make_smove(1, Point(0, -1, 0)),
          Command::make_smove(2, Point(0, -1, 0)),
          Command::make_smove(3, Point(0, -1, 0)),
          Command::make_smove(4, Point(0, -1, 0))
          });
      return;
    }

    auto cube = GetGoodCube();
    if (cube.first) {
      return;
    }

    auto square = GetGoodSquare(len, bot_y - 1);
    LOG(INFO) << "square " << square << " " << bot_y;
    CHECK(square.y == bot_y - 1);
    {
      auto commands = MassuguGo(1,
          ce->GetBotStatus()[1].pos,
          square + Point(0, 1, 0));
      for (auto& c : commands) {
        exec({
            command_by(c, 1),
            command_by(c, 2),
            command_by(c, 3),
            command_by(c, 4),
            });
      }
      Point nd(0, -1, 0);
      exec({
          Command::make_gvoid(1, nd, Point(len-1, 0, len-1)),
          Command::make_gvoid(2, nd, Point(-len+1, 0, len-1)),
          Command::make_gvoid(3, nd, Point(-len+1, 0, -len+1)),
          Command::make_gvoid(4, nd, Point(len-1, 0, -len+1)),
          });
    }


  }

  void FusionSquare() {

    { // Move: 4->1, 3->2
      auto commands = MassuguGo(
          4,
          ce->GetBotStatus()[4].pos,
          ce->GetBotStatus()[1].pos + Point(0, 0, 1));
      for (size_t i = 0; i < commands.size(); ++i) {
        exec({
            Command::make_wait(1),
            Command::make_wait(2),
            command_by(commands[i], 3),
            command_by(commands[i], 4)
            });
      }
    }
    { // Fusion: 4->1, 3->2
      exec({
          Command::make_fusion_p(1, Point(0, 0, 1)),
          Command::make_fusion_p(2, Point(0, 0, 1)),
          Command::make_fusion_s(3, Point(0, 0, -1)),
          Command::make_fusion_s(4, Point(0, 0, -1))
          });
    }
    { // Move: 2->1
      auto commands = MassuguGo(
          2,
          ce->GetBotStatus()[2].pos,
          ce->GetBotStatus()[1].pos + Point(1, 0, 0));
      for (size_t i = 0; i < commands.size(); ++i) {
        exec({ Command::make_wait(1), commands[i] });
      }
    }
    { // Fusion: 2->1
      exec({
          Command::make_fusion_p(1, Point(1, 0, 0)),
          Command::make_fusion_s(2, Point(-1, 0, 0))
          });
    }
  }

  void GotoOrigin() {
    auto commands = MassuguGo(1, ce->GetBotStatus()[1].pos, Point(0, 0, 0));
    for (auto& c : commands) {
      exec({ c });
    }
  }

public:

  int R;
  int height;

  CubeEraser(const vvv &model) : AI(model) {
    R = model.size();
    height = get_height(model);
  }
  ~CubeEraser() override = default;

  void Run() override {
    exec({ Command::make_flip(1) });
    GotoTop();
    MakeSquare();
    LOG(INFO) << "start work";
    while (Imcomplete()) Work();
    LOG(INFO) << "end work";
    if (ce->GetSystemStatus().harmonics == HIGH) {
      exec({
          Command::make_flip(1),
          Command::make_wait(2),
          Command::make_wait(3),
          Command::make_wait(4)
          });
    }
    FusionSquare();
    GotoOrigin();
    exec({ Command::make_halt(1) });
  }
};

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  vvv M = ReadMDL(FLAGS_src_filename);
  // WriteMDL(FLAGS_tgt_filename + "_dbl", M); // should be the same file
  //  OutputMDL(M);

  auto ai = std::make_unique<CubeEraser>(M);
  ai->Run();
  ai->Finalize();
}

/* vim: set tabstop=2 sts=2 shiftwidth=2 expandtab: */
