#ifndef AI_H
#define AI_H

#include <iostream>
#include <memory>
#include <vector>

#include "src/base/base.h"
#include "src/command_executer.h"

class AI {
 protected:
  std::unique_ptr<CommandExecuter> ce;
 public:
 AI(vvv model) : ce(std::make_unique<CommandExecuter>(model, true)) {}

  std::vector<Command> FillCommand(std::vector<Command> commands,
                                   Command c) const;

  virtual ~AI() = default;
  virtual void Run() = 0;
  void Finalize() {
    std::cout << Json2Binary(ce->GetJson());
  }
};

#endif
