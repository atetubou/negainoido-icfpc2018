#ifndef COMMAND_EXECUTER_H
#define COMMAND_EXECUTER_H

#include <vector>
#include <utility>
#include <set>

#include "glog/logging.h"

struct Point {
  int x;
  int y;
  int z;
  Point operator+ (const Point& p);
  Point& operator+= (const Point& p);
};

enum Harmonic {
  LOW = 0,
  HIGH = 1,
};

enum VoxelState {
  FULL = 0,
  VOID = 1,
};

class CommandExecuter {
 public:
  enum {
    kMaxNumBots = 20,
    kMaxResolution = 250,
  };
  struct SystemStatus {
    const int R;
    int energy;
    Harmonic harmonic;
    VoxelState matrix[kMaxResolution][kMaxResolution][kMaxResolution];
    // TODO(hiroh): represent active bot in more efficient way.
    // might be, bool active_bots[kMaxNumBots + 1];
    std::set<int> active_bots;
  };
  struct BotStatus {
    int id; // 1-indexed!
    Point pos;
    std::pair<int,int> seeds; // [x, y)
  };

  CommandExecuter(int R);
  ~CommandExecuter() = default;
  // Func()
  // void LMove(id, sld1, sld2);
  //

 private:
  std::vector<std::pair<Point, Point>> v_cords;
  BotStatus bot_status[kMaxNumBots];
  SystemStatus system_status;
};

#endif
