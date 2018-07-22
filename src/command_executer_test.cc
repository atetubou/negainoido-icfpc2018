#include "command_executer.h"

#include <memory>

#include "gtest/gtest.h"
#include "command.h"

void CheckDefaultBotHasAlltheSeeds(const std::array<CommandExecuter::BotStatus, CommandExecuter::kMaxNumBots+1>& b) {
  EXPECT_TRUE(b[1].seeds.find(0) == b[1].seeds.end());
  for (size_t i = 2; i <= CommandExecuter::kMaxNumBots; i++) {
    const auto& bst = b[i];
    EXPECT_EQ(bst.active, false);
    EXPECT_EQ(bst.seeds.size(), 0u);
    EXPECT_TRUE(b[1].seeds.find(i) != b[1].seeds.end());
  }
  EXPECT_EQ(b[1].seeds.size(), CommandExecuter::kMaxNumBots - 1);
}

TEST(CommandExecuter, CreateAndDestory) {
  const int R = 250;
  auto ce = std::make_unique<CommandExecuter>(R, false);
  EXPECT_EQ(ce->GetActiveBotsNum(), 1u);
  const auto& s = ce->GetSystemStatus();
  const auto& b = ce->GetBotStatus();
  EXPECT_EQ(s.R, R);
  EXPECT_EQ(s.energy, 0LL);
  EXPECT_EQ(s.harmonics, LOW);
  for (size_t i = 0; i < CommandExecuter::kMaxResolution; i++) {
    for (size_t j = 0; j < CommandExecuter::kMaxResolution; j++) {
      for (size_t k = 0; k < CommandExecuter::kMaxResolution; k++) {
        EXPECT_EQ(s.matrix[i][j][k], VOID);
      }
    }
  }
  EXPECT_EQ(b[1].active, true);
  CheckDefaultBotHasAlltheSeeds(b);
}

TEST(CommandExecuter, Halt) {
  const int R = 10;
  auto ce = std::make_unique<CommandExecuter>(R, false);
  EXPECT_EQ(ce->GetActiveBotsNum(), 1u);
  Command command = Command::make_halt(1);
  std::vector<Command> com = {command};
  ce->Execute(com);
  EXPECT_EQ(ce->GetActiveBotsNum(), 0u);
}

TEST(CommandExecuter, Wait) {
  const int R = 10;
  auto ce = std::make_unique<CommandExecuter>(R, false);
  const auto s_bef_energy = ce->GetSystemStatus().energy;
  const size_t bef_active_bots = ce->GetActiveBotsNum();
  // const auto& b_bef = ce->GetBotStatus();
  Command command = Command::make_wait(1);
  std::vector<Command> com = {command};
  ce->Execute(com);
  const auto& s_aft = ce->GetSystemStatus();
  const auto& b_aft = ce->GetBotStatus();
  EXPECT_EQ(s_aft.energy - s_bef_energy, 3 * R * R * R + 20 * bef_active_bots);
  EXPECT_EQ(s_aft.harmonics, LOW);
  EXPECT_EQ(b_aft[1].active, true);
  EXPECT_EQ(b_aft[1].pos, Point(0,0,0));
  CheckDefaultBotHasAlltheSeeds(b_aft);
}

TEST(CommandExecuter, TwoFlip) {
  const int R = 10;
  auto ce = std::make_unique<CommandExecuter>(R, false);
  const auto s_bef_energy = ce->GetSystemStatus().energy;
  const auto s_bef_harmonics = ce->GetSystemStatus().harmonics;
  EXPECT_EQ(s_bef_harmonics, LOW);

  const size_t bef_active_bots = ce->GetActiveBotsNum();
  Command command1 = Command::make_flip(1);
  std::vector<Command> com1 = {command1};
  ce->Execute(com1);
  const auto s_flp1_energy = ce->GetSystemStatus().energy;
  const auto s_flp1_harmonics = ce->GetSystemStatus().harmonics;
  EXPECT_EQ(s_flp1_harmonics, HIGH);
  EXPECT_EQ(s_flp1_energy - s_bef_energy, 3 * R * R * R + 20 * bef_active_bots);

  const size_t flp1_active_bots = ce->GetActiveBotsNum();
  Command command2 = Command::make_flip(1);
  std::vector<Command> com2 = {command2};
  ce->Execute(com2);
  const auto s_flp2_energy = ce->GetSystemStatus().energy;
  const auto s_flp2_harmonics = ce->GetSystemStatus().harmonics;
  EXPECT_EQ(s_flp2_energy - s_flp1_energy, 30 * R * R * R + 20 * flp1_active_bots);
  EXPECT_EQ(s_flp2_harmonics, LOW);

  const auto& b_aft = ce->GetBotStatus();
  EXPECT_EQ(b_aft[1].active, true);
  EXPECT_EQ(b_aft[1].pos, Point(0,0,0));
  CheckDefaultBotHasAlltheSeeds(b_aft);
}

TEST(CommandExecuter, OneSMove) {
  const int R = 250;
  auto ce = std::make_unique<CommandExecuter>(R, false);
  const auto s_bef_energy = ce->GetSystemStatus().energy;
  const size_t bef_active_bots = ce->GetActiveBotsNum();
  const int length = 15;
  Command command = Command::make_smove(1, Point(length,0,0));
  std::vector<Command> com = {command};
  ce->Execute(com);
  const auto& s_aft = ce->GetSystemStatus();
  const auto& b_aft = ce->GetBotStatus();
  EXPECT_EQ(s_aft.energy - s_bef_energy, 3 * R * R * R + 20 * bef_active_bots + 2 * length);
  EXPECT_EQ(s_aft.harmonics, LOW);
  EXPECT_EQ(b_aft[1].pos, Point(length,0,0));
  EXPECT_EQ(b_aft[1].active, true);
  CheckDefaultBotHasAlltheSeeds(b_aft);
}

TEST(CommandExecuter, OneLMove) {
  const int R = 250;
  auto ce = std::make_unique<CommandExecuter>(R, false);
  const auto s_bef_energy = ce->GetSystemStatus().energy;
  const size_t bef_active_bots = ce->GetActiveBotsNum();
  const int length = 5;
  Command command = Command::make_lmove(1, Point(0, length, 0), Point(length, 0, 0));
  std::vector<Command> com = {command};
  ce->Execute(com);
  const auto& s_aft = ce->GetSystemStatus();
  const auto& b_aft = ce->GetBotStatus();
  EXPECT_EQ(s_aft.energy - s_bef_energy, 3 * R * R * R + 20 * bef_active_bots + 2 * (length + 2 + length));
  EXPECT_EQ(s_aft.harmonics, LOW);
  EXPECT_EQ(b_aft[1].pos, Point(length,length,0));
  EXPECT_EQ(b_aft[1].active, true);
  CheckDefaultBotHasAlltheSeeds(b_aft);
}

TEST(CommandExecuter, TwoFill) {
  const int R = 20;
  auto ce = std::make_unique<CommandExecuter>(R, false);
  const auto s_bef_energy = ce->GetSystemStatus().energy;
  const size_t bef_active_bots = ce->GetActiveBotsNum();
  Command command1 = Command::make_fill(1, Point(1, 0, 1));
  std::vector<Command> com1 = {command1};
  ce->Execute(com1);
  const auto s_flp1_energy = ce->GetSystemStatus().energy;
  EXPECT_EQ(s_flp1_energy - s_bef_energy, 3 * R * R * R + 20 * bef_active_bots + 12);
  EXPECT_EQ(ce->GetSystemStatus().matrix[1][0][1], FULL);
  const size_t flp1_active_bots = ce->GetActiveBotsNum();
  Command command2 = Command::make_fill(1, Point(1, 0, 1));
  std::vector<Command> com2 = {command2};
  ce->Execute(com2);
  const auto s_flp2_energy = ce->GetSystemStatus().energy;
  EXPECT_EQ(s_flp2_energy - s_flp1_energy, 3 * R * R * R + 20 * flp1_active_bots + 6);
  EXPECT_EQ(ce->GetSystemStatus().matrix[1][0][1], FULL);
  const auto& b_aft = ce->GetBotStatus();
  EXPECT_EQ(b_aft[1].pos, Point(0,0,0));
  EXPECT_EQ(b_aft[1].active, true);
  CheckDefaultBotHasAlltheSeeds(b_aft);
}

TEST(CommandExecuter, FillAndRemove) {
  const int R = 20;
  auto ce = std::make_unique<CommandExecuter>(R, false);
  const auto s_bef_energy = ce->GetSystemStatus().energy;
  const size_t bef_active_bots = ce->GetActiveBotsNum();
  Command command1 = Command::make_fill(1, Point(1, 0, 1));
  std::vector<Command> com1 = {command1};
  ce->Execute(com1);
  const auto s_flp1_energy = ce->GetSystemStatus().energy;
  EXPECT_EQ(s_flp1_energy - s_bef_energy, 3 * R * R * R + 20 * bef_active_bots + 12);
  EXPECT_EQ(ce->GetSystemStatus().matrix[1][0][1], FULL);
  const size_t flp1_active_bots = ce->GetActiveBotsNum();
  Command command2 = Command::make_void(1, Point(1, 0, 1));
  std::vector<Command> com2 = {command2};
  ce->Execute(com2);
  const auto s_flp2_energy = ce->GetSystemStatus().energy;
  EXPECT_EQ(s_flp2_energy - s_flp1_energy, 3 * R * R * R + 20 * flp1_active_bots - 12);
  EXPECT_EQ(ce->GetSystemStatus().matrix[1][0][1], VOID);
  const auto& b_aft = ce->GetBotStatus();
  EXPECT_EQ(b_aft[1].pos, Point(0,0,0));
  EXPECT_EQ(b_aft[1].active, true);
  CheckDefaultBotHasAlltheSeeds(b_aft);
}


TEST(CommandExecuter, OneFission) {
  const int R = 10;
  auto ce = std::make_unique<CommandExecuter>(R, false);
  const auto s_bef_energy = ce->GetSystemStatus().energy;
  const size_t bef_active_bots = ce->GetActiveBotsNum();
  // const auto& b_bef = ce->GetBotStatus();
  const int M = 15;
  Command command = Command::make_fission(1, Point(0, 1, 1), M);
  std::vector<Command> com = {command};
  ce->Execute(com);
  const auto s_aft_energy = ce->GetSystemStatus().energy;
  const size_t aft_active_bots = ce->GetActiveBotsNum();
  EXPECT_EQ(aft_active_bots, 2);
  EXPECT_EQ(s_aft_energy - s_bef_energy, 3 * R * R * R + 20 * bef_active_bots + 24);
  const auto& b_aft = ce->GetBotStatus();
  EXPECT_EQ(b_aft[1].pos, Point(0,0,0));
  EXPECT_EQ(b_aft[2].pos, Point(0,1,1));
  EXPECT_EQ(b_aft[1].active, true);
  EXPECT_EQ(b_aft[2].active, true);
  EXPECT_EQ(b_aft[1].seeds.size(), CommandExecuter::kMaxNumBots - M - 1 - 1);
  EXPECT_EQ(b_aft[2].seeds.size(), M);
  for (int i = 3 ; i < M + 3; i++) {
    EXPECT_TRUE(b_aft[2].seeds.find(i) != b_aft[2].seeds.end());
  }
  for (int i = M + 3 ; i <= CommandExecuter::kMaxNumBots; i++) {
    EXPECT_TRUE(b_aft[1].seeds.find(i) != b_aft[1].seeds.end());
  }
}

// TODO(hiroh): test Fusion
// TODO(hiroh): test GFill
// TODO(hiroh): test Gvoid
