#include "command_executer.h"

#include <algorithm>

#define UNREACHABLE() CHECK(false)

// CommandExecuter
// Constructors
CommandExecuter::SystemStatus::SystemStatus(int r)
  : R(r), energy(0), harmonics(LOW) {
  // TODO(hiroh): can this be replaced by matrix{}?
  memset(matrix, 0, sizeof(uint8_t) * kMaxResolution * kMaxResolution * kMaxResolution);
}
CommandExecuter::BotStatus::BotStatus()
  : active(false), pos(0, 0, 0), seeds(-1, -1) {}

CommandExecuter::BotStatus::BotStatus(bool active, const Point& pos, const std::pair<int, int>& seeds) : active(active), pos(pos), seeds(seeds) {}

CommandExecuter::CommandExecuter(int R)
  : num_active_bots(1), system_status(R) {
  // Bot[1] is active, exists at (0,0,0) and has all the seeds.
  bot_status[1].active = true;
  bot_status[1].pos = Point(0,0,0);
  bot_status[1].seeds = std::make_pair(1, kMaxNumBots); // [1, 20]
}

// Utilities
bool IsLCD(const Point& p) {
  if ((p.x != 0 && p.y == 0 && p.z == 0) ||
      (p.x == 0 && p.y != 0 && p.z == 0) ||
      (p.x == 0 && p.y == 0 && p.z != 0)) {
    return true;
  }
  return false;
}

uint32_t MLen(const Point& p) {
  return abs(p.x) + abs(p.y) + abs(p.z);
}

uint32_t CLen(const Point& p) {
  return std::max(std::max(abs(p.x), abs(p.y)), abs(p.z));
}

bool IsSLD(const Point& p) {
  return IsLCD(p) && MLen(p) <= 5;
}

bool IsLLD(const Point& p) {
  return IsLCD(p) && MLen(p) <= 15;
}

bool IsNCD(const Point& p) {
  return IsLCD(p) && 0< MLen(p) && MLen(p) <= 2 && CLen(p) == 1;
}

bool IsPath(const Point& p1, const Point& p2) {
  Point p(abs(p1.x - p2.x),
          abs(p1.y - p2.y),
          abs(p1.z - p2.z));
  return IsLCD(p);
}

bool CommandExecuter::BotStatus::HasSeeds() {
  return
    1 <= seeds.first &&
    seeds.second <= 20 &&
    seeds.first < seeds.second;
}

uint32_t CommandExecuter::GetBotsNum() {
  return num_active_bots;
}

bool CommandExecuter::IsActiveBotId(const uint32_t id) {
  return 1 <= id && id <= GetBotsNum() && bot_status[id].active;
}

bool CommandExecuter::IsValidCoordinate(const Point& p) {
  return
    (0 <= p.x && p.x < system_status.R) &&
    (0 <= p.y && p.y < system_status.R) &&
    (0 <= p.z && p.z < system_status.R);
}

bool CommandExecuter::IsVoidCoordinate(const Point& p) {
  return system_status.matrix[p.x][p.y][p.z] == VOID;
}

// Check if
//  * [p1, p2] is a path and
//  * every coordinate in the region [p1,p2] is Void
bool CommandExecuter::IsVoidPath(const Point& p1, const Point& p2) {
  if (!IsPath(p1, p2))
    return false;

  Point delta(0,0,0);

  if (p1.x != p2.x) {
    int dx = (p1.x < p2.x) ? 1 : -1;
    delta = Point(dx, 0, 0);
  } else if (p1.y != p2.y) {
    int dy = (p1.y < p2.y) ? 1 : -1;
    delta = Point(0, dy, 0);
  } else if (p1.z != p2.z) {
    int dz = (p1.z < p2.z) ? 1 : -1;
    delta = Point(0, 0, dz);
  } else {
    UNREACHABLE();
  }

  Point p = p1;
  while (p != p2) {
    if (!IsVoidCoordinate(p))
      return false;

    p += delta;
  }

  return true;
}


// Commands
void CommandExecuter::Halt(const uint32_t bot_id) {
  CHECK(IsActiveBotId(bot_id));
  Point& p = bot_status[bot_id].pos;
  CHECK(p.x == 0 && p.y == 0 && p.z == 0);
  CHECK(GetBotsNum() == 1);
  CHECK(bot_status[bot_id].active);
  CHECK(system_status.harmonics == LOW);
  bot_status[bot_id].active = false;
}

void CommandExecuter::Wait(const uint32_t bot_id) {
  CHECK(IsActiveBotId(bot_id));

  Point c = bot_status[bot_id].pos;
  v_cords.push_back(std::make_pair(c, c));
}

void CommandExecuter::Flip(const uint32_t bot_id) {
  CHECK(IsActiveBotId(bot_id));

  Point c = bot_status[bot_id].pos;
  v_cords.push_back(std::make_pair(c, c));

  if (system_status.harmonics == HIGH) {
    system_status.harmonics = LOW;
  } else {
    system_status.harmonics = HIGH;
  }
}

void CommandExecuter::SMove(const uint32_t bot_id, const Point& lld) {
  CHECK(IsActiveBotId(bot_id));
  CHECK(IsLLD(lld));

  Point c0 = bot_status[bot_id].pos;
  Point c1 = c0 + lld;

  bot_status[bot_id].pos = c1;

  CHECK(IsValidCoordinate(c1));
  CHECK(IsVoidPath(c0, c1));

  v_cords.push_back(std::make_pair(c0, c1));
  system_status.energy += 2 * MLen(lld);
}

void CommandExecuter::LMove(const uint32_t bot_id, const Point& sld1, const Point& sld2) {
  CHECK(IsActiveBotId(bot_id));
  CHECK(IsSLD(sld1));
  CHECK(IsSLD(sld2));
  Point c0 = bot_status[bot_id].pos;
  Point c1 = c0 + sld1;
  Point c2 = c1 + sld2;

  CHECK(IsValidCoordinate(c1));
  CHECK(IsValidCoordinate(c2));
  CHECK(IsVoidPath(c0, c1));
  CHECK(IsVoidPath(c1, c2));

  bot_status[bot_id].pos = c2;
  v_cords.push_back(std::make_pair(c0, c1));
  v_cords.push_back(std::make_pair(c1, c2));
  system_status.energy += 2 * (MLen(sld1) + 2 + MLen(sld2));
}

void CommandExecuter::Fission(const uint32_t bot_id, const Point& nd, const uint32_t m) {

  // BUG(udon): Doesn't work
  UNREACHABLE();


  CHECK(IsActiveBotId(bot_id));
  CHECK(IsNCD(nd));
  CHECK(bot_status[bot_id].HasSeeds());
  BotStatus& bot = bot_status[bot_id];
  Point c0 = bot.pos;
  Point c1 = c1 + nd;
  CHECK(IsValidCoordinate(c1));
  CHECK(IsVoidCoordinate(c1));

  uint32_t bid_1 = bot.seeds.first;
  uint32_t bid_m = bid_1 + m - 1;
  uint32_t bid_n = bot.seeds.second;
  uint32_t n = bid_n - bid_1 + 1;

  CHECK(m+1 <= n);

  bot.seeds.first = bid_m + 2;
  bot.seeds.second = bid_n;

  uint32_t newbot_id = bid_1;
  CHECK(bot_status[newbot_id].active == false);
  BotStatus& newbot = bot_status[newbot_id];
  newbot.active = true;
  newbot.pos = c1;
  newbot.seeds = std::make_pair(bid_1 + 1, bid_m + 1);

  num_active_bots += 1;

  system_status.energy += 24;

  v_cords.push_back(std::make_pair(c0, c0));
  v_cords.push_back(std::make_pair(c1, c1));
}

void CommandExecuter::Fill(const uint32_t bot_id, const Point& nd) {
  CHECK(IsActiveBotId(bot_id));
  CHECK(IsNCD(nd));

  BotStatus& bot = bot_status[bot_id];
  Point c0 = bot.pos;
  Point c1 = c1 + nd;
  CHECK(IsValidCoordinate(c1));

  if (system_status.matrix[c1.x][c1.y][c1.z] == VOID) {
    system_status.matrix[c1.x][c1.y][c1.z] = FULL;
    system_status.energy += 12;
  } else {
    system_status.energy += 6;
  }

  v_cords.push_back(std::make_pair(c0, c0));
  v_cords.push_back(std::make_pair(c1, c1));
}
