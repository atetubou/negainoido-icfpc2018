#ifndef COMMAND_EXECUTER_H
#define COMMAND_EXECUTER_H

#include <vector>
#include <utility>
#include <set>
#include <array>

#include "glog/logging.h"

struct Point {
  int x;
  int y;
  int z;

  Point() = default;
  Point(int x, int y, int z) : x(x), y(y), z(z) {}
  Point operator+ (const Point& p);
  Point& operator+= (const Point& p);
};
bool operator==(const Point& p1, const Point& p2);
bool operator!=(const Point& p1, const Point& p2);

enum Harmonics {
  LOW = 0,
  HIGH = 1,
};

enum VoxelState : uint8_t {
  VOID = 0,
  FULL = 1,
};

class CommandExecuter {
 public:
  enum {
    kMaxNumBots = 20,
    kMaxResolution = 250,
  };
  struct SystemStatus {
    const int R = 0;
    int energy;
    Harmonics harmonics;
    VoxelState matrix[kMaxResolution][kMaxResolution][kMaxResolution];
    // TODO(hiroh): represent active bot in more efficient way.
    // might be, bool active_bots[kMaxNumBots + 1];
    explicit SystemStatus(int r);
  };
  struct BotStatus {
    bool active;
    Point pos;
    std::pair<int,int> seeds; // [x, y)
    BotStatus();
    BotStatus(bool active_, const Point& pos_, const std::pair<int,int>& seeds);
  };

  explicit CommandExecuter(int R);
  ~CommandExecuter() = default;

  // Commands
  // Singleton Commands
  void Halt(const uint32_t bot_id);
  void Wait(const uint32_t bot_id);
  void Flip(const uint32_t bot_id);
  void SMove(const uint32_t bot_id, const Point& lld);
  void LMove(const uint32_t bot_id, const Point& sld1, const Point& sld2);
  // TODO(udon)

 private:
  std::vector<std::pair<Point, Point>> v_cords;
  size_t num_active_bots;
  std::array<BotStatus, kMaxNumBots+1> bot_status;
  SystemStatus system_status;

  // utility
  uint32_t GetBotsNum();
  bool IsValidBotId(const uint32_t id);
  bool IsVoidPath(const Point& p1, const Point& p2);
};

#endif
