#ifndef COMMAND_EXECUTER_H
#define COMMAND_EXECUTER_H

#include <vector>
#include <utility>
#include <set>
#include <array>

// #include "command.h"
#include "command_util.h"

#include "glog/logging.h"

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
    std::pair<int,int> seeds; // [x, y]
    BotStatus();
    BotStatus(bool active_, const Point& pos_, const std::pair<int,int>& seeds);
    bool HasSeeds();
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
  void Fission(const uint32_t bot_id, const Point& nd, const uint32_t m);
  void Fill(const uint32_t bot_id, const Point& nd);
  // TODO(udon)

 private:
  std::vector<std::pair<Point, Point>> v_cords;
  size_t num_active_bots;
  std::array<BotStatus, kMaxNumBots+1> bot_status;
  SystemStatus system_status;

  // utility
  uint32_t GetBotsNum();
  bool IsActiveBotId(const uint32_t id);
  bool IsValidCoordinate(const Point& p);
  bool IsVoidCoordinate(const Point& p);
  bool IsVoidPath(const Point& p1, const Point& p2);
};

#endif
