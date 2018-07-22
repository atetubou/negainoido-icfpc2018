#include "json/json.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "src/command_util.h"
#include "src/command.h"
#include "simple_solve.h"
#include <queue>
#include <cmath>
#include <vector>
#include <iostream>
#include <utility>
#include <algorithm>

using namespace std;

static int dx[] = {-1,0,0,1,0,0};
static int dy[] = {0,-1,0,0,1,0};
static int dz[] = {0,0,-1,0,0,1};

DEFINE_string(mdl_filename, "", "filepath of mdl");

void flush_commands(vector<Command> &results) {
  Json::Value json;
  for (const auto& c : results) {
    std::vector<Command> commands = {c};
    if (c.type == Command::LMOVE) {
    }
    json["turn"].append(Command::CommandsToJson(commands));
  }

  cout << Json2Binary(json);
  results.clear();
}

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
      result_buff = MergeSMove(result_buff);
      flush_commands(result_buff);
      voxel_states[cur.x][cur.y][cur.z] = MyVoxelState::kALREADYFILLED;
    }

    for (size_t i = 1; i < commands.size(); ++i) {
      result_buff.push_back(commands[i]);
    }
  }

  result_buff.push_back(Command::make_halt(1));
  result_buff = MergeSMove(result_buff);
  flush_commands(result_buff);

  LOG(INFO) << "done path construction R=" << R
            << " total_move=" << total_move;


  return 0;
}
