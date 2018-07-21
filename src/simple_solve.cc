#include "json/json.h"
#include "gflags/gflags.h"

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

vector<Command> get_commands_for_next(Point &current, Point &dest, vvv &voxels) {
    int R = voxels.size();
    queue<Point> que;
    int count = 1;
    vvv tmp_map(R,vv(R,v(R,0)));
    que.push(Point(current.x, current.y, current.z));
    cout << "bfs current: (" << current.x << "," << current.y << "," << current.z << ")" << endl;
    cout << "bfs dest: (" << dest.x << "," << dest.y << "," << dest.z << ")" << endl;
    while(!que.empty()) {
      Point tar = que.front();
      que.pop();

      if (tmp_map[tar.x][tar.y][tar.z] > 0) continue;
      tmp_map[tar.x][tar.y][tar.z]  = count;   
      count++;

      if (dest == tar) {
        break;
      }

      for(int i=0;i<6;i++) {
        int nx = tar.x + dx[i];
        int ny = tar.y + dy[i];
        int nz = tar.z + dz[i];
        if (nx >= 0 && ny>= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
          if (voxels[nx][ny][nz] < 2) {
            Point np = Point(nx,ny,nz);
            que.push(np);
          }
        }
      }
    }
    vector<Command> rev_commands;
    Point rc = dest;
    while(rc != current) {
      cout << "Find prev for (" << rc.x << ","  << rc.y << "," << rc.z <<  ")" << endl;
      int mind = -1;
      int minc = tmp_map[rc.x][rc.y][rc.z];
      cout << "count for (" << rc.x << ","  << rc.y << "," << rc.z <<  "): "  << minc << endl;
      for(int i=0;i<6;i++) {
        int nx = rc.x + dx[i];
        int ny = rc.y + dy[i];
        int nz = rc.z + dz[i];
        if (nx >= 0 && ny>= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
          int tmpv = tmp_map[nx][ny][nz];
          cout << "count for (" << nx << ","  << ny << "," << nz <<  "): "  << tmpv << endl;
          if (minc > tmpv && tmpv > 0) {
            minc = tmpv;
            mind = i;
          }
        }
      }
      if (mind < 0) {
        cerr << "Error in BFS to find path" << endl;
        exit(1);
      }
      rc.x = rc.x + dx[mind];
      rc.y = rc.y + dy[mind];
      rc.z = rc.z + dz[mind];
      rev_commands.push_back(Command::make_smove(Point(-dx[mind], -dy[mind], -dz[mind])));
    }

    reverse(rev_commands.begin(), rev_commands.end());

    return rev_commands;
}

DEFINE_string(mdl_filename, "", "filepath of nbt");
int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  
  vvv tar_voxels = ReadMDL(FLAGS_mdl_filename);

  priority_queue<pair<int, Point>, vector< pair<int, Point> >, greater< pair<int, Point> > > pque;

  for (int x = 0; x < (int)tar_voxels.size(); x++)  for (int y = 0; y < (int)tar_voxels[x].size(); y++) for(int z = 0; z < (int) tar_voxels[y].size(); z++) {
    if (tar_voxels[x][y][z] == 1 && y == 0) {
      pque.push(make_pair(x+z, Point(x,0, z)));
    }
  }

  vector<Command> results;
  Point current = Point(0,0,0);  

  int R = tar_voxels.size();

  while(!pque.empty()) {
    Point tar = pque.top().second;
    int cost = pque.top().first;
    pque.pop();
    if (tar_voxels[tar.x][tar.y][tar.z] != 1) continue;
    tar_voxels[tar.x][tar.y][tar.z] = 2;

    cout << "try to fill (" << tar.x << "," << tar.y << "," << tar.z << ")" << endl;

    for(int i=0;i<6;i++) {
      int nx = tar.x + dx[i];
      int ny = tar.y + dy[i];
      int nz = tar.z + dz[i];
      if (nx >= 0 && ny>= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
        if (tar_voxels[nx][ny][nz] == 1) {
          pque.push(make_pair(cost+1, Point(nx,ny,nz)));
        }
      }
    }

    for (auto command : get_commands_for_next(current, tar, tar_voxels)) {
      results.push_back(command);
    }

    Point next;
    if (!pque.empty()) {
      next = pque.top().second;
    } else {
      for(int i=0;i<6;i++) {
        int nx = tar.x + dx[i];
        int ny = tar.y + dy[i];
        int nz = tar.z + dz[i];

        if (nx >= 0 && ny>= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
          if (tar_voxels[nx][ny][nz] == 0) {
            next = Point(nx,ny,nz);
            break;
          }
        }
        if (i==5) {
          cerr << "In space ship" << endl;
          exit(1);
        }
      }
    }

    vector<Command> ncs = get_commands_for_next(tar, next, tar_voxels);
    
    results.push_back(ncs[0]);
    Point nd = ncs[0].smove_lld;

    results.push_back(Command::make_fill(Point(-nd.x, -nd.y, -nd.z)));
    current = tar + nd;
  }

  return 0;
}
