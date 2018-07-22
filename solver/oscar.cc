#include "src/base/base.h"
#include "src/command.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <set>

using namespace std;

using bv = std::vector<bool>;
using bvv = std::vector<bool>;
using bvvv = std::vector<bool>;

DEFINE_string(src_filename, "", "filepath of mdl");
DEFINE_string(out_filename, "", "filepath of mdl");

static int dx[] = {-1,1,0,0,0,0};
static int dy[] = {0,0,-1,1,0,0};
static int dz[] = {0,0,0,0,-1,1};

static Point dP[] = {
  Point(-1,0,0),
  Point(1,0,0),
  Point(0,-1,0),
  Point(0,1,0),
  Point(0,0,-1),
  Point(0,0,1),
};

#define DOWN_X 0
#define UP_X 1
#define DOWN_Y 2
#define UP_Y 3
#define DOWN_Z 4
#define UP_Z 5


struct Vox {
  vector<bool> voxels;
  vector<int> colors;
  int R;

  Vox(vvv voxels) {
    R = voxels.size();
    this->voxels.resize((R+2)*(R+2)*(R+2), false);
    this->colors.resize((R+2)*(R+2)*(R+2), -1);
    for (int i=0;i < R; i++) for (int j=0; j < R; j++) for (int k=0;k < R; k++) {
      this->voxels[(i+1)*(R+2)*(R+2) + (j+1) * (R+2) + k+1] = voxels[i][j][k] == 1;
    }
  }
  bool get(int x, int y, int z) {
    CHECK(x >= -1 && x < R+1) << "out of range x: " << x << " R: " << R;
    CHECK(y >= -1 && y < R+1) << "out of range y: " << y << " R: " << R;
    CHECK(z >= -1 && z < R+1) << "out of range z: " << z << " R: " << R;
    return voxels[(x+1)*(R+2)*(R+2) + (y+1) * (R+2) + z+1];
  }

  int get_color(int x, int y, int z) {
    CHECK(x >= -1 && x < R+1) << "out of range x: " << x << " R: " << R;
    CHECK(y >= -1 && y < R+1) << "out of range y: " << y << " R: " << R;
    CHECK(z >= -1 && z < R+1) << "out of range z: " << z << " R: " << R;
    return colors[(x+1)*(R+2)*(R+2) + (y+1) * (R+2) + z+1];
  }

  void set_color(int v, int x, int y, int z) {
    CHECK(x >= -1 && x < R+1) << "out of range x: " << x << " R: " << R;
    CHECK(y >= -1 && y < R+1) << "out of range y: " << y << " R: " << R;
    CHECK(z >= -1 && z < R+1) << "out of range z: " << z << " R: " << R;
    colors[(x+1)*(R+2)*(R+2) + (y+1) * (R+2) + z+1] = v;
  }

  int get_color(Point &p) {
    return get_color(p.x, p.y, p.z);
  }
    
  void set_color(int v, Point &p) {
    set_color(v, p.x, p.y, p.z);
  }

  vvv convert() {
    vvv ret = vvv(R, vv(R, v(R, 0)));
    for (int i=0;i < R; i++) for (int j=0; j < R; j++) for (int k=0;k < R; k++) {
      ret[i][j][k] = get_color(i,j,k) >= 0 ? 1 : 0;
    }
    return ret;
  }

  void dfs(int color, int x, int y, int z, int dir, int base) {
    if (!get(x,y,z) || get_color(x,y,z) >= 0) {
      return;
    }
    if (y!=0 && get_color(x-dx[dir], y-dy[dir], z-dz[dir]) < 0) {
      return;
    }
    set_color(color, x, y, z);
    if (dir/2 == 0 && base == x) {
      dfs(color, x, y-1, z, dir, base);
      dfs(color, x, y+1, z, dir, base);
      dfs(color, x, y, z-1, dir, base);
      dfs(color, x, y, z+1, dir, base);
    } else if (dir/2==1 && base == y) {
      dfs(color, x-1, y, z, dir, base);
      dfs(color, x+1, y, z, dir, base);
      dfs(color, x, y, z-1, dir, base);
      dfs(color, x, y, z+1, dir, base);
    } else if (dir/2==2 && base == z) {
      dfs(color, x-1, y, z, dir, base);
      dfs(color, x+1, y, z, dir, base);
      dfs(color, x, y-1, z, dir, base);
      dfs(color, x, y+1, z, dir, base);
    }
    dfs(color,x + dx[dir], y + dy[dir], z + dz[dir], dir, base);
  }
};


int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  
  const vvv voxels = ReadMDL(FLAGS_src_filename);
  const int R = voxels.size();
  Vox vox = Vox(voxels);

  queue<Point> que;

  map<int,int> g2d;
  
  int gcount = 0;
  for (int i = 0; i < R; i++) for (int j = 0; j < R; j++) {
    if (vox.get(i, 0, j) && vox.get_color(i,0,j) < 0) {
      vox.dfs(gcount, i, 0, j, UP_Y, 0);
      g2d[gcount] = UP_Y;
      gcount++;
    }
  }

  while(1) {
    int prev = gcount;
    for (int d=0;d<6;d++) for (int i=0;i<R;i++) for (int j=0;j<R;j++) for (int k=0;k<R;k++) {
      if (vox.get(i,j,k) && vox.get_color(i,j,k) < 0 && vox.get_color(i-dx[d],j-dy[d],k-dz[d]) >= 0) {
        if (d/2 == 0) {
          vox.dfs(gcount, i, j, k, d, i);
          g2d[gcount] = d;
          gcount++;
        } else if (d/2==1) {
          vox.dfs(gcount, i, j, k, d, j);
          g2d[gcount] = d;
          gcount++;
        } else {
          vox.dfs(gcount, i, j, k, d, k);
          g2d[gcount] = d;
          gcount++;
        }
      }
    }
    if (prev==gcount) break;
  }
  
  WriteMDL(FLAGS_out_filename, vox.convert());
  
  for(int i=0;i<R;i++) for(int j=0;j<R;j++) for (int k=0;k<R;k++) {
    CHECK(voxels[i][j][k]==0 || vox.get_color(i,j,k) >= 0) << "FAILED to fill :" << Point(i,j,k) << vox.get(i,j,k);
  }

  LOG(INFO) << "used color: " << gcount;
  return 0;
}