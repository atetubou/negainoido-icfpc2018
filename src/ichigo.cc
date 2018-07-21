#include "json/json.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "src/command_util.h"

#include <vector>
#include <stack>

#define rep(i, n) for(int i=0;i<(int)(n);++i)
using namespace std;

DEFINE_string(mdl_filename, "", "filepath of mdl");
DEFINE_bool(json, false, "output command in json if --json is set");

int adj_dx[] = {-1, 1, 0, 0, 0, 0};
int adj_dy[] = { 0, 0,-1, 1, 0, 0};
int adj_dz[] = { 0, 0, 0, 0,-1, 1};


vvv memo = vvv(256, vv(256, v(256, 0)));
bool is_grounded(vvv& M) {

    int R = M.size();
    rep(x, R) rep(y, R) rep(z, R)
        memo[x][y][z] = 0;
    int num = 0;
    rep(x, R) rep(y, R) rep(z, R)
        if (M[x][y][z]) num++;

    stack<Point> s;
    for (int x=0; x<R; ++x) {
        for (int z=0; z<R; ++z) {
            if (not M[x][0][z]) continue;
            s.push(Point(x, 0, z));
            memo[x][0][z] = 1;
            num -= 1;
        }
    }

    while (not s.empty()) {
        Point p = s.top(); s.pop();
        rep (k, 6) {
            int x = p.x + adj_dx[k];
            int y = p.y + adj_dy[k];
            int z = p.z + adj_dz[k];
            if (x < 0 || y < 0 || z < 0) continue;
            if (x >= R || y >= R || z >= R) continue;
            if (memo[x][y][z]) continue;
            if (not M[x][y][z]) continue;
            memo[x][y][z] = 1;
            num -= 1;
            s.push(Point(x, y, z));
        }
    }

    return (num == 0);
}



struct Area {
  int x0, x1;
  int y0, y1;
  int z0, z1;
  Area() = default;
  Area(int x0, int x1,
       int y0, int y1,
       int z0, int z1)
      : x0(x0), x1(x1), y0(y0), y1(y1), z0(z0), z1(z1) {}
};


struct Bot {
    Point p;
    int id;
    vector<int> seed;
    Area area;
    Bot() {
        p = Point(0, 0, 0);
        id = 1;
        for (int i=2; i<=20; ++i)
            seed.push_back(i);
    }
}


Point
get_closest_point(
        const Point&from,
        const Area& a,
        const vvv&N,
        const vvv&M)
{
    Point p;
    int mn = 10000000;
    vector<pair<int, Point>> result;
    for (int x = a.x0; x < a.x1; ++x) {
    for (int y = a.y0; y < a.y1; ++y) {
    for (int z = a.z0; z < a.z1; ++z) {
        if (not M[x][y][z]) continue;
        if (N[x][y][z]) continue;
        int d = abs(x - from.x)
            + abs(y - from.y) * 300
            + abs(z - from.z);
        if (d < mn) {
            mn = d;
            p = Point(x, y, z);
        }
    }}}
    return p;
}


int main(int argc, char** argv) {

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_mdl_filename.empty()) {
    std::cout << "need to pass --mdl_filename=/path/to/mdl\n";
    exit(1);
  }

  vvv M = ReadMDL(FLAGS_mdl_filename);
  int R = M.size();
  vvv N = vvv(R, vv(R, v(R, 0)));

  vector<Bot> bots;
  bots.push_back(Bot());
  {
      Area a(0, R, 0, R, 0, R);
      bots[0].area = a;
  }

  Json::Value result;

  while (true) {
      if (is_completed(N, M)) {
          break;
      }
      vector<Command> cs;
      for (Bot&bot: bots) {
          Command c;
          auto p = get_closest_point(bot.p, bot.area, N, M);
          if (is_near(bot.p, p)) {
              c = make_fill(bot.id, p);
          } else {
          }
          cs.push_back(c);
      }
  }

  return 0;
}
