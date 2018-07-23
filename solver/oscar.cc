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
  this->color_count = 0;
  for (int i = 0; i < R; i++)
    for (int j = 0; j < R; j++)
      for (int k = 0; k < R; k++)
      {
        this->voxels[(i + 1) * (R + 2) * (R + 2) + (j + 1) * (R + 2) + k + 1] = voxels[i][j][k] == 1;
      }
}

void Vox::add_color(int dir) {
  color_count++;
}
bool Vox::get(int x, int y, int z)
{
  DCHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
  DCHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
  DCHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
  return voxels[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1];
}

void Vox::set(bool v,int x, int y, int z)
{
  DCHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
  DCHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
  DCHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
  voxels[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1] = v;
}

int Vox::get_color(int x, int y, int z)
{
  DCHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
  DCHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
  DCHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
  return colors[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1];
}

void Vox::set_color(int v, int x, int y, int z)
{
  DCHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
  DCHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
  DCHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
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