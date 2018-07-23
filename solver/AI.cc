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
