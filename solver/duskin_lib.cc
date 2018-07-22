#include "duskin_lib.h"

#include <utility>
#include <queue>
#include <vector>

#include "src/command.h"
#include "src/command_util.h"
#include "glog/logging.h"

struct FissionOP {
  Point p;
  int m;
};

constexpr size_t num_bots = 8;

bool Delivered(CommandExecuter& ce, const std::array<Point, num_bots + 1>& goals, const std::array<std::queue<FissionOP>, num_bots + 1>& commands_to_do) {
  const auto& bot_status = ce.GetBotStatus();
  for (size_t i = 1; i <= num_bots; i++) {
    if (bot_status[i].pos != goals[i] || !commands_to_do[i].empty()) {
      return false;
    }
  }
  return true;
}

std::pair<Command, Command> GenFusionFromFission(CommandExecuter& ce, const auto& fission_command) {
  int primary_bot_id = fission_command.id;
  const auto& bot_status = ce.GetBotStatus();
  Point primary_pos = bot_status[primary_bot_id].pos;
  Point secondary_pos = primary_pos + fission_command.fission_.nd;
  int secondary_bot_id = -1;
  for (size_t i = 1; i <= num_bots; i++) {
    if (secondary_pos == bot_status[i].pos) {
      secondary_bot_id = i;
      break;
    }
  }

  LOG_ASSERT(secondary_bot_id != -1) << " " << primary_bot_id << " " << fission_command.fission_.nd;

  Command fusion_p = Command::make_fusion_p(primary_bot_id,
                                            fission_command.fission_.nd);
  Command fusion_s = Command::make_fusion_s(secondary_bot_id,
                                            fission_command.fission_.nd * (-1));

  return {fusion_p, fusion_s};
}

void RequestOneClean(CommandExecuter& ce, int leng_x, int leng_y, int leng_z) {
  LOG_ASSERT(leng_x >= 2);
  LOG_ASSERT(leng_y >= 2);
  LOG_ASSERT(leng_z >= 2);
  // leng >= 2
  // Gvoid (o, o, o) --- (o+leng, o+leng, o+leng)
  static const Point dirs[9] = {
    Point(-1,-1,-1), // dummy for 1-indexed
    Point(0, 0, 0), // 1
    Point(1, 0, 0), // 2
    Point(1, 1, 0), // 3
    Point(1, 1, 1), // 4
    Point(1, 0, 1), // 5
    Point(0, 1, 0), // 6
    Point(0, 1, 1), // 7
    Point(0, 0, 1), // 8
  };
  Point origin = ce.GetBotStatus()[1].pos;
  std::array<Point, num_bots + 1> goals;
  std::array<std::queue<FissionOP>, num_bots + 1> commands_to_do;
  std::vector<std::vector<Command>> deliver_ops;
  for (size_t i = 1; i <= num_bots; i++) {
    Point d(dirs[i].x * leng_x,
            dirs[i].y * leng_y,
            dirs[i].z * leng_z);
    goals[i] = d + origin;
  }
  // 1
  commands_to_do[1].push({Point(1, 0, 0), 3});
  commands_to_do[1].push({Point(0, 1, 0), 1});
  commands_to_do[1].push({Point(0, 0, 1), 0});
  // 2
  commands_to_do[2].push({Point(0, 1, 0), 1});
  commands_to_do[2].push({Point(0, 0, 1), 0});
  // 6
  commands_to_do[6].push({Point(0, 0, 1), 0});
  // 3
  commands_to_do[3].push({Point(0, 0, 1), 0});

  while (!Delivered(ce, goals, commands_to_do)) {
    const auto& bot_status = ce.GetBotStatus();
    std::vector<Command> commands;
    for (size_t i = 1; i <= num_bots; i++) {
      if (!bot_status[i].active) {
        continue;
      }
      const auto& pos = bot_status[i].pos;
      if (pos == goals[i]) {
        if (commands_to_do[i].empty()) {
          commands.emplace_back(Command::make_wait(i));
        } else {
          const auto p = commands_to_do[i].front().p;
          const auto m = commands_to_do[i].front().m;
          LOG_ASSERT(IsNCD(p));
          commands.emplace_back(Command::make_fission(i, p, m));
          commands_to_do[i].pop();
        }
      } else {
        constexpr uint32_t smove_max_leng = 15;
        const auto diff = goals[i] - pos;
        LOG_ASSERT(IsLCD(diff)) << i << " " << diff;
        uint32_t move_leng = std::min(CLen(diff), smove_max_leng);
        Point d(std::min(1, diff.x), std::min(1, diff.y), std::min(1, diff.z));
        commands.emplace_back(Command::make_smove(i, d * move_leng));
      }
    }
    ce.Execute(commands);
    deliver_ops.emplace_back(std::move(commands));
  }

  std::vector<Point> nds(num_bots+1);
  std::vector<Point> nd_pos(num_bots+1);
  const auto& bot_status = ce.GetBotStatus();
  for (size_t i = 1; i <= num_bots; i++) {
    const auto& pos = bot_status[i].pos;
    std::pair<int,int> p;
    if (pos.x == origin.x) {
      p.first = 1;
    } else {
      p.first = -1;
    }
    if (pos.z == origin.z) {
      p.second = 1;
    } else {
      p.second = -1;
    }
    nds[i] = Point(p.first, 0, p.second);
    nd_pos[i] = pos + nds[i];
  }

  std::vector<Point> fds(num_bots+1);
  for (size_t i = 1; i <= num_bots; i++) {
    Point max_diff(0, 0, 0);
    for (size_t j = 1; j <= num_bots; j++) {
      const auto diff = nd_pos[j] - nd_pos[i];
      if (MLen(max_diff) < MLen(diff)) {
        max_diff = diff;
      }
    }
    fds[i] = max_diff;
  }

  std::vector<Command> commands;
  for (size_t i = 1; i <= num_bots; i++) {
    commands.emplace_back(Command::make_gvoid(i, nds[i], fds[i]));
  }
  ce.Execute(commands);

  while(!deliver_ops.empty()) {
    const auto commands = std::move(deliver_ops.back());
    deliver_ops.pop_back();
    std::vector<Command> return_commands;
    for (const auto& com : commands) {
      switch(com.type) {
      case Command::Type::WAIT:
        return_commands.emplace_back(std::move(com));
        break;
      case Command::Type::SMOVE:
        return_commands.emplace_back(Command::make_smove(com.id, com.smove_.lld * (-1)));
        break;
      case Command::Type::FISSION:
        {
          // DO Fusion
          Command fusion_p,fusion_s;
          std::tie(fusion_p, fusion_s) = GenFusionFromFission(ce, com);
          return_commands.emplace_back(std::move(fusion_p));
          return_commands.emplace_back(std::move(fusion_s));
        }
        break;
      default:
        LOG(FATAL) << "Unexpected: " << com.type;
        break;
      }
    }
    ce.Execute(std::move(return_commands));
  }
}
