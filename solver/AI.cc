#include "solver/AI.h"

std::vector<Command> AI::FillCommand(std::vector<Command> commands, Command fill) const {
  const auto& bots = ce->GetBotStatus();
  bool has_order[CommandExecuter::kMaxNumBots + 1] = {};

  for (const auto& c : commands) {
    has_order[c.id] = true;
  }

  for (int i = 1; i <= CommandExecuter::kMaxNumBots; ++i) {
    if (has_order[i]) continue;
    if (!bots[i].active) continue;
    fill.id = i;
    commands.push_back(fill);
  }

  return commands;
}

/* static */
Command AI::GetStepSMove(int id, Point start, Point goal) {
  if (start.x != goal.x) {
    int d = std::min(kMAXSMOVE, std::abs(start.x - goal.x));
    if (goal.x < start.x) d = -d;
    Point p;
    p.x = d;

    return Command::make_smove(id, p);
  }

  if (start.y != goal.y) {
    int d = std::min(kMAXSMOVE, std::abs(start.y - goal.y));
    if (goal.y < start.y) d = -d;
    Point p;
    p.y = d;

    return Command::make_smove(id, p);
  }

  if (start.z != goal.z) {
    int d = std::min(kMAXSMOVE, std::abs(start.z - goal.z));
    if (goal.z < start.z) d = -d;
    Point p;
    p.z = d;

    return Command::make_smove(id, p);
  }

  LOG(FATAL) << "same point?";
  return Command::make_wait(id);

}

/* static */
std::vector<Command> AI::GetSMoves(int id, Point start, Point goal) {
  std::vector<Command> smoves;

  while (start != goal) {
    auto smove = GetStepSMove(id, start, goal);
    smoves.push_back(smove);
    start += smove.smove_.lld;
  }

  return smoves;
}


std::vector<Command> AI::MultiStepSMove(const std::vector<std::pair<int, Point>>& targets) const {
  std::vector<Command> command;

  for (const auto& b : targets) {
    if (bot(b.first).pos == b.second) continue;
    command.push_back(GetStepSMove(b.first, bot(b.first).pos, b.second));
  }
  
  return command;
}
