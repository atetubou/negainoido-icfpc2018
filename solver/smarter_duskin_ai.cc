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

#include "duskin_lib.h"

class SmarterDuskinAI : public AI {
public:
  SmarterDuskinAI(const vvv& src_model);
  ~SmarterDuskinAI() override = default;
  void Run() override;
  bool Accept();
private:
  const vvv model;
  const size_t R;
  Point minP;
  Point maxP;
};

SmarterDuskinAI::SmarterDuskinAI(const vvv& src_model)
  : AI(src_model), model(src_model), R(src_model.size()),
    minP(300, 300, 300), maxP(0,0,0) {}

bool SmarterDuskinAI::Accept() {
  for (size_t i = 0; i < R; i++) {
    for (size_t j = 0; j < R; j++) {
      for (size_t k = 0; k < R; k++) {
        if (!model[i][j][k]) {
          continue;
        }
        minP.x = std::min(minP.x, (int) i);
        minP.y = std::min(minP.y, (int) j);
        minP.z = std::min(minP.z, (int) k);
        maxP.x = std::max(maxP.x, (int) i);
        maxP.y = std::max(maxP.y, (int) j);
        maxP.z = std::max(maxP.z, (int) k);
      }
    }
  }
  auto leng = CLen(maxP - minP);
  return leng <= 30;
}

std::vector<Command> Goto(Point target) {
  constexpr int smove_max_leng = 15;
  int tx = target.x / smove_max_leng;
  int ty = target.y / smove_max_leng;
  int tz = target.z / smove_max_leng;
  std::vector<Command> commands;
  for (int i = 0; i < tx; i++)
    commands.emplace_back(Command::make_smove(1, Point(smove_max_leng, 0, 0)));
  for (int i = 0; i < ty; i++)
    commands.emplace_back(Command::make_smove(1, Point(0, smove_max_leng, 0)));
  for (int i = 0; i < tz; i++)
    commands.emplace_back(Command::make_smove(1, Point(0, 0, smove_max_leng)));

  // TODO(hiroh): optimize more.
  if (target.x % smove_max_leng)
    commands.emplace_back(Command::make_smove(1, Point(target.x % smove_max_leng, 0, 0)));
  if (target.y % smove_max_leng)
    commands.emplace_back(Command::make_smove(1, Point(0, target.y % smove_max_leng, 0)));
  if (target.z % smove_max_leng)
    commands.emplace_back(Command::make_smove(1, Point(0, 0, target.z % smove_max_leng)));
  return commands;
}

void SmarterDuskinAI::Run() {
  Point origin(minP.x - 1, minP.y, minP.z - 1);
  // Go to Origin.
  LOG(INFO) << "minP=" << minP << ", maxP=" << maxP;
  auto commands = Goto(origin);
  for (const auto& com : commands) {
    ce->Execute({com});
  }
  int leng_x = maxP.x - origin.x + 1;
  int leng_y = maxP.y - origin.y;
  int leng_z = maxP.z - origin.z + 1;
  RequestOneClean(*ce, leng_x, leng_y, leng_z);
  // Back to Origin.
  for (const auto& com : commands) {
    Command rev_com;
    LOG_ASSERT(com.type == Command::Type::SMOVE);
    rev_com = Command::make_smove(1, com.smove_.lld * -1);
    ce->Execute({rev_com});
  }
  ce->Execute({Command::make_halt(1)});
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
  auto smarter_duskin_ai = std::make_unique<SmarterDuskinAI>(src_model);
  if (!smarter_duskin_ai->Accept()) {
    LOG(ERROR) << "Smarter Duskin AI is not smart enough to clean this.";
    exit(1);
    return 0;
  }
  smarter_duskin_ai->Run();
  smarter_duskin_ai->Finalize();
  return 0;
}
