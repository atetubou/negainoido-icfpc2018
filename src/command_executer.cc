#include "command_executer.h"

#include <iostream>
#include <algorithm>

#define UNREACHABLE() CHECK(false)
#define CE_DEBUG

// CommandExecuter
// Constructors
CommandExecuter::SystemStatus::SystemStatus(int r)
  : R(r), energy(0), harmonics(LOW) {
  LOG_ASSERT(r <= kMaxResolution) << r;
  // TODO(hiroh): can this be replaced by matrix{}?
  memset(matrix, 0, sizeof(matrix));
}

CommandExecuter::BotStatus::BotStatus()
  : active(false), pos(0, 0, 0) {}

CommandExecuter::CommandExecuter(int R, bool output_json)
  : num_active_bots(1), system_status(R), output_json(output_json) {
  // Bot[1] is active, exists at (0,0,0) and has all the seeds.
  bot_status[1].active = true;
  bot_status[1].pos = Point(0,0,0);

  for (int i = 2; i <= 20; i++) {
    bot_status[1].seeds.insert(i);
  }
}

CommandExecuter::~CommandExecuter() {}

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
  return std::max({abs(p.x), abs(p.y), abs(p.z)});
}

bool IsSLD(const Point& p) {
  return IsLCD(p) && MLen(p) <= 5;
}

bool IsLLD(const Point& p) {
  return IsLCD(p) && MLen(p) <= 15;
}

bool IsNCD(const Point& p) {
  return 0 < MLen(p) && MLen(p) <= 2 && CLen(p) == 1;
}

bool IsPath(const Point& p1, const Point& p2) {
  Point p(abs(p1.x - p2.x),
          abs(p1.y - p2.y),
          abs(p1.z - p2.z));
  return IsLCD(p);
}

bool ColidX(int from_x1, int to_x1, int from_x2, int to_x2) {

  if (from_x1 > to_x1)
    std::swap(from_x1, to_x1);

  if (from_x2 > to_x2)
    std::swap(from_x2, to_x2);

  if (from_x1 <= from_x2 && from_x2 <= to_x1)
    return true;

  if (from_x2 <= from_x1 && from_x1 <= to_x2)
    return true;

  return false;
}

uint32_t CommandExecuter::GetActiveBotsNum() {
#ifdef CE_DEBUG
  size_t cnt_num_active_bots = 0;
  for (int i = 1; i <= kMaxNumBots; i++) {
    cnt_num_active_bots += bot_status[i].active;
  }
  LOG_ASSERT(cnt_num_active_bots == num_active_bots) << cnt_num_active_bots << " " << num_active_bots;
#endif

  return num_active_bots;
}

bool CommandExecuter::IsActiveBotId(const uint32_t id) {
  return 1 <= id && id <= kMaxNumBots && bot_status[id].active;
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

  // No need to check and in that case must not be called in this func.
  LOG_ASSERT(p1 != p2) << "p1=" << p1 << ", p2=" << p2;

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

  // Point p1 can be already FULL.
  Point p = p1;
  p += delta;

  while (true) {

    if (!IsVoidCoordinate(p))
      return false;

    if (p == p2)
      break;

    p += delta;
  }

  return true;
}

void CommandExecuter::Execute(const std::vector<Command>& commands) {

  // Update energy
  const long long R = system_status.R;
  if (system_status.harmonics == HIGH) {
    system_status.energy += 30 * R * R * R;
  } else {
    system_status.energy += 3 * R * R * R;
  }

  system_status.energy += 20 * num_active_bots;

  int fusion_count = 0;
  // Execute FusionP and FusionS
  for (const auto& com1 : commands) {
    for (const auto& com2 : commands) {
      if (com1.id != com2.id &&
          com1.type == Command::Type::FUSION_P &&
          com2.type == Command::Type::FUSION_S) {
        BotStatus& bot1 = bot_status[com1.id];
        BotStatus& bot2 = bot_status[com2.id];

        if (bot1.pos + com1.fusion_p_nd == bot2.pos &&
            bot2.pos + com2.fusion_s_nd == bot1.pos) {
          Fusion(com1.id, com1.fusion_p_nd, com2.id, com1.fusion_s_nd);
          fusion_count += 2;
        }
      }
    }
  }

  // Execute other commands
  for (const auto& c : commands) {
    auto id = c.id;
    switch(c.type) {
    case Command::Type::HALT:
      Halt(id);
      break;
    case Command::Type::WAIT:
      Wait(id);
      break;
    case Command::Type::FLIP:
      Flip(id);
      break;
    case Command::Type::SMOVE:
      SMove(id, c.smove_lld);
      break;
    case Command::Type::LMOVE:
      LMove(id, c.lmove_sld1, c.lmove_sld2);
      break;
    case Command::Type::FISSION:
      Fission(id, c.fission_nd, c.fission_m);
      break;
    case Command::Type::FILL:
      Fill(id, c.fill_nd);
      break;
    case Command::Type::FUSION_P:
    case Command::Type::FUSION_S:
      // do nothing
      fusion_count--;
      break;
    }
  }

  LOG_ASSERT(fusion_count == 0) << fusion_count;

  // Check volatile cordinates
  for (const auto& vcord1 : v_cords) {
    for (const auto& vcord2 : v_cords) {
      if (vcord1.id == vcord2.id) {
        continue;
      }

      LOG_ASSERT(IsLCD(vcord1.from - vcord1.to)) << vcord1.from << " " << vcord1.to;
      LOG_ASSERT(IsLCD(vcord2.from - vcord2.to)) << vcord2.from << " " << vcord2.to;

      bool cold_x = ColidX(vcord1.from.x, vcord1.to.x, vcord2.from.x, vcord2.to.x);
      bool cold_y = ColidX(vcord1.from.y, vcord1.to.y, vcord2.from.y, vcord2.to.y);
      bool cold_z = ColidX(vcord1.from.z, vcord1.to.z, vcord2.from.z, vcord2.to.z);
      LOG_ASSERT(cold_x && cold_y && cold_z) 
        << "Invalid Move (Colid)\n"
        << "vc1: id=" << vcord1.id << ", from= " << vcord1.from << ", to=" << vcord1.to << "\n"
        << "vc2: id=" << vcord2.id << ", from= " << vcord2.from << ", to=" << vcord2.to << "\n";
    }
  }

  if (output_json) {
    auto turn_json = Command::CommandsToJson(commands);
    json["turn"].append(std::move(turn_json));
  }

  v_cords.clear();
}

Json::Value CommandExecuter::GetJson() {
  CHECK(output_json);
  return json;
}

void CommandExecuter::PrintTraceAsJson() {
  if (output_json) {
    std::cout << json << std::endl;
  }
}

// Commands
void CommandExecuter::Halt(const uint32_t bot_id) {
  LOG_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  Point& p = bot_status[bot_id].pos;
  LOG_ASSERT(p == Point(0,0,0)) << p;
  LOG_ASSERT(GetActiveBotsNum() == 1) << GetActiveBotsNum();
  LOG_ASSERT(system_status.harmonics == LOW);
  bot_status[bot_id].active = false;
  num_active_bots--;
  v_cords.emplace_back(bot_id, p, p);
  LOG_ASSERT(num_active_bots == 0) << num_active_bots;
}

void CommandExecuter::Wait(const uint32_t bot_id) {
  LOG_ASSERT(IsActiveBotId(bot_id)) << bot_id;

  Point c = bot_status[bot_id].pos;
  v_cords.emplace_back(bot_id, c, c);
}

void CommandExecuter::Flip(const uint32_t bot_id) {
  LOG_ASSERT(IsActiveBotId(bot_id)) << bot_id;

  Point c = bot_status[bot_id].pos;
  v_cords.emplace_back(bot_id, c, c);

  if (system_status.harmonics == HIGH) {
    system_status.harmonics = LOW;
  } else {
    system_status.harmonics = HIGH;
  }
}

void CommandExecuter::SMove(const uint32_t bot_id, const Point& lld) {
  LOG_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  LOG_ASSERT(IsLLD(lld)) << lld;

  Point c0 = bot_status[bot_id].pos;
  Point c1 = c0 + lld;

  LOG_ASSERT(IsValidCoordinate(c1)) << c1;
  LOG_ASSERT(IsVoidPath(c0, c1)) << c0 << " " << c1;

  bot_status[bot_id].pos = c1;
  v_cords.emplace_back(bot_id, c0, c1);
  system_status.energy += 2 * MLen(lld);
}

void CommandExecuter::LMove(const uint32_t bot_id, const Point& sld1, const Point& sld2) {
  LOG_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  LOG_ASSERT(IsSLD(sld1)) << sld1;
  LOG_ASSERT(IsSLD(sld2)) << sld2;
  Point c0 = bot_status[bot_id].pos;
  Point c1 = c0 + sld1;
  Point c2 = c1 + sld2;
  LOG_ASSERT(IsValidCoordinate(c1)) << c1;
  LOG_ASSERT(IsValidCoordinate(c2)) << c2;
  LOG_ASSERT(IsVoidPath(c0, c1)) << c0 << " " << c1;
  LOG_ASSERT(IsVoidPath(c1, c2)) << c1 << " " << c2;

  bot_status[bot_id].pos = c2;
  v_cords.emplace_back(bot_id, c0, c1);
  v_cords.emplace_back(bot_id, c1, c2);
  system_status.energy += 2 * (MLen(sld1) + 2 + MLen(sld2));
}

void CommandExecuter::Fission(const uint32_t bot_id, const Point& nd, const uint32_t m) {
  LOG_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  LOG_ASSERT(IsNCD(nd)) << nd;
  LOG_ASSERT(!bot_status[bot_id].seeds.empty()) << bot_status[bot_id].seeds.size();
  BotStatus& bot = bot_status[bot_id];
  Point c0 = bot.pos;
  Point c1 = c0 + nd;
  LOG_ASSERT(IsValidCoordinate(c1)) << c1;
  LOG_ASSERT(IsVoidCoordinate(c1)) << c1;

  const uint32_t n = bot.seeds.size();
  LOG_ASSERT(m + 1 <= n) << m + 1 << " " << n;

  auto seeds_iter = bot.seeds.begin();

  uint32_t newbot_id = *seeds_iter++;
  LOG_ASSERT(bot_status[newbot_id].active == false) << newbot_id;
  BotStatus& newbot = bot_status[newbot_id];
  newbot.active = true;
  newbot.pos = c1;
  num_active_bots += 1;

  for (auto i = 0U; i < m; i++) {
    newbot.seeds.insert(*seeds_iter++);
  }

  std::set<uint32_t> bot_seeds;
  while(seeds_iter != bot.seeds.end()) {
    bot_seeds.insert(*seeds_iter++);
  }

  bot.seeds = bot_seeds;

  LOG_ASSERT(newbot.seeds.size() == m) << newbot.seeds.size() << " " << m;
  LOG_ASSERT(bot.seeds.size() == n - 1 - m) << newbot.seeds.size() << " " << n << " " << m;

  system_status.energy += 24;

  v_cords.emplace_back(bot_id, c0, c0);
  v_cords.emplace_back(bot_id, c1, c1);
}

void CommandExecuter::Fill(const uint32_t bot_id, const Point& nd) {
  LOG_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  LOG_ASSERT(IsNCD(nd)) << nd;

  BotStatus& bot = bot_status[bot_id];
  Point c0 = bot.pos;
  Point c1 = c0 + nd;
  LOG_ASSERT(IsValidCoordinate(c1)) << c1;

  if (system_status.matrix[c1.x][c1.y][c1.z] == VOID) {
    system_status.matrix[c1.x][c1.y][c1.z] = FULL;
    system_status.energy += 12;
  } else {
    system_status.energy += 6;
  }

  v_cords.emplace_back(bot_id, c0, c0);
  v_cords.emplace_back(bot_id, c1, c1);
}

void CommandExecuter::Fusion(const uint32_t bot_id1, const Point& nd1,
                             const uint32_t bot_id2, const Point& nd2) {
  LOG_ASSERT(IsActiveBotId(bot_id1)) << bot_id1;
  LOG_ASSERT(IsActiveBotId(bot_id2)) << bot_id2;
  LOG_ASSERT(IsNCD(nd1)) << nd1;
  LOG_ASSERT(IsNCD(nd2)) << nd2;

  BotStatus& bot1 = bot_status[bot_id1];
  BotStatus& bot2 = bot_status[bot_id2];
  LOG_ASSERT(IsValidCoordinate(bot1.pos)) << bot1.pos;
  LOG_ASSERT(IsValidCoordinate(bot2.pos)) << bot2.pos;
  LOG_ASSERT(bot1.pos + nd1 == bot2.pos) << bot1.pos << " " << nd1 << " " << bot2.pos;
  LOG_ASSERT(bot2.pos + nd2 == bot1.pos) << bot2.pos << " " << nd2 << " " << bot1.pos;

  bot2.active = false;
  num_active_bots -= 1;

  for (uint32_t seed : bot2.seeds) {
    bot1.seeds.insert(seed);
  }
  bot1.seeds.insert(bot_id2);

  system_status.energy -= 24;

  v_cords.emplace_back(bot_id1, bot1.pos, bot1.pos);
  v_cords.emplace_back(bot_id2, bot2.pos, bot2.pos);
}
