#include <iostream>
#include <memory>
#include <queue>
#include <map>
#include <iomanip>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

#include "solver/AI.h"
#include "src/command.h"
#include "solver/simple_solve.h"
// #include "solver/square_delete.h"
#include "src/base/flags.h"
using namespace std;

#define REP2(i, m, n) for(int i = (int)(m); i < (int)(n); i++)
#define REPD(i, n) for(int i = (int)(n) - 1; i >= 0; i--)
#define REP(i, n) REP2(i, 0, n)
#define ALL(c) (c).begin(), (c).end()
#define PB(e) push_back(e)
#define FOREACH(i, c) for(auto i = (c).begin(); i != (c).end(); ++i)
#define MP(a, b) make_pair(a, b)
#define BIT(n, m) (((n) >> (m)) & 1)

template <typename S, typename T> ostream &operator<<(ostream &out, const pair<S, T> &p) {
  out << "(" << p.first << ", " << p.second << ")";
  return out;
}

template <typename T> ostream &operator<<(ostream &out, const vector<T> &v) {
  out << "[";
  REP(i, v.size()){
    if (i > 0) out << ", ";
    out << v[i];
  }
  out << "]";
  return out;
}

class VerticalAI : public AI {
  const vvv M;
  const int R;
  const int maxy;
  const int MAXB;
  const int dx[6] = {-1,0,0,1,0,0};
  const int dy[6] = {0,-1,0,0,1,0};
  const int dz[6] = {0,0,-1,0,0,1};
  typedef pair<Point, Point> Region; // I don't care y-axis for this type
  
public:
  VerticalAI(const vvv &tgt_model) : AI(tgt_model.size()), M(tgt_model), R(tgt_model.size()), maxy(ComputeMaxY()), MAXB(min(30, R)){ }
  void Run() override {
    vector<pair<Point, Point> > decomposed_regions = DecomposeRegions();
    const int bot_num = decomposed_regions.size();
    FissionBots(bot_num);
    vector<int> bot_ids = getActiveBots();
    LOG(INFO) << "num bots: " << bot_ids.size();
    LOG_ASSERT(int(bot_ids.size()) == bot_num);

    LOG(INFO) << "Started moving to each origin: ";
    GotoEachOrigin(decomposed_regions, bot_ids);

    LOG(INFO) << "Start solving each origin: ";

    vector<vector<Command>> commands_list;
    REP(i, bot_ids.size()) {
      commands_list.push_back(SolveRegion(bot_ids[i], decomposed_regions[i]));
    }
    size_t max_len = 0;
    for (const auto &commands : commands_list) {
      max_len = max(max_len, commands.size());
    }

    LOG(INFO) << " max-len: " << max_len;
    REP(i, bot_ids.size()) {
      const int b = bot_ids[i];
      while (commands_list[i].size() < max_len) {
        commands_list[i].push_back(Command::make_wait(b));
      }
    }

    REP(i, max_len) {
      vector<Command> cmds;
      REP(j, bot_ids.size()) {
        auto c = commands_list[j][i];
        c.id = bot_ids[j];
        cmds.push_back(c);
      }
      ce->Execute(cmds);
    }
    
    // REP(y, maxy - 1){
    //   vector<Command> cmds;
    //   for (int b : getActiveBots()) {
    //     cmds.push_back(Command::make_smove(b, Point(0, 1, 0)));
    //   }
    //   ce->Execute(cmds);
    // }
    LOG(INFO) << "Moved to y = ymax";
    FusionBots();
    FinishTravel();
  }

private:

  vector<Region> DecomposeRegions() {
    typedef pair<int, Region> State;
    priority_queue<State> pque;
    Region region(Point(1, 0, 1), Point(R - 2, 0, R - 2));
    pque.push(make_pair(CountFullVoxel(region), region));

    vector<Region> leaf_regions;
    while (!pque.empty() && int(pque.size() + leaf_regions.size()) < MAXB) {
      // show(pque);
      const auto p = pque.top(); pque.pop();
      const auto region = p.second;
      const int lx = region.first.x;
      const int lz = region.first.z;
      const int hx = region.second.x;
      const int hz = region.second.z;
      vector<pair<int, pair<Region, Region>> > sub_regions;

      const int minimum_size = 2;
      for (int mx = lx + minimum_size; mx < hx - minimum_size + 2; mx++) {
        const Region sub_region1 = Region(region.first, Point(mx - 1, 0, region.second.z));
        const Region sub_region2 = Region(Point(mx, 0, region.first.z), region.second);
        if (IsGrounded(sub_region1) && IsGrounded(sub_region2)) {
          int count1 = CountFullVoxel(sub_region1);
          int count2 = CountFullVoxel(sub_region2);
          sub_regions.push_back(make_pair(abs(count1 - count2), make_pair(sub_region1, sub_region2)));
        }
      }

      for (int mz = lz + minimum_size; mz < hz - minimum_size + 2; mz++) {
        const Region sub_region1 = Region(region.first, Point(region.second.x, 0, mz - 1));
        const Region sub_region2 = Region(Point(region.first.x, 0, mz), region.second);
        if (IsGrounded(sub_region1) && IsGrounded(sub_region2)) {
          int count1 = CountFullVoxel(sub_region1);
          int count2 = CountFullVoxel(sub_region2);
          sub_regions.push_back(make_pair(abs(count1 - count2), make_pair(sub_region1, sub_region2)));
        }
      }

      sort(ALL(sub_regions));
      if (sub_regions.size() > 0) {
        const auto region_a = sub_regions[0].second.first;
        const auto region_b = sub_regions[0].second.second;
        pque.push(State(CountFullVoxel(region_b), region_b));
        pque.push(State(CountFullVoxel(region_a), region_a));
      }
      else {
        LOG(INFO) << "add to leaf: " <<region<< " pque: "<<  pque.size();
        leaf_regions.push_back(region);
      }
    }

    while (!pque.empty()) {
      const auto p = pque.top(); pque.pop();
      const auto region = p.second;
      leaf_regions.push_back(region);
    }

    LOG(INFO) << "Splitted regions " << "(" << leaf_regions.size() << "): " << leaf_regions << endl;
    return leaf_regions;
  }

  void GotoEachOrigin(const vector<Region> &regions, const vector<int> bot_ids) {
    LOG(INFO) << " bot_ids: " << bot_ids;
    vector<Point> origins;
    REP(i, regions.size()) {
      origins.push_back(regions[i].first);
    }
    
    {
      auto &bot_status = ce->GetBotStatus();
      while(true) {
        bool finish = true;
        vector<Command> cmds;
        REP(i, bot_ids.size()) {
          const int b = bot_ids[i];
          if (bot_status[b].pos.y < i) {
            int dy = min(15, i - bot_status[b].pos.y );
            cmds.push_back(Command::make_smove(b, Point(0, dy, 0)));
            finish = false;
          } else {
            cmds.push_back(Command::make_wait(b));
          }
        }
        if (finish) break;
        ce->Execute(cmds);
      }
    }

    LOG(INFO) << " origins: " << origins;
    OutputBotPoints();
    

    int num_iter = 0;
    while (!ReachedToOrigin(origins, bot_ids)) {
      auto bot_status = ce->GetBotStatus();
      // LOG(INFO) << " origins: " << origins;
      // OutputBotPoints();
      num_iter++;

      vector<Command> cmds;
      REP(i, bot_ids.size()) {
        const int b = bot_ids[i];
        const int max_dx = origins[i].x - bot_status[bot_ids[i]].pos.x;
        const int max_dz = origins[i].z - bot_status[bot_ids[i]].pos.z;
        int best_x = min(abs(max_dx), 15);
        int best_z = min(abs(max_dz), 15);

        if (best_x == 0 && best_z == 0) {
          cmds.push_back(Command::make_wait(b));
        } else if (abs(best_x) > abs(best_z)) {
          const int dx = max_dx / abs(max_dx);
          cmds.push_back(Command::make_smove(b, Point(best_x * dx, 0, 0)));
        } else {
          const int dz = max_dz / abs(max_dz);
          cmds.push_back(Command::make_smove(b, Point(0, 0, best_z * dz)));
        }
      }
      LOG_IF(FATAL, num_iter > 10000) << " Too many iterations. Let's reimplement this function:)";
      ce->Execute(cmds);
    }

    
    {
      auto &bot_status = ce->GetBotStatus();
      while(true) {
        bool finish = true;
        vector<Command> cmds;
        REP(i, bot_ids.size()) {
          const int b = bot_ids[i];
          if (bot_status[b].pos.y > 0) {
            int dy = min(15, bot_status[b].pos.y);
            cmds.push_back(Command::make_smove(b, Point(0, -dy, 0)));
            finish = false;
          } else {
            cmds.push_back(Command::make_wait(b));
          }
        }
        if (finish) break;
        ce->Execute(cmds);
      }
    }
  }

  bool ReachedToOrigin(const vector<Point> &origins, const vector<int> &bot_ids) {
    auto bot_status = ce->GetBotStatus();
    REP(i, origins.size()) {
      if (origins[i].x != bot_status[bot_ids[i]].pos.x) return false;
      if (origins[i].z != bot_status[bot_ids[i]].pos.z) return false;
    }
    return true;
  }

  vector<Command> SolveRegion(const int b, const Region &region) {
    const int lx = region.first.x;
    const int lz = region.first.z;
    const int hx = region.second.x;
    const int hz = region.second.z;

    const int xlen = hx - lx + 1;
    const int zlen = hz - lz + 1;
    const int ylen = maxy + 1;
    cerr << xlen << " " << ylen << " " << zlen << " " << endl;
    vvv sub_voxels = vvv(xlen, vv(ylen, v(zlen)));
    for (int x = lx; x <= hx; x++) {
      for (int y = 0; y < ylen; y++) {
        for (int z = lz; z <= hz; z++) {
          if (M[x][y][z]) {
            sub_voxels[x - lx][y][z - lz] = 1;
          }
        }
      }
    }
    LOG(INFO) << " Prepared sub_voxels" ;
    return SimpleSolve(sub_voxels, Point(hx - lx, maxy, hz - lz), false);
    // return vector<Command>();
  }

  void FissionBots(const int size) {
    LOG(INFO) << "Started creating bots";
    const int width = 5;
    const int nzs = (size + width - 1) / width;

    for (int b = 1; b < min(5, size); b++) {
      vector<Command> cmds;
      REP2(b_, 1, b) {
        cmds.push_back(Command::make_wait(b_));
      }
      cmds.push_back(Command::make_fission(b, Point(1, 0, 0), 39 - 8 * b));
      LOG(INFO) << "Created bot " << b + 1;
      ce->Execute(cmds);
    }
    LOG(INFO) << "Created first five bots";

    if (size <= 5) {
      return;
    }

    int rest = size - 5;
    REP(z, nzs - 1) {
      int go = min(rest, 5);
      const auto bots = ce->GetBotStatus();
      vector<Command> cmds;
      for (const int b : getActiveBots()) {
        if (bots[b].pos.z == z && go > 0) {
          cmds.push_back(Command::make_fission(b, Point(0, 0, 1), bots[b].seeds.size() - 1));
          rest--;
          go--;
        } else {
          cmds.push_back(Command::make_wait(b));
        }
      }
      ce->Execute(cmds);
    }
    LOG_ASSERT(rest == 0);
    LOG(INFO) << "Finished creating bots";
  }
  

  void FusionBots() {
    
    typedef pair<int, int>  P;

    // Move every bots to z = 0
    while (nonzeroZBotExists()) {
      auto &bots = ce->GetBotStatus();
      vector<Command> cmds;
      map<int, vector<P> > z_bots;
      for (const int b : getActiveBots()) {
        z_bots[bots[b].pos.x].push_back(make_pair(bots[b].pos.z, b));
      }

      for (auto &p: z_bots) {
        auto &vec = p.second;
        sort(ALL(vec));
        reverse(ALL(vec));
        if (bots[vec[0].second].pos.z == 0) {
          LOG_ASSERT(vec.size() == 1) << p.first << " " << vec.size();
          cmds.push_back(Command::make_wait(vec[0].second));
        } else if (vec.size() == 1) {
          const int dz = min(15, vec[0].first);
          cmds.push_back(Command::make_smove(vec[0].second, Point(0, 0, -dz)));
        } else if (vec[1].first + 1 == vec[0].first) {
          if (vec[0].second < vec[1].second) {
            cmds.push_back(Command::make_fusion_s(vec[1].second, Point(0, 0, 1)));
            cmds.push_back(Command::make_fusion_p(vec[0].second, Point(0, 0, -1)));
          } else {
            cmds.push_back(Command::make_fusion_p(vec[1].second, Point(0, 0, 1)));
            cmds.push_back(Command::make_fusion_s(vec[0].second, Point(0, 0, -1)));
          }
          REP2(i, 2, vec.size()) {
            cmds.push_back(Command::make_wait(vec[i].second));
          }
        } else {
          const int dz = min(15, vec[0].first - vec[1].first - 1);
          cmds.push_back(Command::make_smove(vec[0].second, Point(0, 0, -dz)));
          REP2(i, 1, vec.size()) {
            cmds.push_back(Command::make_wait(vec[i].second));
          }
        }
      }
      ce->Execute(cmds);
    }
    LOG(INFO) << "Moved towards z = 0";
    OutputBotPoints();

    // Move every bots to x = 0
    while (nonzeroXBotExists()) {
      auto &bots = ce->GetBotStatus();
      vector<Command> cmds;
      vector<P> vec;
      
      for (const int b : getActiveBots()) {
        vec.push_back(make_pair(bots[b].pos.x, b));
      }
      sort(ALL(vec));
      reverse(ALL(vec));
      LOG_ASSERT(bots[vec[0].second].pos.x > 0);
      if (vec.size() == 1) {
        const int dx = min(15, vec[0].first);
        cmds.push_back(Command::make_smove(vec[0].second, Point(-dx, 0, 0)));
      } else if (vec[1].first + 1 == vec[0].first) {
        if (vec[0].second < vec[1].second) {
          cmds.push_back(Command::make_fusion_s(vec[1].second, Point(1, 0, 0)));
          cmds.push_back(Command::make_fusion_p(vec[0].second, Point(-1, 0, 0)));
        } else {
          cmds.push_back(Command::make_fusion_p(vec[1].second, Point(1, 0, 0)));
          cmds.push_back(Command::make_fusion_s(vec[0].second, Point(-1, 0, 0)));
        }
        REP2(i, 2, vec.size()) {
          cmds.push_back(Command::make_wait(vec[i].second));
        }
      } else {
        const int dx = min(15, vec[0].first - vec[1].first - 1);
        cmds.push_back(Command::make_smove(vec[0].second, Point(-dx, 0, 0)));
        REP2(i, 1, vec.size()) {
          cmds.push_back(Command::make_wait(vec[i].second));
        }
      }
      ce->Execute(cmds);
    }
    LOG(INFO) << "Moved towards x = 0";
    OutputBotPoints();
    
  }

  bool nonzeroXBotExists() {
    const auto &bots = ce->GetBotStatus();
    for (const int b: getActiveBots()) {
      if (bots[b].pos.x > 0) return true;
    }
    return false;    
  }

  bool nonzeroZBotExists() {
    const auto &bots = ce->GetBotStatus();
    for (const int b: getActiveBots()) {
      if (bots[b].pos.z > 0) return true;
    }
    return false;    
  }

  vector<int> getActiveBots() {
    const auto bots = ce->GetBotStatus();
    vector<int> res;
    for (int b = 1; b < int(bots.size()); b++) {
      if (bots[b].active) {
        res.push_back(b);
      }
    }
    return res;
  }

  void OutputBotPoints() {
    const auto bots = ce->GetBotStatus();
    for (int b = 1; b <= CommandExecuter::kMaxNumBots; b++) {
      if (bots[b].active) {
        LOG(INFO) << "Bot " << b << ": " << bots[b].pos;
      }
    }
  }

  void FinishTravel() {
    auto &bots = ce->GetBotStatus();
    const int b = getActiveBots()[0];
    while (bots[b].pos.y > 0) {
      const int dy = std::min(bots[b].pos.y, 15);
      ce->Execute({Command::make_smove(b, Point(0, -dy, 0))});
    }
    ce->Execute({Command::make_halt(1)});
  };

  int ComputeMaxY() {
    int maxy = 0;
    for (int y = R - 1; y >= 0; --y) {
      for (int x = 0; x < R; ++x) {
        for (int z = 0; z < R; ++z) {
          if (M[x][y][z]) {
            maxy = y + 1;
            break;
          }
        }
      }
      if (maxy) {
        break;
      }
    }
    return maxy;
  }

  map<Region, int> count_memo;
  int CountFullVoxel(const Region & region) {

    if (count_memo.count(region)) {
      return count_memo[region];
    }
    
    const int lx = region.first.x;
    const int lz = region.first.z;
    const int hx = region.second.x;
    const int hz = region.second.z;
    int count = 0;
    REP2(x, lx, hx + 1) REP(y, R) REP2(z, lz, hz + 1) {
      if (M[x][y][z]) count++;
    }
    count_memo[region] = count;
    return count;
  }

  int grounded_search_timestamp = 1;
  int grounded_visit[CommandExecuter::kMaxResolution][CommandExecuter::kMaxResolution][CommandExecuter::kMaxResolution];
  bool IsGrounded(const Region &region) {
    grounded_search_timestamp++;
    const int lx = region.first.x;
    const int lz = region.first.z;
    const int hx = region.second.x;
    const int hz = region.second.z;
    queue<int> quex, quey, quez;

    REP2(x, lx, hx + 1) REP2(z, lz, hz + 1) {
      if (M[x][0][z]) {
        quex.push(x);
        quey.push(0);
        quez.push(z);
        grounded_visit[x][0][z] = grounded_search_timestamp;
      }
    }

    if (quex.empty()) {
      return false;
    }

    int count = 0;
    while (!quex.empty()) {
      int cx = quex.front(); quex.pop();
      int cy = quey.front(); quey.pop();
      int cz = quez.front(); quez.pop();
      count++;
      REP(k, 6) {
        int nx = cx + dx[k];
        int ny = cy + dy[k];
        int nz = cz + dz[k];
        if (lx <= nx && nx <= hx && lz <= nz && nz <= hz && 0 <= ny && ny < R &&
            M[nx][ny][nz] && grounded_visit[nx][ny][nz] != grounded_search_timestamp) {
          grounded_visit[nx][ny][nz] = grounded_search_timestamp;
          quex.push(nx);
          quey.push(ny);
          quez.push(nz);
        }
      }
    }
    return count == CountFullVoxel(region);
  }
};

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const vvv voxels = ReadMDL(FLAGS_tgt_filename);
  auto vertical_ai = std::make_unique<VerticalAI>(voxels);
  vertical_ai->Run();
  vertical_ai->Finalize();
  return 0;
}

