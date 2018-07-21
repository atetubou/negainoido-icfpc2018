// vim-compile: cd .. && bazel run //src:skeleton -- --mdl_filename=/home/vagrant/icfpc/problems/LA049_tgt.mdl
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

const Point fill_here(-1, -1, -1);

class Skeleton {
  vvv M;
  int R;

  CommandExecuter ce;

  vvv wall;
  set<Point> grand;
public:
  Skeleton(const vvv &M) : M(M), R((int)M.size()), ce((int)M.size(), true) {
    wall = InitVVV(R);

    // add grand
    for(int x=0; x<R; x++) {
      for(int z=0; z<R; z++) {
        if (M[x][0][z]) grand.insert(Point(x, 0, z));
      }
    }
  }

  Path extract_a_skeleton() {
    if (grand.empty()) {
      cerr << "No grand vertex!" << endl;
      exit(1);
    }
    Point s = *grand.begin();

    queue<Point> q;
    map<Point, int> dist;
    map<Point, Point> prev;
    q.push(s);
    dist[s] = 0;
    Path path;
    int maxy = 0, miny = R;
    while(!q.empty()) {
      Point v = q.front();
      q.pop();
      int d = dist[v];
      maxy = max(maxy, v.y);
      miny = min(miny, v.y);
      for(int dx=-1; dx<=1; dx++) {
        for(int dy=-1; dy<=1; dy++) {
          for(int dz=-1; dz<=1; dz++) {
            if (abs(dx) + abs(dy) + abs(dz) != 1) continue;
            if (dy < 0) continue; // may be bad?
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

    bool usemax = abs(s.y - miny) < abs(s.y - maxy);
    for(int y=0; y<R; y++) {
      for(int x=0; x<R; x++) {
        for(int z=0; z<R; z++) {
          Point v(x, y, z);
          if (!dist.count(v)) continue;
          bool ok = false;
          if (v.y == miny && !usemax) {
            ok = true;
          }
          if (v.y == maxy && usemax) {
            ok = true;
          }
          if (ok) {
            while(s != v) {
              path.push_back(v);
              v = prev[v];
            }
            path.push_back(s);
            return path;
          }
        }
      }
    }
    cerr << "FATAL: Empty at extract a skeleton" << endl;
    exit(1);
  }

  bool IsMEmpty() {
    for(int y=0; y<R; y++) {
      for(int x=0; x<R; x++) {
        for(int z=0; z<R; z++) {
          if (M[x][y][z]) return false;
        }
      }
    }
    return true;
  }
  int CountM() {
      int mcount = 0;
      for(int y=0; y<R; y++) {
        for(int x=0; x<R; x++) {
          for(int z=0; z<R; z++) {
            mcount += M[x][y][z];
          }
        }
      }
      return mcount;
  }

  void extract_skeletons() {
    Point pos(0, 0, 0);
    vector<Point> movepointlist;
    int lastmcount = CountM();
    while(!IsMEmpty()) {
      Path path = extract_a_skeleton();
      cerr << "OK" << endl;
      vvv rank = RankingAccordingToPath(path);
      vector<Point> cmds = FillAccordingToRank(rank, pos);
      pos = cmds.back();
      movepointlist.insert(movepointlist.end(), cmds.begin(), cmds.end());

      int mcount = CountM();
      if (lastmcount == mcount) {
        cerr << "No progress! Aborting" << endl;
        break;
//        exit(1);
      }
      lastmcount = mcount;
      cerr << mcount << " " << grand.size() << endl; 
    }

    // move to the origin
    vector<Point> cmds = MoveFromTo(pos, Point(0, 0, 0));
    movepointlist.insert(movepointlist.end(), cmds.begin(), cmds.end());

    ExecMovePointList(movepointlist);
  }

  vvv RankingAccordingToPath(const vector<Point> &points) {
    // ranking voxel.  We regard nonzero ranking as a priority, and fill it in the order of priority.
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
          if (!M[w.x][w.y][w.z] || wall[w.x][w.y][w.z]) continue;
          if (rank[w.x][w.y][w.z]) continue;
          rank[w.x][w.y][w.z] = d + 1;
          q.push(w);
        }
      }
    }

    for(int y=0; y<R; y++) {
      for(int x=0; x<R; x++) {
        for(int z=0; z<R; z++) {
          if (rank[x][y][z] > 1)
            rank[x][y][z] = rank[x][y][z] + y * (R*R+1);
        }
      }
    }

    return rank;
  }

  vector<Point> FillAccordingToRank(vvv rank, Point from) {
    Point pos = from;
//    rank[0][0][0] = 1<<29; // return point
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
        Path path;
        Point w;

        q.pop();
        if (pos != v && rank[v.x][v.y][v.z] == target_rank && grand.count(v)) {
          w = v;
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

          M[v.x][v.y][v.z] = 0;
          grand.erase(v);
          for(int dx=-1; dx<=1; dx++) {
            for(int dy=-1; dy<=1; dy++) {
              for(int dz=-1; dz<=1; dz++) {
                if (abs(dx) + abs(dy) + abs(dz) != 1) continue;
                int ax = v.x + dx, ay = v.y + dy, az = v.z + dz;
                if (0 <= ax && ax < R && 0 <= ay && ay < R && 0 <= az && az < R && M[ax][ay][az]) {
                  grand.emplace(ax, ay, az);
                }
              }
            }
          }
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

    cmds.push_back(pos);
    return cmds;
  }

  Path MoveFromTo(const Point &from, const Point &to) {
    map<Point, Point> prev;
    queue<Point> q;
    q.push(from);
    prev[from] = from;
    while(!q.empty()) {
      Point v = q.front();
      q.pop();
      if (v == to) {
        Path path;
        Point w = v;
        while(w != from) {
          path.push_back(w);
          w = prev[w];
        }
        path.push_back(from);
        reverse(path.begin(), path.end());
        return path;
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
    cerr << "FATAL: could not back" << endl;
    exit(1);
  }



  void ExecMovePointList(const vector<Point> &cmds) {
    vector<Command> commands;
    Point pos;
    for(int i=0; i<(int)cmds.size(); i++) {
      if (cmds[i] == fill_here) {
        while((cmds[i] == fill_here || cmds[i] == pos) && i < (int)cmds.size()) i++;
        if (i >= (int)cmds.size() - 1) {
          break;
        } else {
          commands.emplace_back(Command::make_smove(1, cmds[i] - pos));
          commands.emplace_back(Command::make_fill(1, pos - cmds[i]));
          pos = cmds[i];
        }
      } else {
        if (cmds[i] == pos) continue;
        commands.emplace_back(Command::make_smove(1, cmds[i] - pos));
        pos = cmds[i];
      }
    }
    // Last move
    commands.emplace_back(Command::make_halt(1));

    ce.Execute(commands);
//    cout << ce.json << endl;
    cout << Json2Binary(ce.json);
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
  skeleton->extract_skeletons();
//  OutputMDL(PointsToVVV(path, R));
}
