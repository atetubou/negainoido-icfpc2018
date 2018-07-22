#include "src/base/base.h"
#include "src/command.h"
#include "glog/logging.h"
#include "oscar.h"
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <set>

using bv = std::vector<bool>;
using bvv = std::vector<bool>;
using bvvv = std::vector<bool>;

using namespace std;

static int dx[] = {-1,1,0,0,0,0};
static int dy[] = {0,0,-1,1,0,0};
static int dz[] = {0,0,0,0,-1,1};

#define DOWN_X 0
#define UP_X 1
#define DOWN_Y 2
#define UP_Y 3
#define DOWN_Z 4
#define UP_Z 5

Vox::Vox(const vvv &voxels)
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

void Vox::add_color(int dir) {
  g2d[color_count] = dir;
  max_pos[color_count] = dir%2==1 ? 0 : R;
  color_count++;
}
bool Vox::get(int x, int y, int z)
{
  CHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
  CHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
  CHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
  return voxels[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1];
}

int Vox::get_color(int x, int y, int z)
{
  CHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
  CHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
  CHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
  return colors[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1];
}

void Vox::set_color(int v, int x, int y, int z)
{
  CHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
  CHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
  CHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
  colors[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1] = v;
}

int Vox::get_color(Point &p)
{
  return get_color(p.x, p.y, p.z);
}

void Vox::set_color(int v, Point &p)
{
  set_color(v, p.x, p.y, p.z);
}

vvv Vox::convert()
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

void Vox::dfs(int color, int x, int y, int z, int dir, int base)
{
  if ((!get(x, y, z) || get_color(x, y, z) >= 0) || (y != 0 && get(x-dx[dir], y-dy[dir],z-dz[dir]) && get_color(x - dx[dir], y - dy[dir], z - dz[dir]) < 0))
  {
    if (dir/2 ==0) {
      max_pos[color] = dir%2 == 1 ? max(max_pos[color], x-1) : min(max_pos[color], x+1);
    } else if (dir/2 ==1) {
      max_pos[color] = dir%2 == 1 ? max(max_pos[color], y-1) : min(max_pos[color], y+1);
    } else {
      max_pos[color] = dir%2 == 1 ? max(max_pos[color], z-1) : min(max_pos[color], z+1);
    }
    return;
  }
  set_color(color, x, y, z);
  if (dir / 2 == 0)
  {
    dfs(color, x, y - 1, z, dir, base);
    dfs(color, x, y + 1, z, dir, base);
    dfs(color, x, y, z - 1, dir, base);
    dfs(color, x, y, z + 1, dir, base);
  }
  else if (dir / 2 == 1)
  {
    dfs(color, x - 1, y, z, dir, base);
    dfs(color, x + 1, y, z, dir, base);
    dfs(color, x, y, z - 1, dir, base);
    dfs(color, x, y, z + 1, dir, base);
  }
  else if (dir / 2 == 2)
  {
    dfs(color, x - 1, y, z, dir, base);
    dfs(color, x + 1, y, z, dir, base);
    dfs(color, x, y - 1, z, dir, base);
    dfs(color, x, y + 1, z, dir, base);
  }
  dfs(color, x + dx[dir], y + dy[dir], z + dz[dir], dir, base);
}

void Vox::set_colors()
{
  for (int i = 0; i < R; i++) for (int j = 0; j < R; j++) {
    if (get(i, 0, j) && get_color(i, 0, j) < 0) {
      add_color(UP_Y);
      dfs(color_count-1, i, 0, j, UP_Y, 0);
    }
  }

  while (1)
  {
    int prev = color_count;
    for (int d = 0; d < 6; d++) for (int i = 0; i < R; i++) for (int j = 0; j < R; j++) for (int k = 0; k < R; k++) {
      if (get(i, j, k) && get_color(i, j, k) < 0 && get_color(i - dx[d], j - dy[d], k - dz[d]) >= 0) {
        if (d / 2 == 0)
        {
          add_color(d);
          dfs(color_count-1, i, j, k, d, i);
        }
        else if (d / 2 == 1)
        {
          add_color(d);
          dfs(color_count-1, i, j, k, d, j);
        }
        else
        {
          add_color(d);
          dfs(color_count-1, i, j, k, d, k);
        }
      }
    }
    if (prev == color_count)
      break;
  }
  LOG(INFO) << "used color :" << color_count;

  for (int i=0;i<R;i++) for (int j=0;j<R;j++) for (int k=0;k<R;k++) {
    CHECK(!get(i,j,k) || get_color(i,j,k) >= 0) << "found missing voxel: " << Point(i,j,k);
  }
}

int Vox::get_color_count() {
  return color_count;
}