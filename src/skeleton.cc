// vim-compile: cd .. && bazel run //src:skeleton -- --mdl_filename=$PWD/a.mdl
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <deque>
using namespace std;

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "src/command_util.h"
#include "src/command_executer.h"

DEFINE_string(mdl_filename, "", "filepath of mdl");

using Path = vector<Point>;

Point find_one_basepoint(const vvv &M, int diff = 1) { // return lowest point if diff == 1; return highest point if diff == -1
  int R = (int)M.size();
  int y0 = diff == 1 ? 0 : R-1;
  for(int y=y0; 0 <= y && y<R; y += diff) {
    for (int x=0;x<R;++x) {
      for (int z=0;z<R;++z) {
        if (M[x][y][z]) {
          return Point(x, y, z);
        }
      }
    }
  }
  return Point(-1, -1, -1); // error
}

vvv PointsToVVV(const Path &path, int R = -1) {
  if (R == -1) {
    for(const auto &v : path) {
      R = max(R, v.x + 1);
      R = max(R, v.y + 1);
      R = max(R, v.z + 1);
    }
  }
  vvv M(R, vv(R, v(R, 0)));
  for(const auto &v : path) {
    M[v.x][v.y][v.z] = 1;
  }
  return M;
}

vvv InitVVV(int R) {
  return vvv(R, vv(R, v(R, 0)));
}

class Skeleton {
  vvv M;
  int R;

  CommandExecuter ce;

  vector<vvv> Ms; // remaining vertices
  vector<Path> skeletons; // extracted skeletons so far

  vector<Path> skeleton; // final skeleton
public:
  Skeleton(const vvv &M) : M(M), R((int)M.size()), ce((int)M.size(), true) {}

  Path extract_a_skeleton(Point s) {
    queue<Point> q;
    map<Point, int> dist;
    map<Point, Point> prev;
    q.push(s);
    dist[s] = 0;
    Path path;
    int target_y = find_one_basepoint(M, -1).y;
    while(!q.empty()) {
      Point v = q.front();
      q.pop();
      int d = dist[v];
      if (v.y == target_y) {
        while(s != v) {
          path.push_back(v);
//          cout << v.x << " " << v.y << " " << v.z << endl;
          v = prev[v];
        }
        path.push_back(s);
        return path;
      }
      for(int dx=-1; dx<=1; dx++) {
        for(int dy=-1; dy<=1; dy++) {
          for(int dz=-1; dz<=1; dz++) {
            if (abs(dx) + abs(dy) + abs(dz) != 1) continue;
            Point w = v + Point(dx, dy, dz);
            if (w.x < 0 || R <= w.x || w.y < 0 || R <= w.y || w.z < 0 || R <= w.z) continue;
            if (!M[w.x][w.y][w.z]) continue;
            if (dist.count(w)) continue;
            dist[w] = d + 1;
            prev[w] = v;
            q.push(w);
          }
        }
      }
    }
    cerr << "FATAL: disconnected!?" << endl;
    exit(1);
  }


  void Enbody(const vector<Point> &points) { // nikuzuke
    vvv rank = PointsToVVV(points, R);

    queue<Point> q;
    for(const auto &v : points) {
      q.push(v);
    }
    while(!q.empty()) {
      Point v = q.front();
      q.pop();
      int d = rank[v.x][v.y][v.z];
      for(int dx=-1; dx<=1; dx++) {
        for(int dz=-1; dz<=1; dz++) {
          if (max(abs(dx), abs(dz)) != 1) continue;
          Point w = v + Point(dx, 0, dz);
          if (w.x < 0 || R <= w.x || w.y < 0 || R <= w.y || w.z < 0 || R <= w.z) continue;
          if (!M[w.x][w.y][w.z]) continue;
          if (rank[w.x][w.y][w.z]) continue;
          rank[w.x][w.y][w.z] = d + 1;
          q.push(w);
        }
      }
    }

    for(int y=0; y<R; y++) {
      for(int x=0; x<R; x++) {
        for(int z=0; z<R; z++) {
          if (rank[x][y][z])
            rank[x][y][z] = rank[x][y][z] * R + y;
        }
      }
    }

    FillAccordingToRank(rank, InitVVV(R), Point(0, 0, 0));


    OutputMDL(rank);
  }

  void FillAccordingToRank(const vvv &rank, vvv wall, Point pos) {
    deque<int> ranks;
    for(int y=0; y<R; y++) {
      for(int x=0; x<R; x++) {
        for(int z=0; z<R; z++) {
          if (rank[x][y][z])
            ranks.push_back(rank[x][y][z]);
        }
      }
    }
    sort(ranks.begin(), ranks.end());

    const Point fill_here(-1, -1, -1);
    Path cmds;
    while(!ranks.empty()) {
      int target_rank = ranks.front();
      ranks.pop_front();

      map<Point, Point> prev;
      queue<Point> q;
      q.push(pos);
      prev[pos] = pos;
      while(!q.empty()) {
        Point v = q.front();
        q.pop();
        if (rank[v.x][v.y][v.z] == target_rank) {
          Path path;
          Point w = v;
          while(w != pos) {
            path.push_back(w);
            w = prev[w];
          }
          path.push_back(pos);
          reverse(path.begin(), path.end());
          cmds.insert(cmds.end(), path.begin(), path.end()); // append path
          cmds.push_back(fill_here);

          wall[v.x][v.y][v.z] = 1; // fill
          pos = v;

          break;
        }
        for(int dx=-1; dx<=1; dx++) {
          for(int dy=-1; dy<=1; dy++) {
            for(int dz=-1; dz<=1; dz++) {
              if (abs(dx) + abs(dy) + abs(dz) != 1) continue;
              Point w = v + Point(dx, dy, dz);
              if (w.x < 0 || R <= w.x || w.y < 0 || R <= w.y || w.z < 0 || R <= w.z) continue;
              if (wall[w.x][w.y][w.z]) continue;
              if (prev.count(w)) continue;
              prev[w] = v;
              q.push(w);
            }
          }
        }
      }
    }

    for(int i=0; i<(int)cmds.size(); i++) {
      cout << cmds[i] << endl;
    }


//    ce.SMove(1, Point(1, 0, 0));

  }

};

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_mdl_filename.empty()) {
    std::cout << "need to pass --mdl_filename=/path/to/mdl";
    exit(1);
  }

  vvv M = ReadMDL(FLAGS_mdl_filename);
  WriteMDL(FLAGS_mdl_filename + "_dbl", M); // should be the same file
  int R = (int)M.size();
//  OutputMDL(M);

  auto skeleton = std::make_unique<Skeleton>(M);
  Path path = skeleton->extract_a_skeleton(find_one_basepoint(M));
  skeleton->Enbody(path);
//  OutputMDL(PointsToVVV(path, R));
}
