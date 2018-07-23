#ifndef COMMAND_EXECUTER_H
#define COMMAND_EXECUTER_H

#include <vector>
#include <utility>
#include <set>
#include <array>

#include "command.h"
#include "command_util.h"

#include "glog/logging.h"
#include "json/json.h"

using v = std::vector<int>;
using vv = std::vector<v>;
using vvv = std::vector<vv>;

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
    kMaxNumBots = 40,
    kMaxResolution = 250,
  };
  struct SystemStatus {
    const int R = 0;
    long long energy;
    Harmonics harmonics;
    VoxelState matrix[kMaxResolution][kMaxResolution][kMaxResolution];
    explicit SystemStatus(int r);
  };
  struct BotStatus {
    int bot_id;
    bool active;
    Point pos;
    std::set<uint32_t> seeds;
    BotStatus();
    std::pair<BotStatus, BotStatus> TryFission(const Point& p, int m);
  };

  CommandExecuter(int R, bool output_json);
  CommandExecuter(vvv src_model, bool output_json);
  ~CommandExecuter();

  void Execute(const std::vector<Command>& commands);
  Json::Value GetJson();
  void PrintTraceAsJson();

  bool IsGrounded(const Point&);

  const SystemStatus& GetSystemStatus() {
    return system_status;
  }
  const std::array<BotStatus, kMaxNumBots+1>& GetBotStatus() {
    return bot_status;
  }

  uint32_t GetActiveBotsNum();
  Json::Value json;

 private:
  struct VolCord {
    enum {
      kGFill = 1106,
      kGVoid = 1111,
    };
    uint32_t id;
    Point from;
    Point to; // enclosed region [From, To].
    int n;
  VolCord(uint32_t id, const Point& from, const Point& to, int n = 2) : id(id), from(std::move(from)), to(std::move(to)), n(n) {}
  };
  std::vector<VolCord> v_cords;
  size_t num_active_bots;
  std::array<BotStatus, kMaxNumBots+1> bot_status;
  SystemStatus system_status;
  bool output_json;

  // Used for IsGrounded
  bool grounded_memo[kMaxResolution][kMaxResolution][kMaxResolution];
  bool all_voxels_are_grounded = true;
  // false if VOID/GVOID was called after the last calculation
  // or harmonics == HIGH
  bool valid_grounded_memo = false;

  // utility
  bool IsActiveBotId(const uint32_t id);
  bool IsValidCoordinate(const Point& p);
  bool IsVoidCoordinate(const Point& p);
  bool IsVoidPath(const Point& p1, const Point& p2);

  void UpdateGroundedMemo();
  void UpdateGroundedPoint(const Point& pos);
  bool IsGroundedSlow(const Point&, bool);
  bool AllVoxelsAreGrounded();
  void VerifyWellFormedSystem();
  std::pair<Point, Point> VerifyGFillCommand(const Command& com, Point *neighbor);
  std::pair<Point, Point> VerifyGVoidCommand(const Command& com, Point *neighbor);
  void Halt(const uint32_t bot_id);
  void Wait(const uint32_t bot_id);
  void Flip(const uint32_t bot_id);
  void SMove(const uint32_t bot_id, const Point& lld);
  void LMove(const uint32_t bot_id, const Point& sld1, const Point& sld2);
  void Fill(const uint32_t bot_id, const Point& nd);
  void Void(const uint32_t bot_id, const Point& nd);

  void Fission(const uint32_t bot_id, const Point& nd, const uint32_t m);
  void Fusion(const uint32_t bot_id1, const Point& nd1,
              const uint32_t bot_id2, const Point& nd2);
  void GFill(const std::vector<uint32_t>& bot_ids,
             const Point& r1, const Point& r2);
  void GVoid(const std::vector<uint32_t>& bot_ids,
             const Point& r1, const Point& r2);
};

#endif
