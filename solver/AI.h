#ifndef AI_H
#define AI_H

#include <iostream>
#include <memory>
#include <vector>

#include "src/base/base.h"
#include "src/command_executer.h"

class AI {
 private:
  static const int kMAXSMOVE = 15;

 protected:
  std::unique_ptr<CommandExecuter> ce;
 public:
  const CommandExecuter::BotStatus& bot(int id) const {
    return ce->GetBotStatus()[id];
  }

 AI(int R) : ce(std::make_unique<CommandExecuter>(R, true)) {}

 AI(vvv model) : ce(std::make_unique<CommandExecuter>(model, true)) {}

  std::vector<Command> FillCommand(std::vector<Command> commands,
                                   Command c) const;

  std::vector<Command> FillWait(std::vector<Command> commands) const;

  // id番目のボットをstartからgoalに一ステップ近づけるようなSMoveを返す
  static Command GetStepSMove(int id, Point start, Point goal);

  // id番目のボットをstartからgoalに向かわせるSMoveの列を返す
  static std::vector<Command> GetSMoves(int id, Point start, Point goal);

  // 指定されたボットをそれぞれのgoalに一ステップ近づけるようなSMoveの列を返す
  std::vector<Command> MultiStepSMove(const std::vector<std::pair<int, Point>>& targets) const;

  virtual ~AI() = default;
  virtual void Run() = 0;
  void Finalize() {
    std::cout << Json2Binary(ce->GetJson());
  }
};

#endif
