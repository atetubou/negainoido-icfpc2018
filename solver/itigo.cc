#include <iostream>
#include <memory>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

#include "src/base/flags.h"

#include "solver/AI.h"

DEFINE_bool(flip, true, "do flip?");
DEFINE_bool(json, false, "output json");
DEFINE_int32(x_num, 6, "x separation");
DEFINE_int32(z_num, 6, "z separation");

class Itigo : public AI {
  const vvv model;
  const int R;
  const bool use_flip;

  std::vector<int> bids;

  Point low, high;
  int Rx = 0;
  int Ry = 0;
  int Rz = 0;

public:
  Itigo(const vvv &model, bool use_flip) 
    : AI(model.size()), model(model), R(model.size()),
      use_flip(use_flip) {
    bids.push_back(1);
  }

  ~Itigo() override = default;

  void ExecuteOrWait(const std::vector<Command> commands) {
    ce->Execute(FillWait(commands));
  }

  void FissionAndGo(int bid, const Point& goal, int m) {
    Point v = (goal - bot(bid).pos).Normalize();
    CHECK_EQ(v.Manhattan(), 1) 
      << v << " " << bid << " " << bot(bid).pos << " " << goal;

    bids.push_back(bot(bid).TryFission(v, m).second.id);
    
    ExecuteOrWait({Command::make_fission(bid, v, m)});

    std::vector<std::pair<int, Point>> targets = {
      {bids.back(), goal},
    };

    for (;;) {
      const auto cmds = MultiStepSMove(targets);
      if (cmds.empty()) break;
      ExecuteOrWait(cmds);
    }
  }

  void DebugSeed() const {
    for (size_t i = 0; i < bids.size(); ++i) {
      int b = bids[i];
      LOG(INFO) 
        << " i=" << i
        << " bid=" << b 
        << " pos=" << bot(b).pos
        << " seed size=" << bot(b).seeds.size();
    }
  }

  void DoHaiti(int x_num, int z_num) {
    CHECK_GT(x_num, 0);
    CHECK_GT(z_num, 0);

    CHECK_EQ(bids.size(), 1u);
    CHECK_EQ(bids.back(), 1);

    DoMove({std::make_pair(1, Point(low.x, 1, low.z))});

    for (int i = 0; i < x_num - 1; ++i) {
      int bid = bids.back();
      const auto& b = bot(bid);

      int child_n = z_num * (x_num - i - 1) - 1;

      FissionAndGo(bid, b.pos + Point(Rx/x_num, 0, 0), child_n);
    }

    for (int i = 0; i < z_num - 1; ++i) {
      for (int j = 0; j < x_num; ++j) {
        int bid = bids.at(bids.size() - x_num);
        const auto& b = bot(bid);
        FissionAndGo(bid, b.pos + Point(0, 0, Rz / z_num), z_num - i - 2);
      }
    }
  }

  int DoMove(const std::vector<std::pair<int, Point>>& targets) {
    int cnt = 0;
    while (true) {
      ++cnt;
      std::vector<Command> turn;

      for (const auto& t : targets) {
        const Point target = t.second;
        int bid = t.first;
        
        if (bot(bid).pos == target) {
          continue;
        }

        turn.push_back(GetStepSMove(bid, bot(bid).pos, target));
      }
      
      if (turn.empty()) {
        break;
      }

      ExecuteOrWait(turn);
    }
    return cnt;
  }

  void CalcRegion() {
    low = Point(R - 1, R - 1, R - 1);
    high = Point(0, 0, 0);

    for (int y = 0; y < R; ++y) {
      for (int x = 0; x < R; ++x) {
        for (int z = 0; z < R; ++z) {
          if (!model[x][y][z]) continue;
          low.x = std::min(low.x, x);
          low.y = std::min(low.y, y);
          low.z = std::min(low.z, z);

          high.x = std::max(high.x, x + 1);
          high.y = std::max(high.y, y + 1);
          high.z = std::max(high.z, z + 1);
        }
      }
    }

    Rx = high.x - low.x + 1;
    Ry = high.y - low.y + 1;
    Rz = high.z - low.z + 1;
  }

  void Run() override {
    CalcRegion();

    LOG(INFO) << low << " " << high;

    ExecuteOrWait({Command::make_smove(1, Point(0, 1, 0))});
    const int n = std::min(FLAGS_x_num, Rx);
    const int m = std::min(FLAGS_z_num, Rz);

    DoHaiti(n, m);

    int lenx = Rx / n;
    int lenz = Rz / m;

    std::vector<std::vector<Point>> targets(n * m);

    for (int sy = 0; sy < Ry - 1; ++sy) {
      for (int sx = 0; sx < Rx; ++sx) {
        for (int sz = 0; sz < Rz; ++sz) {
          int x = sx + low.x;
          int y = sy + low.y;
          int z = sz + low.z;
          if (!model[x][y][z]) continue;

          int bx = std::min(sx / lenx, n - 1);
          int bz = std::min(sz / lenz, m - 1);
          
          targets[bz * n + bx].emplace_back(x, y + 1, z);
        }
      }
    }

    if (use_flip) {
      ExecuteOrWait({Command::make_flip(1)});
    }

    std::vector<size_t> idx(n * m);
    while (true) {
      std::vector<Command> turn;

      for (size_t i = 0; i < targets.size(); ++i) {
        if (idx[i] == targets[i].size()) continue;
        const Point target = targets[i][idx[i]];
        int bid = bids[i];
        
        if (bot(bid).pos == target) {
          ++idx[i];
          turn.push_back(Command::make_fill(bid, Point(0, -1, 0)));
          continue;
        }

        turn.push_back(GetStepSMove(bid, bot(bid).pos, target));
      }
      
      if (turn.empty()) {
        break;
      }

      ExecuteOrWait(turn);
    }


    if (use_flip) {
      ExecuteOrWait({Command::make_flip(1)});
    }

    
    {
      std::vector<std::pair<int, Point>> before_fusion;

      for (int j = 0; j < m; ++j) {
        for (int i = 0; i < n; ++i) {
          before_fusion.emplace_back(bids[j * n + i],
                                     Point(i * lenx + low.x, R - 1, j * lenz + low.z));
        }
      }

      LOG(INFO) << "move to up";

      DoMove(before_fusion);

      LOG(INFO) << "moved to up";
    }

    // OK
    // return;

    for (int j = m - 1; j > 0; --j) {
      std::vector<std::pair<int, Point>> before_fusion;
      std::vector<Command> fusion;

      for (int i = 0; i < n; ++i) {
        int cbid = bids[j * n + i];
        int pbid = bids[(j - 1) * n + i];
        const Point ppos = bot(pbid).pos;

        before_fusion.emplace_back(cbid, ppos + Point(0, 0, 1));

        fusion.push_back(Command::make_fusion_p(pbid, Point(0, 0, 1)));
        fusion.push_back(Command::make_fusion_s(cbid, Point(0, 0, -1)));
      }
      DoMove(before_fusion);
      ExecuteOrWait(fusion);
    }

    LOG(INFO) << "xfusion";

    for (int i = n - 1; i > 0; --i) {
      std::vector<std::pair<int, Point>> before_fusion;
      std::vector<Command> fusion;
      
      int cbid = bids[i];
      int pbid = bids[i - 1];
      const Point ppos = bot(pbid).pos;

      before_fusion.emplace_back(cbid, ppos + Point(1, 0, 0));
      fusion.push_back(Command::make_fusion_p(pbid, Point(1, 0, 0)));
      fusion.push_back(Command::make_fusion_s(cbid, Point(-1, 0, 0)));

      DoMove(before_fusion);
      ExecuteOrWait(fusion);
    }

    DoMove({std::make_pair(1, Point(0, R - 1, 0))});

    DoMove({std::make_pair(1, Point())});

    ExecuteOrWait({Command::make_halt(1)});
  }
};

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const vvv voxels = ReadMDL(FLAGS_tgt_filename);
  auto ai = std::make_unique<Itigo>(voxels, FLAGS_flip);
  ai->Run();
  ai->Finalize();
}
