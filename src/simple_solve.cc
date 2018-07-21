#include "json/json.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "command_util.h"
#include "command.h"
#include <queue>
#include <cmath>
#include <vector>
#include <iostream>
#include <utility>
#include <algorithm>

using namespace std;

int dx[] = {-1,0,0,1,0,0};
int dy[] = {0,-1,0,0,1,0};
int dz[] = {0,0,-1,0,0,1};

enum class VoxelState {
  kALWAYSEMPTY,
  kSHOULDBEFILLED,
  kALREADYFILLED,
};

using ev = std::vector<VoxelState>;
using evv = std::vector<ev>;
using evvv = std::vector<evv>;

int total_visit = 0;

vector<Command> get_commands_for_next(const Point& current, const Point& dest, 
                                      const evvv& voxel_states) {
  const int R = voxel_states.size();
  queue<Point> que;
  int count = 1;

  static vvv tmp_map(R, vv(R, v(R, false)));
  
  vector<Point> visited;

  que.push(Point(current.x, current.y, current.z));

  while(!que.empty()) {
    const Point tar = que.front();
    que.pop();

    if (tmp_map[tar.x][tar.y][tar.z] > 0) continue;
    tmp_map[tar.x][tar.y][tar.z]  = count;
    visited.push_back(tar);
    ++total_visit;
    count++;

    if (dest == tar) {
      break;
    }

    for(int i=0;i<6;i++) {
      int nx = tar.x + dx[i];
      int ny = tar.y + dy[i];
      int nz = tar.z + dz[i];
      if (nx >= 0 && ny>= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
        if (voxel_states[nx][ny][nz] != VoxelState::kALREADYFILLED) {
          Point np = Point(nx,ny,nz);
          que.push(np);
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
        if (minc > tmpv && tmpv > 0) {
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
    tmp_map[v.x][v.y][v.z] = 0;
  }

  return rev_commands;
}

DEFINE_string(mdl_filename, "", "filepath of mdl");
DEFINE_bool(json, false, "output command in json if --json is set");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  
  const vvv voxels = ReadMDL(FLAGS_mdl_filename);

  priority_queue<pair<int, Point>, vector< pair<int, Point> >, greater< pair<int, Point> > > pque;

  const int R = voxels.size();

  for (size_t x = 0; x < voxels.size(); x++) {
    for(size_t z = 0; z < voxels[x][0].size(); z++) {
      if (voxels[x][0][z] == 1) {
        pque.push(make_pair(x + z, Point(x, 0, z)));
      }
    }
  }

  vector<Command> results;

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

  evvv voxel_states(R, evv(R, ev(R, VoxelState::kALWAYSEMPTY)));
  for (int x = 0; x < R; ++x) {
    for (int y = 0; y < R; ++y) {
      for (int z = 0; z < R; ++z) {
        if (voxels[x][y][z]) {
          voxel_states[x][y][z] = VoxelState::kSHOULDBEFILLED;
        }
      }
    }
  }
  
  LOG(INFO) << "start path construction";

  for (size_t i = 0; i + 1 < visit_order.size(); ++i) {
    const auto cur = visit_order[i];
    const auto& next = visit_order[i + 1];

    const auto commands = get_commands_for_next(cur, next, voxel_states);

    results.push_back(commands[0]);
    if (i > 0) {
      const Point& nd = commands[0].smove_lld;
      results.push_back(Command::make_fill(1, Point(-nd.x, -nd.y, -nd.z)));
      voxel_states[cur.x][cur.y][cur.z] = VoxelState::kALREADYFILLED;
    }

    for (size_t i = 1; i < commands.size(); ++i) {
      results.push_back(commands[i]);
    }
  }

  results.push_back(Command::make_halt(1));

  LOG(INFO) << "done path construction total_visit " << total_visit;

  Json::Value json;  
  json["turn"].append(Command::CommandsToJson(results));

  if (FLAGS_json) {
    cout << json << endl;
  } else {
    cout << Json2Binary(json);
  }

  return 0;
}
