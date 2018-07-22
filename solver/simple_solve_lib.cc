#include "simple_solve.h"
#include "glog/logging.h"

#include <vector>
#include <queue>

using namespace std;

static int dx[] = {-1,0,0,1,0,0};
static int dy[] = {0,-1,0,0,1,0};
static int dz[] = {0,0,-1,0,0,1};

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

std::vector<Command> SimpleSolve(const vvv& voxels) {
  priority_queue<pair<int, Point>, vector< pair<int, Point> >, greater< pair<int, Point> > > pque;

  const int R = voxels.size();

  for (size_t x = 0; x < voxels.size(); x++) {
    for(size_t z = 0; z < voxels[x][0].size(); z++) {
      if (voxels[x][0][z] == 1) {
        pque.push(make_pair(x + z, Point(x, 0, z)));
      }
    }
  }

  vector<Point> visit_order;
  visit_order.emplace_back(0, 0, 0);

  vvv visited(R, vv(R, v(R, 0)));

  while(!pque.empty()) {
    const Point cur = pque.top().second;
    pque.pop();

    if (visited[cur.x][cur.y][cur.z]) {
      continue;
    }

    visited[cur.x][cur.y][cur.z] = 1;
    visit_order.push_back(cur);

    for(int i=0;i<6;i++) {
      int nx = cur.x + dx[i];
      int ny = cur.y + dy[i];
      int nz = cur.z + dz[i];
      if (nx >= 0 && ny>= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
        if (voxels[nx][ny][nz] == 1 && !visited[nx][ny][nz]) {
          pque.push(make_pair(nx + ny + nz, Point(nx,ny,nz)));
        }
      }
    }
  }

  visit_order.emplace_back(0, 0, 0);

  evvv voxel_states(R, evv(R, ev(R, MyVoxelState::kALWAYSEMPTY)));
  for (int x = 0; x < R; ++x) {
    for (int y = 0; y < R; ++y) {
      for (int z = 0; z < R; ++z) {
        if (voxels[x][y][z]) {
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
    const auto& next = visit_order[i + 1];

    const auto commands = get_commands_for_next(cur, next, voxel_states);

    total_move += commands.size();

    result_buff.push_back(commands[0]);
    if (i > 0) {
      const Point& nd = commands[0].smove_.lld;
      result_buff.push_back(Command::make_fill(1, Point(-nd.x, -nd.y, -nd.z)));
      voxel_states[cur.x][cur.y][cur.z] = MyVoxelState::kALREADYFILLED;
    }

    for (size_t i = 1; i < commands.size(); ++i) {
      result_buff.push_back(commands[i]);
    }
  }

  result_buff.push_back(Command::make_halt(1));
  result_buff = MergeSMove(result_buff);

  LOG(INFO) << "done path construction R=" << R
            << " total_move=" << total_move;

  return result_buff;
}
