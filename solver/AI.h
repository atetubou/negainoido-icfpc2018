#ifndef AI_H
#define AI_H

#include <iostream>
#include <memory>
#include "src/base/base.h"
#include "src/command_executer.h"

class AI {
 protected:
  std::unique_ptr<CommandExecuter> ce;
 public:
 AI(vvv model) : ce(std::make_unique<CommandExecuter>(model, true)) {}
  virtual ~AI() = default;
  virtual void Run() = 0;
  void Finalize() {
    std::cout << Json2Binary(ce->GetJson());
  }
};

#endif
