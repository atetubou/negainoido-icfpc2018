#include "command_executer.h"

#include <iostream>
#include <algorithm>

#define CE_DEBUG
#ifdef CE_DEBUG
#define UNREACHABLE() CHECK(false)
#define CE_ASSERT(cond) LOG_ASSERT(cond)
#else
#define UNREACHABLE()
#define CE_ASSERT(cond) DLOG_ASSERT(cond)
#endif // CE_DEBUG


// CommandExecuter
// Constructors
CommandExecuter::SystemStatus::SystemStatus(int r)
  : R(r), energy(0), harmonics(LOW) {
  CE_ASSERT(r <= kMaxResolution) << r;
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

  for (int i = 2; i <= kMaxNumBots; i++) {
    bot_status[1].seeds.insert(i);
  }

  // Initialize for IsGrounded
  always_low = true;
  valid_grounded_memo = false;
}

CommandExecuter::CommandExecuter(vvv src_model, bool output_json)
  : num_active_bots(1), system_status(src_model.size()), output_json(output_json) {
  // Bot[1] is active, exists at (0,0,0) and has all the seeds.
  bot_status[1].active = true;
  bot_status[1].pos = Point(0,0,0);

  for (int i = 2; i <= kMaxNumBots; i++) {
    bot_status[1].seeds.insert(i);
  }

  always_low = true;
  int R = src_model.size();
  for (int i = 0; i < R; ++i)
    for (int j = 0; j < R; ++j)
      for (int k = 0; k < R; ++k)
        if (src_model[i][j][k])
          system_status.matrix[i][j][k] = FULL;

  // Initialize for IsGrounded
  always_low = true;
  valid_grounded_memo = false;
}

CommandExecuter::~CommandExecuter() {}

uint32_t CommandExecuter::GetActiveBotsNum() {
#ifdef CE_DEBUG
  size_t cnt_num_active_bots = 0;
  for (int i = 1; i <= kMaxNumBots; i++) {
    cnt_num_active_bots += bot_status[i].active;
  }
  CE_ASSERT(cnt_num_active_bots == num_active_bots) << cnt_num_active_bots << " " << num_active_bots;
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
  CE_ASSERT(p1 != p2) << "p1=" << p1 << ", p2=" << p2;

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

  while (true) {

    if (!IsVoidCoordinate(p))
      return false;

    if (p == p2)
      break;

    p += delta;
  }

  return true;
}

bool CommandExecuter::IsGrounded(const Point& p) {
  if (always_low && valid_grounded_memo) {
    return grounded_memo[p.x][p.y][p.z];
  } else {
    return IsGroundedSlow(p, false);
  }
}

bool CommandExecuter::IsGroundedSlow(const Point& p, bool check_all_mode) {
  // If check_all_mode is 'true', ignore p and
  // check if every FULL point is grounded

  const int adj_dx[] = {-1, 1, 0, 0, 0, 0};
  const int adj_dy[] = { 0, 0,-1, 1, 0, 0};
  const int adj_dz[] = { 0, 0, 0, 0,-1, 1};

  const int R = system_status.R;
  int full_num = 0;

  // Initialize
  for (int x=0; x<R; ++x) {
    for (int y=0; y<R; ++y) {
      for (int z=0; z<R; ++z) {
        grounded_memo[x][y][z] = false;
        if (system_status.matrix[x][y][z] == FULL) {
          full_num++;
        }
      }
    }
  }

  SystemStatus& status = system_status;

  // Check voxels where y = 0
  std::stack<Point> s;
  for (int x=0; x<R; ++x) {
    for (int z=0; z<R; ++z) {
      if (status.matrix[x][0][z] == VOID) continue;
      s.push(Point(x, 0, z));
      grounded_memo[x][0][z] = true;
      full_num--;
    }
  }

  while (not s.empty()) {
      Point p = s.top(); s.pop();
      for (int k = 0; k < 6; k++) {
        int x = p.x + adj_dx[k];
        int y = p.y + adj_dy[k];
        int z = p.z + adj_dz[k];
        if (x < 0 || y < 0 || z < 0) continue;
        if (x >= R || y >= R || z >= R) continue;
        if (grounded_memo[x][y][z]) continue;
        if (status.matrix[x][y][z] == VOID) continue;
        grounded_memo[x][y][z] = 1;
        full_num--;
        s.push(Point(x, y, z));
      }
  }

  valid_grounded_memo = true;

  if (check_all_mode) {
    return full_num == 0;
  } else {
    return grounded_memo[p.x][p.y][p.z];
  }
}

void CommandExecuter::VerifyWellFormedSystem() {
  // Check if the harmonics is Low, then all Full voxels of the matrix are grounded.
  CE_ASSERT(system_status.harmonics == HIGH ||
            IsGrounded(Point(0,0,0), true));

  // The position of each active nanobot is distinct and is Void in the matrix.
  std::set<Point> p_set;
  std::set<uint32_t> seed_set;
  uint32_t len = 0;
  for (int i = 0; i < CommandExecuter::kMaxNumBots; ++i) {
    if (bot_status[i].active) {
      Point &p = bot_status[i].pos;
      p_set.insert(p);
      CE_ASSERT(system_status.matrix[p.x][p.y][p.z] == VOID) << p;

      for (auto x : bot_status[i].seeds) {
        seed_set.insert(x);
      }
      len += bot_status[i].seeds.size();
    }
  }
  CE_ASSERT(p_set.size() == num_active_bots);

  // The seeds of each active nanobot are disjoint.
  CE_ASSERT(seed_set.size() == len);

  // The seeds of each active nanobot does not include
  // the identifier of any active nanobot.
  for (auto seed : seed_set) {
    CE_ASSERT(!bot_status[seed].active);
  }
}

void VerifyCommandSeq(const std::vector<Command>& commands) {

  std::set<uint32_t> bot_id_set;
  for (auto c : commands) {
    bot_id_set.insert(c.id);
  }

  CE_ASSERT(commands.size() == bot_id_set.size());
}

std::pair<Point, Point> CommandExecuter::VerifyGFillCommand(const Command& com, Point *neighbor) {
  auto bot_id = com.id;
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  CE_ASSERT(IsNCD(com.gfill_.nd)) << com.gfill_.nd;
  CE_ASSERT(IsFCD(com.gfill_.fd)) << com.gfill_.fd;
  auto pos = bot_status[bot_id].pos;
  auto r1 = pos + com.gfill_.nd;
  auto r2 = r1 + com.gfill_.fd;
  CE_ASSERT(IsValidCoordinate(r1)) << r1;
  CE_ASSERT(IsValidCoordinate(r2)) << r2;
  auto r3 = Point(std::min(r1.x, r2.x), std::min(r1.y, r2.y), std::min(r1.z, r2.z));
  auto r4 = Point(std::max(r1.x, r2.x), std::max(r1.y, r2.y),std::max(r1.z, r2.z));
  CE_ASSERT(IsValidCoordinate(r3)) << r3;
  CE_ASSERT(IsValidCoordinate(r4)) << r4;
  *neighbor = std::move(r1);
  return std::make_pair(r3, r4);
}

std::pair<Point, Point> CommandExecuter::VerifyGVoidCommand(const Command& com, Point *neighbor) {
  auto bot_id = com.id;
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  CE_ASSERT(IsNCD(com.gvoid_.nd)) << com.gvoid_.nd;
  CE_ASSERT(IsFCD(com.gvoid_.fd)) << com.gvoid_.fd;
  auto pos = bot_status[bot_id].pos;
  auto r1 = pos + com.gvoid_.nd;
  auto r2 = r1 + com.gvoid_.fd;
  CE_ASSERT(IsValidCoordinate(r1)) << r1;
  CE_ASSERT(IsValidCoordinate(r2)) << r2;
  auto r3 = Point(std::min(r1.x, r2.x), std::min(r1.y, r2.y), std::min(r1.z, r2.z));
  auto r4 = Point(std::max(r1.x, r2.x), std::max(r1.y, r2.y),std::max(r1.z, r2.z));
  CE_ASSERT(IsValidCoordinate(r3)) << r3;
  CE_ASSERT(IsValidCoordinate(r4)) << r4;
  *neighbor = std::move(r1);
  return std::make_pair(r3, r4);
}

void CommandExecuter::Execute(const std::vector<Command>& commands) {

  VerifyWellFormedSystem();
  VerifyCommandSeq(commands);

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

        if (bot1.pos + com1.fusion_p_.nd == bot2.pos &&
            bot2.pos + com2.fusion_s_.nd == bot1.pos) {
          Fusion(com1.id, com1.fusion_p_.nd, com2.id, com1.fusion_s_.nd);
          fusion_count += 2;
        }
      }
    }
  }

  // GVoid and GFill
  std::map<std::pair<Point, Point>, std::vector<std::pair<uint32_t, Point>>> gvoid_group, gfill_group;
  for (size_t i = 0; i < commands.size(); i++) {
    const auto& com = commands[i];
    if (com.type == Command::Type::GFILL) {
      Point neighbor;
      auto represent_key = VerifyGFillCommand(com, &neighbor);
      gvoid_group[std::move(represent_key)].emplace_back(i, neighbor);
    } else if (com.type == Command::Type::GVOID){
      Point neighbor;
      auto represent_key = VerifyGVoidCommand(com, &neighbor);
      gvoid_group[std::move(represent_key)].emplace_back(i, neighbor);
    }
  }
  for (const auto& gg : gfill_group) {
    std::vector<uint32_t> bot_ids(gg.second.size());
    // Check neighbor points are unique.
    for (size_t i = 0; i < gg.second.size(); i++) {
      bot_ids[i] = gg.second[i].first;
      for (size_t j = i + 1; j < gg.second.size(); j++) {
        CE_ASSERT(gg.second[i].second != gg.second[j].second) << i << " " << j << " " << gg.second[i].second;
      }
    }

    GFill(bot_ids, gg.first.first, gg.first.second);
  }

  for (const auto& gg : gvoid_group) {
    std::vector<uint32_t> bot_ids(gg.second.size());
    // Check neighbor points are unique.
    for (size_t i = 0; i < gg.second.size(); i++) {
      bot_ids[i] = gg.second[i].first;
      for (size_t j = i + 1; j < gg.second.size(); j++) {
        CE_ASSERT(gg.second[i].second != gg.second[j].second) << i << " " << j << " " << gg.second[i].second;
      }
    }
    GVoid(bot_ids, gg.first.first, gg.first.second);
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
      SMove(id, c.smove_.lld);
      break;
    case Command::Type::LMOVE:
      LMove(id, c.lmove_.sld1, c.lmove_.sld2);
      break;
    case Command::Type::FISSION:
      Fission(id, c.fission_.nd, c.fission_.m);
      break;
    case Command::Type::FILL:
      Fill(id, c.fill_.nd);
      break;
    case Command::Type::VOID:
      Void(id, c.void_.nd);
      break;
    case Command::Type::FUSION_P:
    case Command::Type::FUSION_S:
      // do nothing
      fusion_count--;
      break;
    case Command::Type::GVOID:
    case Command::Type::GFILL:
      // do nothing here.
      break;
    }
  }

  CE_ASSERT(fusion_count == 0) << fusion_count;

  // Check volatile cordinates
  for (const auto& vcord1 : v_cords) {
    for (const auto& vcord2 : v_cords) {
      bool colid = false;
      if (vcord1.id == VolCord::kGVoid || vcord1.id == VolCord::kGFill ) {
        // TODO(hiroh)
      } else if (vcord1.id == VolCord::kGVoid || vcord1.id == VolCord::kGFill) {
        // TODO(hiroh)
      } else {
        if (vcord1.id == vcord2.id) {
          continue;
        }
        bool cold_x = ColidX(vcord1.from.x, vcord1.to.x, vcord2.from.x, vcord2.to.x);
        bool cold_y = ColidX(vcord1.from.y, vcord1.to.y, vcord2.from.y, vcord2.to.y);
        bool cold_z = ColidX(vcord1.from.z, vcord1.to.z, vcord2.from.z, vcord2.to.z);
        colid = cold_x && cold_y && cold_z;
      }
      CE_ASSERT(!colid)
        << "Invalid Move (Colid)\n"
        << "vc1: id=" << vcord1.id << ", from= " << vcord1.from << ", to=" << vcord1.to << "\n"
        << "vc2: id=" << vcord2.id << ", from= " << vcord2.from << ", to=" << vcord2.to << "\n";
    }
  }

  if (output_json) {
    // Sort
    auto commands_ = commands;
    std::sort(commands_.begin(), commands_.end(),
              [](const auto& c1, const auto& c2) {
                return c1.id < c2.id;
              });

    auto turn_json = Command::CommandsToJson(commands);
    json["turn"].append(std::move(turn_json));
  }

  v_cords.clear();
}

Json::Value CommandExecuter::GetJson() {
  CE_ASSERT(output_json);
  return json;
}

void CommandExecuter::PrintTraceAsJson() {
  if (output_json) {
    std::cout << json << std::endl;
  }
}

// Commands
void CommandExecuter::Halt(const uint32_t bot_id) {
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  Point& p = bot_status[bot_id].pos;
  CE_ASSERT(p == Point(0,0,0)) << p;
  CE_ASSERT(GetActiveBotsNum() == 1) << GetActiveBotsNum();
  CE_ASSERT(system_status.harmonics == LOW);
  bot_status[bot_id].active = false;
  num_active_bots--;
  v_cords.emplace_back(bot_id, p, p);
  CE_ASSERT(num_active_bots == 0) << num_active_bots;
}

void CommandExecuter::Wait(const uint32_t bot_id) {
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;

  Point c = bot_status[bot_id].pos;
  v_cords.emplace_back(bot_id, c, c);
}

void CommandExecuter::Flip(const uint32_t bot_id) {
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;

  Point c = bot_status[bot_id].pos;
  v_cords.emplace_back(bot_id, c, c);

  if (system_status.harmonics == HIGH) {
    system_status.harmonics = LOW;
  } else {
    system_status.harmonics = HIGH;
  }

  // For IsGrounded
  always_low = false;
}

void CommandExecuter::SMove(const uint32_t bot_id, const Point& lld) {
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  CE_ASSERT(IsLLD(lld)) << lld;

  Point c0 = bot_status[bot_id].pos;
  Point c1 = c0 + lld;

  CE_ASSERT(IsValidCoordinate(c1)) << c1;
  CE_ASSERT(IsVoidPath(c0, c1)) << c0 << " " << c1;

  bot_status[bot_id].pos = c1;
  v_cords.emplace_back(bot_id, c0, c1);
  system_status.energy += 2 * MLen(lld);
}

void CommandExecuter::LMove(const uint32_t bot_id, const Point& sld1, const Point& sld2) {
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  CE_ASSERT(IsSLD(sld1)) << sld1;
  CE_ASSERT(IsSLD(sld2)) << sld2;
  Point c0 = bot_status[bot_id].pos;
  Point c1 = c0 + sld1;
  Point c2 = c1 + sld2;
  CE_ASSERT(IsValidCoordinate(c1)) << c1;
  CE_ASSERT(IsValidCoordinate(c2)) << c2;
  CE_ASSERT(IsVoidPath(c0, c1)) << c0 << " " << c1;
  CE_ASSERT(IsVoidPath(c1, c2)) << c1 << " " << c2;

  bot_status[bot_id].pos = c2;
  v_cords.emplace_back(bot_id, c0, c1);
  v_cords.emplace_back(bot_id, c1, c2);
  system_status.energy += 2 * (MLen(sld1) + 2 + MLen(sld2));
}

void CommandExecuter::Void(const uint32_t bot_id, const Point& nd) {
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  CE_ASSERT(IsNCD(nd)) << nd;
  BotStatus& bot = bot_status[bot_id];
  Point c0 = bot.pos;
  Point c1 = c0 + nd;
  CE_ASSERT(IsValidCoordinate(c1)) << c1;

  if (system_status.matrix[c1.x][c1.y][c1.z] == FULL) {
    system_status.matrix[c1.x][c1.y][c1.z] = VOID;
    system_status.energy -= 12;
  } else {
    system_status.energy += 3;
  }

  v_cords.emplace_back(bot_id, c0, c0);
  v_cords.emplace_back(bot_id, c1, c1);

  // re-calculation of grounded_memo is needed
  valid_grounded_memo = false;
}

void CommandExecuter::Fission(const uint32_t bot_id, const Point& nd, const uint32_t m) {
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  CE_ASSERT(IsNCD(nd)) << nd;
  CE_ASSERT(!bot_status[bot_id].seeds.empty()) << bot_status[bot_id].seeds.size();
  BotStatus& bot = bot_status[bot_id];
  Point c0 = bot.pos;
  Point c1 = c0 + nd;
  CE_ASSERT(IsValidCoordinate(c1)) << c1;
  CE_ASSERT(IsVoidCoordinate(c1)) << c1;

  const uint32_t n = bot.seeds.size();
  CE_ASSERT(m + 1 <= n) << m + 1 << " " << n;

  auto seeds_iter = bot.seeds.begin();

  uint32_t newbot_id = *seeds_iter++;
  CE_ASSERT(bot_status[newbot_id].active == false) << newbot_id;
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

  CE_ASSERT(newbot.seeds.size() == m) << newbot.seeds.size() << " " << m;
  CE_ASSERT(bot.seeds.size() == n - 1 - m) << newbot.seeds.size() << " " << n << " " << m;

  system_status.energy += 24;

  v_cords.emplace_back(bot_id, c0, c0);
  v_cords.emplace_back(bot_id, c1, c1);
}

void CommandExecuter::Fill(const uint32_t bot_id, const Point& nd) {
  CE_ASSERT(IsActiveBotId(bot_id)) << bot_id;
  CE_ASSERT(IsNCD(nd)) << nd;

  BotStatus& bot = bot_status[bot_id];
  Point c0 = bot.pos;
  Point c1 = c0 + nd;
  CE_ASSERT(IsValidCoordinate(c1)) << c1;

  if (system_status.matrix[c1.x][c1.y][c1.z] == VOID) {
    system_status.matrix[c1.x][c1.y][c1.z] = FULL;
    system_status.energy += 12;
  } else {
    system_status.energy += 6;
  }

  v_cords.emplace_back(bot_id, c0, c0);
  v_cords.emplace_back(bot_id, c1, c1);

  // For IsGrounded
  if (c1.y == 0 || grounded_memo[c1.x][c1.y - 1][c1.z]) {
    grounded_memo[c1.x][c1.y][c1.z] = true;
  }
}

void CommandExecuter::Fusion(const uint32_t bot_id1, const Point& nd1,
                             const uint32_t bot_id2, const Point& nd2) {
  CE_ASSERT(IsActiveBotId(bot_id1)) << bot_id1;
  CE_ASSERT(IsActiveBotId(bot_id2)) << bot_id2;
  CE_ASSERT(IsNCD(nd1)) << nd1;
  CE_ASSERT(IsNCD(nd2)) << nd2;

  BotStatus& bot1 = bot_status[bot_id1];
  BotStatus& bot2 = bot_status[bot_id2];
  CE_ASSERT(IsValidCoordinate(bot1.pos)) << bot1.pos;
  CE_ASSERT(IsValidCoordinate(bot2.pos)) << bot2.pos;
  CE_ASSERT(bot1.pos + nd1 == bot2.pos) << bot1.pos << " " << nd1 << " " << bot2.pos;
  CE_ASSERT(bot2.pos + nd2 == bot1.pos) << bot2.pos << " " << nd2 << " " << bot1.pos;

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

void CommandExecuter::GFill(const std::vector<uint32_t>& bot_ids,
                            const Point& r1, const Point& r2) {
  const size_t N = bot_ids.size();
  CE_ASSERT(N == 2 || N == 4 || N == 8) << N;
  if (N == 2) { // Line Case
    CE_ASSERT(IsPath(r1, r2)) << r1 << " " << r2;
  } else if (N == 4) { // Lectangle Case
    CE_ASSERT(r1.x == r2.x || r1.y == r2.y || r1.z == r2.z) << r1 << " " << r2;
  } else { // N == 8
    CE_ASSERT(r1.x != r2.x && r1.y != r2.y && r1.z != r2.z) << r1 << " " << r2;;
  }
  for (int x = r1.x; x <= r2.x; x++) {
    for (int y = r1.y; x <= r2.y; y++) {
      for (int z = r1.z; z <= r2.z; z++) {
        if (system_status.matrix[x][y][z] == VOID) {
          system_status.matrix[x][y][z] = FULL;
          system_status.energy += 12;
        } else {
          system_status.energy += 6;
        }
      }
    }
  }

  for (const auto& id : bot_ids) {
    v_cords.emplace_back(id, bot_status[id].pos, bot_status[id].pos);
  }
  v_cords.emplace_back(VolCord::kGFill, r1, r2);
}

void CommandExecuter::GVoid(const std::vector<uint32_t>& bot_ids,
                            const Point& r1, const Point& r2) {
  const size_t N = bot_ids.size();
  CE_ASSERT(N == 2 || N == 4 || N == 8) << N;
  if (N == 2) { // Line Case
    CE_ASSERT(IsPath(r1, r2)) << r1 << " " << r2;
  } else if (N == 4) { // Lectangle Case
    CE_ASSERT(r1.x == r2.x || r1.y == r2.y || r1.z == r2.z) << r1 << " " << r2;
  } else { // N == 8
    CE_ASSERT(r1.x != r2.x && r1.y != r2.y && r1.z != r2.z) << r1 << " " << r2;;
  }
  for (int x = r1.x; x <= r2.x; x++) {
    for (int y = r1.y; x <= r2.y; y++) {
      for (int z = r1.z; z <= r2.z; z++) {
        if (system_status.matrix[x][y][z] == FULL) {
          system_status.matrix[x][y][z] = VOID;
          system_status.energy -= 12;
        } else {
          system_status.energy += 3;
        }
      }
    }
  }
  for (const auto& id : bot_ids) {
    v_cords.emplace_back(id, bot_status[id].pos, bot_status[id].pos);
  }
  v_cords.emplace_back(VolCord::kGVoid, r1, r2);

  // re-calculation of grounded_memo is needed
  valid_grounded_memo = false;
}
