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
