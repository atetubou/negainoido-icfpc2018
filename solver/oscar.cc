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
  this->par = vector<int>(1, 0);
  for (int i = 0; i < R; i++)
    for (int j = 0; j < R; j++)
      for (int k = 0; k < R; k++)
      {
        this->voxels[(i + 1) * (R + 2) * (R + 2) + (j + 1) * (R + 2) + k + 1] = voxels[i][j][k] == 1;
      }
}

int Vox::add_color() {
  int res = par.size();
  par.push_back(res);
  return res;
}

int Vox::merge(int l, int r) {
  if(l==-1 || r==-1) {
    DLOG(INFO) << "failed union";
    return -1;
  }
  if(l==r) return l;
  if(l < r) {
    par[r] = l;
    return l;
  }
  else {
    par[l] = r;
    return r;
  }
}

int Vox::get_parent(int c) {
  int p = par[c];
  if(p==c) {
    return c;
  }
  p = get_parent(p);
  par[c] = p;
  return p;
}

int Vox::get_parent_color(int x, int y, int z) {
  int color = get_color(x,y,z);
  return get_parent(color);
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
  DVLOG(2) << "set" << Point(x,y,z);
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
  DVLOG(2) << "set_color" << Point(x,y,z);
  DCHECK(x >= -1 && x < R + 1) << "out of range x: " << x << " R: " << R;
  DCHECK(y >= -1 && y < R + 1) << "out of range y: " << y << " R: " << R;
  DCHECK(z >= -1 && z < R + 1) << "out of range z: " << z << " R: " << R;
  colors[(x + 1) * (R + 2) * (R + 2) + (y + 1) * (R + 2) + z + 1] = v;
  for (int d=0;d<6;d++) {
    int nx = x+dx[d];
    int ny = y+dy[d];
    int nz = z+dz[d];
    if(get(nx,ny,nz)) {
      int nc = get_color(nx,ny,nz);
      int res = merge(nc, v);
      DCHECK(res!=-1) << "failed to union at " << Point(nx,ny,nz) << "with " << Point(x,y,z);
    }
  }
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