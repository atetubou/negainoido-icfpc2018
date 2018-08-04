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

#define FOR(i,a,b) for(int i=(a);i<(int)(b);i++)
#define REP(i,n)  FOR(i,0,n)
#define INR(x,R) (0 <= (x) && (x) < (R))

bool VTarget::in(int x, int z) {
  return this->x <= x && x < this->ex && this->z <= z && z < this->ez;
}

bool VBot::inNextTarget(int x, int z) {
  CHECK(!reserved.empty()) << "task is empty";
  auto tar = reserved.front();
  return tar.x <= x && x < tar.ex && tar.z <= z && z < tar.ez;
}

bool VArea::checkReserve(int id, int x, int y) {
  if (x < 0 || x>=R || y < 0 || y>=R) return false;
  if (get(x, y) == -1 || get(x,y) == id) {
    return true;
  }
  return false;
}

bool VArea::reserve(int id, int x, int y) {
  if(checkReserve(id, x, y)) {
    DLOG(INFO) << "reserve: " << id << " " << x << " " << y ;
    area[x * R + y] = id;
    return true;
  }
  DLOG(INFO) << "failed to reserve: " << id << " " << x << " " <<y << " " << get(x,y);
  return false;
}

bool VArea::reserve(int id, const VTarget &tar) {
  FOR(i,tar.x, tar.ex) FOR(j, tar.z, tar.ez) {
    if (!checkReserve(id, i, j)) return false;
  }
  FOR(i,tar.x, tar.ex) FOR(j, tar.z, tar.ez) {
    reserve(id,i,j);
  }
  return true;
}

void VArea::free(int x, int y) {
  DLOG(INFO) << "free " << " " << x << " " << y ;
  release.insert(make_pair(x,y));
}

void VArea::freeArea(const VBot &bot, const VTarget &tar) {
  FOR(i,tar.x, tar.ex) FOR(j, tar.z, tar.ez) {
    if (bot.pos.x != i || bot.pos.z != j) free(i,j);
  }
}

int VArea::get(int x, int y) {
  if (x < 0 || x>=R || y < 0 || y>=R) return 1000;
  return area[x * R + y];
}

void VArea::runFree() {
  for (auto &tar : release) {
    area[tar.first * R + tar.second] =  -1;
  }
  release.clear();
}

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
    DCHECK(false) << "failed union";
    return -1;
  }
  l = get_parent(l);
  r = get_parent(r);
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