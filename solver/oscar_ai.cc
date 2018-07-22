#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <deque>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "src/command_util.h"
#include "src/command_executer.h"

#include "solver/AI.h"

using namespace std;

static int dx[] = {-1, 1, 0, 0, 0, 0};
static int dy[] = {0, 0, -1, 1, 0, 0};
static int dz[] = {0, 0, 0, 0, -1, 1};

enum class MyVoxelState {
  kALWAYSEMPTY,
  kSHOULDBEFILLED,
  kALREADYFILLED,
};

using ev = std::vector<MyVoxelState>;
using evv = std::vector<ev>;
using evvv = std::vector<evv>;

int total_visit = 0;

vector<Command> get_commands_for_next(const Point& current, const Point& dest, 
                                      const evvv& voxel_states) {
  const int R = voxel_states.size();

  struct State {
    State(int estimated, int current_cost, const Point& p) :
      estimated(estimated), current_cost(current_cost), p(p) {};
    
    int estimated;
    int current_cost;
    Point p;
    
    bool operator<(const State& o) const {
      return estimated > o.estimated;
    }
  };

  std::priority_queue<State> que;
  int count = 1;

  static vvv tmp_map(R, vv(R, v(R, -1)));
  
  vector<Point> visited;

  que.push(State((dest - current).Manhattan(), 0, current));

  while(!que.empty()) {
    const State st = que.top();
    const Point cur = st.p;
    que.pop();

    if (tmp_map[cur.x][cur.y][cur.z] >= 0) continue;
    tmp_map[cur.x][cur.y][cur.z]  = st.current_cost;
    visited.push_back(cur);
    ++total_visit;
    count++;

    if (dest == cur) {
      break;
    }

    for(int i=0;i<6;i++) {
      int nx = cur.x + dx[i];
      int ny = cur.y + dy[i];
      int nz = cur.z + dz[i];
      if (nx >= 0 && ny>= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
        if (tmp_map[nx][ny][nz] >= 0) continue;

        if (voxel_states[nx][ny][nz] != MyVoxelState::kALREADYFILLED) {
          Point np = Point(nx,ny,nz);
          que.push(State(st.current_cost + 1 + (np - dest).Manhattan(), st.current_cost + 1, np));
        }
      }
    }
  }

  LOG_IF(FATAL, tmp_map[dest.x][dest.y][dest.z] == 0) << "can not reach to dest";

  vector<Command> rev_commands;
  Point rc = dest;
  while(rc != current) {
    int mind = -1;
    int minc = tmp_map[rc.x][rc.y][rc.z];
    for(int i=0;i<6;i++) {
      int nx = rc.x + dx[i];
      int ny = rc.y + dy[i];
      int nz = rc.z + dz[i];
      if (nx >= 0 && ny>= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
        const int tmpv = tmp_map[nx][ny][nz];
        if (minc > tmpv && tmpv >= 0) {
          mind = i;
          break;
        }
      }
    }
    
    LOG_IF(FATAL, mind < 0) << "Error in BFS to find path";

    rc.x = rc.x + dx[mind];
    rc.y = rc.y + dy[mind];
    rc.z = rc.z + dz[mind];
    rev_commands.push_back(Command::make_smove(1, Point(-dx[mind], -dy[mind], -dz[mind])));
  }

  reverse(rev_commands.begin(), rev_commands.end());

  for (const auto& v : visited) {
    tmp_map[v.x][v.y][v.z] = -1;
  }

  return rev_commands;
}

#define DOWN_X 0
#define UP_X 1
#define DOWN_Y 2
#define UP_Y 3
#define DOWN_Z 4
#define UP_Z 5

struct Vox
{
    vector<bool> voxels;
    vector<int> colors;
    int R;
    map<int,int> g2d;
    int color_count;

    Vox(const vvv &voxels)
    {
        R = voxels.size();
        this->voxels.resize((R + 2) * (R + 2) * (R + 2), false);
        this->colors.resize((R + 2) * (R + 2) * (R + 2), -1);
        for (int i = 0; i < R; i++)
            for (int j = 0; j < R; j++)
                for (int k = 0; k < R; k++)
                {
                    this->voxels[(i + 1) * (R + 2) * (R + 2) + (j + 1) * (R + 2) + k + 1] = voxels[i][j][k] == 1;
                }
    }
    bool get(int x, int y, int z)
    {
        CHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
        CHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
        CHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
        return voxels[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1];
    }

    int get_color(int x, int y, int z)
    {
        CHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
        CHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
        CHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
        return colors[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1];
    }

    void set_color(int v, int x, int y, int z)
    {
        CHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
        CHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
        CHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
        colors[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1] = v;
    }

    int get_color(Point &p)
    {
        return get_color(p.x, p.y, p.z);
    }

    void set_color(int v, Point &p)
    {
        set_color(v, p.x, p.y, p.z);
    }

    vvv convert()
    {
        vvv ret = vvv(R, vv(R, v(R, 0)));
        for (int i = 0; i < R; i++)
            for (int j = 0; j < R; j++)
                for (int k = 0; k < R; k++)
                {
                    ret[i][j][k] = get_color(i, j, k) >= 0 ? 1 : 0;
                }
        return ret;
    }

    void dfs(int color, int x, int y, int z, int dir, int base)
    {
        if (!get(x, y, z) || get_color(x, y, z) >= 0)
        {
            return;
        }
        if (y != 0 && get_color(x - dx[dir], y - dy[dir], z - dz[dir]) < 0)
        {
            return;
        }
        set_color(color, x, y, z);
        if (dir / 2 == 0 && base == x)
        {
            dfs(color, x, y - 1, z, dir, base);
            dfs(color, x, y + 1, z, dir, base);
            dfs(color, x, y, z - 1, dir, base);
            dfs(color, x, y, z + 1, dir, base);
        }
        else if (dir / 2 == 1 && base == y)
        {
            dfs(color, x - 1, y, z, dir, base);
            dfs(color, x + 1, y, z, dir, base);
            dfs(color, x, y, z - 1, dir, base);
            dfs(color, x, y, z + 1, dir, base);
        }
        else if (dir / 2 == 2 && base == z)
        {
            dfs(color, x - 1, y, z, dir, base);
            dfs(color, x + 1, y, z, dir, base);
            dfs(color, x, y - 1, z, dir, base);
            dfs(color, x, y + 1, z, dir, base);
        }
        dfs(color, x + dx[dir], y + dy[dir], z + dz[dir], dir, base);
    }

    void set_colors() {
        int gcount = 0;
        for (int i = 0; i < R; i++) for (int j = 0; j < R; j++) {
            if (get(i, 0, j) && get_color(i, 0, j) < 0) {
                dfs(gcount, i, 0, j, UP_Y, 0);
                g2d[gcount] = UP_Y;
                gcount++;
            }
        }

        while (1) {
            int prev = gcount;
            for (int d = 0; d < 6; d++) for (int i = 0; i < R; i++) for (int j = 0; j < R; j++) for (int k = 0; k < R; k++) {
                if (get(i, j, k) && get_color(i, j, k) < 0 && get_color(i - dx[d], j - dy[d], k - dz[d]) >= 0) {
                    if (d / 2 == 0) {
                        dfs(gcount, i, j, k, d, i);
                        g2d[gcount] = d;
                        gcount++;
                    } else if (d / 2 == 1) {
                        dfs(gcount, i, j, k, d, j);
                        g2d[gcount] = d;
                        gcount++;
                    } else {
                        dfs(gcount, i, j, k, d, k);
                        g2d[gcount] = d;
                        gcount++;
                    }
                }
            }
            if (prev == gcount)
                break;
        }
        color_count = gcount;
    }
};

class OscarAI : public AI
{
    vvv tgt_model;
    Vox vox;

  public:
    OscarAI(const vvv &src_model, const vvv &tgt_model) : AI(src_model), tgt_model(tgt_model), vox(src_model) { }
    ~OscarAI() override = default;
    void Run() override {
        vox.set_colors();

        priority_queue<pair<int, Point>, vector<pair<int, Point>>, greater<pair<int, Point>>> pque;

        const int R = tgt_model.size();

        for (size_t x = 0; x < tgt_model.size(); x++) {
            for (size_t z = 0; z < tgt_model[x][0].size(); z++) {
                if (tgt_model[x][0][z] == 1) {
                    pque.push(make_pair(x + z, Point(x, 0, z)));
                }
            }
        }

        vector<Point> visit_order;
        visit_order.emplace_back(0, 0, 0);

        vvv visited(R, vv(R, v(R, 0)));

        while (!pque.empty()) {
            const Point cur = pque.top().second;
            pque.pop();

            if (visited[cur.x][cur.y][cur.z]) {
                continue;
            }

            visited[cur.x][cur.y][cur.z] = 1;
            visit_order.push_back(cur);

            for (int i = 0; i < 6; i++) {
                int nx = cur.x + dx[i];
                int ny = cur.y + dy[i];
                int nz = cur.z + dz[i];
                if (nx >= 0 && ny >= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
                    if (tgt_model[nx][ny][nz] == 1 && !visited[nx][ny][nz]) {
                        pque.push(make_pair(nx + ny + nz, Point(nx, ny, nz)));
                    }
                }
            }
        }

        visit_order.emplace_back(0, 0, 0);

        evvv voxel_states(R, evv(R, ev(R, MyVoxelState::kALWAYSEMPTY)));
        for (int x = 0; x < R; ++x) {
            for (int y = 0; y < R; ++y) {
                for (int z = 0; z < R; ++z) {
                    if (tgt_model[x][y][z]) {
                        voxel_states[x][y][z] = MyVoxelState::kSHOULDBEFILLED;
                    }
                }
            }
        }

        LOG(INFO) << "start path construction";

        int total_move = 0;
        vector<Command> result_buff;

        for (size_t i = 0; i + 1 < visit_order.size(); ++i) {
            const auto cur = visit_order[i];
            const auto &next = visit_order[i + 1];

            const auto commands = get_commands_for_next(cur, next, voxel_states);

            total_move += commands.size();

            result_buff.push_back(commands[0]);
            if (i > 0) {
                const Point &nd = commands[0].smove_.lld;
                result_buff.push_back(Command::make_fill(1, Point(-nd.x, -nd.y, -nd.z)));
                result_buff = MergeSMove(result_buff);
                for (auto c : result_buff) {
                    ce->Execute({c});
                }
                result_buff.clear();
                voxel_states[cur.x][cur.y][cur.z] = MyVoxelState::kALREADYFILLED;
            }

            for (size_t i = 1; i < commands.size(); ++i)
            {
                result_buff.push_back(commands[i]);
            }
        }

        result_buff.push_back(Command::make_halt(1));
        result_buff = MergeSMove(result_buff);
        for (auto c : result_buff) {
            ce->Execute({c});
            result_buff.clear();
        }

        LOG(INFO) << "done path construction R=" << R
                  << " total_visit=" << total_visit
                  << " total_move=" << total_move
                  << " move_per_voxel=" << static_cast<double>(total_move) / (visit_order.size() - 2)
                  << " visit_per_voxel=" << static_cast<double>(total_visit) / (visit_order.size() - 2);

    }
};

DEFINE_string(src_filename, "", "filepath of src");
DEFINE_string(tgt_filename, "", "filepath of tgt");

int main(int argc, char *argv[])
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_src_filename.empty() && FLAGS_tgt_filename.empty())
    {
        std::cout << "need to pass --mdl_filename=/path/to/mdl";
        exit(1);
    }


    int R = 1;
    vvv src_model;
    if (!FLAGS_src_filename.empty()) {
        src_model = ReadMDL(FLAGS_src_filename);
        R = src_model.size();
    }

    vvv tgt_model;
    if(!FLAGS_tgt_filename.empty()) {
        tgt_model = ReadMDL(FLAGS_tgt_filename);
        R = tgt_model.size();
    }

    if (FLAGS_src_filename.empty()) {
        LOG(INFO) << "Start with empty src";
        src_model = vvv(R, vv(R, v(R, 0)));
    }
    if (FLAGS_tgt_filename.empty()) {
        LOG(INFO) << "Start with empty tgt";
        tgt_model = vvv(R, vv(R, v(R, 0)));
    }
    LOG(INFO) << "R: " << R;
    auto oscar_ai = std::make_unique<OscarAI>(src_model, tgt_model);
    oscar_ai->Run();
    oscar_ai->Finalize();
}
