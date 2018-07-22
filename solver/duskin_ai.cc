#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <deque>

#include "glog/logging.h"

#include "src/base/flags.h"
#include "solver/AI.h"

class DuskinAI : public AI {
public:
  DuskinAI(const vvv& src_model);
  ~DuskinAI() override = default;
  void Run() override;
private:
  struct FissionOP {
    Point p;
    int m;
  };
  const Point dirs[9] = {
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

  void Deliver();
  bool Delivered();
  void DoGVoid();
  void ReturnToOffice();
  std::pair<Command, Command> GenFusionFromFission(const auto& fission_command);
  static constexpr size_t num_bots = 8;
  const size_t R;
  std::array<Point, num_bots + 1> goals;
  std::array<std::queue<FissionOP>, num_bots + 1> commands_to_do;
  std::vector<std::vector<Command>> deliver_ops;
};

DuskinAI::DuskinAI(const vvv& src_model)
  : AI(src_model), R(src_model.size()) {
  for (size_t i = 1; i <= num_bots; i++) {
    goals[i] = dirs[i] * (R - 1);
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
}

bool DuskinAI::Delivered() {
  const auto& bot_status = ce->GetBotStatus();
  for (size_t i = 1; i <= num_bots; i++) {
    if (bot_status[i].pos != goals[i] || !commands_to_do[i].empty()) {
      return false;
    }
  }
  return true;
}

void DuskinAI::Deliver() {
  while (!Delivered()) {
    const auto& bot_status = ce->GetBotStatus();
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
    ce->Execute(commands);
    deliver_ops.emplace_back(std::move(commands));
  }
}

void DuskinAI::DoGVoid() {
  std::vector<Point> nds(num_bots+1);
  std::vector<Point> nd_pos(num_bots+1);
  const auto& bot_status = ce->GetBotStatus();
  for (size_t i = 1; i <= num_bots; i++) {
    const auto& pos = bot_status[i].pos;
    std::pair<int,int> p(std::min(1, pos.x),
                         std::min(1, pos.z));
    p.first = p.first ? -1 : 1;
    p.second = p.second ? -1 : 1;
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
  ce->Execute(commands);
}

std::pair<Command, Command> DuskinAI::GenFusionFromFission(const auto& fission_command) {
  int primary_bot_id = fission_command.id;
  const auto& bot_status = ce->GetBotStatus();
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

void DuskinAI::ReturnToOffice() {
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
          std::tie(fusion_p, fusion_s) = GenFusionFromFission(com);
          return_commands.emplace_back(std::move(fusion_p));
          return_commands.emplace_back(std::move(fusion_s));
        }
        break;
      default:
        LOG(FATAL) << "Unexpected: " << com.type;
        break;
      }
    }
    ce->Execute(std::move(return_commands));
  }
  // Halt
  ce->Execute({Command::make_halt(1)});
}

void DuskinAI::Run() {
  // Deliver
  Deliver();
  DoGVoid();
  ReturnToOffice();
  LOG(INFO) << ce->GetSystemStatus().energy;
}

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_src_filename.empty()) {
    std::cout << "need to pass --src_filename=/path/to/mdl";
    exit(1);
  }
  size_t R = 1;
  vvv src_model;
  if (!FLAGS_src_filename.empty()) {
    src_model = ReadMDL(FLAGS_src_filename);
    R = src_model.size();
  }

  LOG(INFO) << "R: " << R;
  if (R > 30) {
    LOG(ERROR) << "DuskinAI cannot clean up R > 30";
    exit(1);
    return 0;
  }
  auto duskin_ai = std::make_unique<DuskinAI>(src_model);
  duskin_ai->Run();
  duskin_ai->Finalize();
  return 0;
}
