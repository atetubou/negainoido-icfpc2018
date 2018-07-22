#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <deque>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "src/command_util.h"
#include "src/command_executer.h"

#include "src/base/flags.h"
#include "solver/AI.h"

struct FissionInfo {
  int id;
  Point d;
  int seed_m;
};

class UdonAI;

class Agent {
public:
  UdonAI* udonAI;

  int bot_id;

  Point fission_pos;
  std::queue<FissionInfo> children;
  int seeds_num;

  Point start_pos;
  Point area_min, area_max;


  bool go_finished;

  Command Go(std::vector<Agent>& agents);

  void InitWork();
  void LastLayer();
  Command Work();
  Command Fusion(std::vector<Agent>& agents);

  Point GetCurPos();

  bool InArea(Point&);

  bool GoFinished();
  bool WorkFinished();
  bool FusionFinished();
  bool GoHomeFinished();

  void PushCommand(Command);
  void PushMoveStraight(Point);

  void Dfs(Point & cur);
};

bool visited[250][250][250];

class UdonAI : public AI {
public:
  std::vector<Agent> agents;
  static constexpr size_t num_bots = 8;
  vvv model;
  UdonAI(const vvv &model_) : AI(model_) {

    int r = model_.size();
    model = vvv(r, vv(r, v(r, 0)));

    for (int i = 0; i < r; i++)
      for (int j = 0; j < r; j++)
        for (int k = 0; k < r; k++)
          model[i][j][k] = model_[i][j][k];
  }
  ~UdonAI() override = default;
  void Run();

  void RegisterAgents();
  void AgentsGo();
  void DoGFill();
  void AgentsWork();
  void AgentsFusion();
  void AgentsLastLayer();
  void AgentsGoHome();

  Point GetPosition(int bot_id);
  bool AgentValid(int bot_id);

  CommandExecuter::SystemStatus GetSystemStatus() {
    return ce->GetSystemStatus();
  }

  std::queue<Command> agents_commands[20];
};

inline int sign(int x) {
  if (x == 0) {
    return 0;
  } else if (0 < x) {
    return 1;
  } else {
    return -1;
  }
}

Point sign_vec(Point p) {
  return Point(sign(p.x),
               sign(p.y),
               sign(p.z));
}

Command make_go_straight(const int id, const Point& src, const Point& dst) {
   constexpr uint32_t smove_max_leng = 12;
   auto diff = dst - src;

   if (diff.x != 0) {
     diff.y = diff.z = 0;
   } else if (diff.y != 0) {
     diff.z = 0;
   }

   LOG_ASSERT(IsLCD(diff)) << id << " " << diff;

   uint32_t move_leng = std::min(CLen(diff), smove_max_leng);
   Point d(sign(diff.x) * std::min(1, abs(diff.x)),
           sign(diff.y) * std::min(1, abs(diff.y)),
           sign(diff.z) * std::min(1, abs(diff.z)));

   CHECK(d * move_leng != Point(-15, 0, 0));
   return Command::make_smove(id, d * move_leng);
}

void Agent::PushCommand(Command com) {
  udonAI->agents_commands[bot_id].push(com);
}

void Agent::PushMoveStraight(Point p) {

  for(int x = 0; x < abs(p.x); x++) {
    PushCommand(Command::make_smove(bot_id, Point(sign(p.x), 0, 0)));
  }

  for(int y = 0; y < abs(p.y); y++) {
    PushCommand(Command::make_smove(bot_id,  Point(0, sign(p.y), 0)));
  }

  for(int z = 0; z < abs(p.z); z++) {
    PushCommand(Command::make_smove(bot_id, Point(0, 0, sign(p.z))));
  }
}

bool Agent::InArea(Point& p) {

  return (area_min.x <= p.x && p.x <= area_max.x &&
          area_min.y <= p.y && p.y <= area_max.y &&
          area_min.z <= p.z && p.z <= area_max.z);
}

Point Agent::GetCurPos() {
  return udonAI->GetPosition(bot_id);
}

bool Agent::GoFinished() {
  return children.empty() && GetCurPos() == start_pos;
}

Command Agent::Go(std::vector<Agent>& agents) {
  if (children.empty()) {
    if (GetCurPos() == start_pos) {
      return Command::make_wait(bot_id);
    }
    return make_go_straight(bot_id, GetCurPos(), start_pos);
  } else {
    if (GetCurPos() != fission_pos) {
      return make_go_straight(bot_id, GetCurPos(), fission_pos);
    }

    // Fission
    auto finfo = children.front();
    children.pop();
    return Command::make_fission(bot_id, finfo.d, finfo.seed_m);
  }
}

void Agent::Dfs(Point & cur) {
  const Point dxz[] = {
    Point(1,0,0), Point(-1,0,0),
    Point(0,0,1), Point(0,0,-1),
  };

  for(int i=0;i<4; ++i) {
    Point p = cur + dxz[i];

    if (!InArea(p)) continue;
    if(visited[p.x][p.y][p.z]) continue;

    if (udonAI->model[p.x][p.y][p.z] == FULL) {
      continue;
    }

    visited[p.x][p.y][p.z] = true;


    PushCommand(Command::make_void(bot_id, p - cur));

    PushCommand(Command::make_smove(bot_id, p - cur));

    Dfs(p);

    PushCommand(Command::make_smove(bot_id, cur - p));
  }
}

void Agent::InitWork() {
  if (bot_id != 3 && bot_id != 4 && bot_id != 6 && bot_id != 7) {
    return;
  }

  std::cerr<<"Bot" << bot_id << " "<< area_min << " "<< area_max<<std::endl;

  int R = udonAI->GetSystemStatus().R;

  for(int x = 0; x < R; x++)
    for(int y = 0; y < R; y++)
      for(int z = 0; z < R; z++)
        visited[x][y][z] = false;


  // Go Area Ma
  std::stack<Point> s;
  Point cur = GetCurPos();
  for (int xx = 0; xx < area_max.y - area_min.y; xx++) {
    Dfs(cur);

    if (cur.y != 1) {
      PushCommand(Command::make_smove(bot_id, Point(0,-1,0)));
      cur.y--;
    }
  }
}

void Agent::LastLayer() {
  CHECK(bot_id != 3 && bot_id != 4 && bot_id != 6 && bot_id != 7);

  std::cerr<<"Bot" << bot_id << " "<< area_min << " "<< area_max<<std::endl;

  int R = udonAI->GetSystemStatus().R;

  for(int x = 0; x < R; x++)
    for(int y = 0; y < R; y++)
      for(int z = 0; z < R; z++)
        visited[x][y][z] = false;

  // Go Area Ma
  std::stack<Point> s;
  Point cur = GetCurPos();
  Dfs(cur);
}



Command Agent::Work() {
  return Command::make_wait(bot_id);
}

Command Agent::Fusion(std::vector<Agent>& agents) {
  return Command::make_wait(bot_id);
}

Point UdonAI::GetPosition(int bot_id) {
  return ce->GetBotStatus()[bot_id].pos;
}

bool UdonAI::AgentValid(int bot_id) {
  return ce->GetBotStatus()[bot_id].active;
}

void UdonAI::RegisterAgents() {
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

  const int R = ce->GetSystemStatus().R;

  for (size_t i = 1; i <= 8; i++) {
    Agent agent;
    agent.udonAI = this;
    agent.bot_id = i;
    agent.fission_pos = dirs[i] * (R-1);
    agent.start_pos = dirs[i] * (R-1);

    agent.area_min = Point(0,0,0);
    agent.area_max = Point(0,0,0);

    agents.push_back(agent);
  }

  agents[1 - 1].children.push({2, Point(1, 0, 0), 3});
  agents[1 - 1].children.push({6, Point(0, 1, 0), 1});
  agents[1 - 1].children.push({8, Point(0, 0, 1), 0});

  agents[2 - 1].children.push({3, Point(0, 1, 0), 1});
  agents[2 - 1].children.push({5, Point(0, 0, 1), 0});

  agents[6 - 1].children.push({7, Point(0, 0, 1), 0});
  agents[3 - 1].children.push({4, Point(0, 0, 1), 0});


  //
  agents[4 - 1].area_min = Point(R/2 + 1, 0, R/2 +1);
  agents[4 - 1].area_max = Point(R-1, R-1, R-1);

  agents[3 - 1].area_min = Point(R/2 + 1, 0, 0);
  agents[3 - 1].area_max = Point( R-1, R-1, R/2);

  agents[6 - 1].area_min = Point(0, 0, 0);
  agents[6 - 1].area_max = Point(R/2, R-1, R/2);

  agents[7 - 1].area_min = Point(0, 0, R/2 + 1);
  agents[7 - 1].area_max = Point(R/2, R-1, R - 1);


  agents[5 - 1].area_min = Point(R/2 + 1, 0, R/2 +1);
  agents[5 - 1].area_max = Point(R-1, R-1, R-1);

  agents[2 - 1].area_min = Point(R/2 + 1, 0, 0);
  agents[2 - 1].area_max = Point( R-1, R-1, R/2);

  agents[1 - 1].area_min = Point(0, 0, 0);
  agents[1 - 1].area_max = Point(R/2, R-1, R/2);

  agents[8 - 1].area_min = Point(0, 0, R/2 + 1);
  agents[8 - 1].area_max = Point(R/2, R-1, R - 1);

}

void UdonAI::AgentsGo() {
  bool done = false;
  while (!done) {
    done = true;
    std::vector<Command> coms;
    for (auto& agent : agents) {

      if (!AgentValid(agent.bot_id)) {
        std::cerr << "Bot " << agent.bot_id << ": ___" << std::endl;
        continue;
      }

      auto com = agent.Go(agents);
      done &= agent.GoFinished();

      std::cerr << "Bot " << agent.bot_id << ": " << agent.GetCurPos() << std::endl;

      coms.push_back(com);
    }

    std::cerr << Command::CommandsToJson(coms);
    ce->Execute(coms);
  }
}

void UdonAI::DoGFill() {
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
    commands.emplace_back(Command::make_gfill(i, nds[i], fds[i]));
  }

  ce->Execute(commands);
}

void UdonAI::AgentsWork() {
  for (auto& agent : agents) {
    if (!AgentValid(agent.bot_id)) continue;
    agent.InitWork();
  }

  bool done = false;
  while (!done) {
    done = true;
    std::vector<Command> commands;
    for (auto& agent : agents) {
      if (!AgentValid(agent.bot_id)) continue;
      if (agents_commands[agent.bot_id].empty()) {
        commands.push_back(Command::make_wait(agent.bot_id));
      } else {
        done = false;
        auto com = agents_commands[agent.bot_id].front();
        agents_commands[agent.bot_id].pop();
        commands.push_back(com);
      }
    }

    ce->Execute(commands);
  }
}


void UdonAI::AgentsFusion() {
  std::vector<Command> commands;

  for (auto & agent : agents) {
    if (agent.GetCurPos().y == 1) {
      auto com = Command::make_fusion_s(agent.bot_id, Point(0, -1, 0));
      commands.push_back(com);
    } else {
      CHECK(agent.GetCurPos().y == 0);
      auto com = Command::make_fusion_p(agent.bot_id, Point(0, 1, 0));
      commands.push_back(com);
    }
  }

  ce->Execute(commands);
}

void UdonAI::AgentsLastLayer() {
  std::vector<std::queue<Command>> coms;

  for (auto& agent : agents) {
    if (AgentValid(agent.bot_id))
      agent.LastLayer();
  }

  bool done = false;
  while (!done) {
    done = true;
    std::vector<Command> commands;

    for (auto& agent : agents) {
      if (!AgentValid(agent.bot_id)) continue;
      if (agents_commands[agent.bot_id].empty()) {
        commands.push_back(Command::make_wait(agent.bot_id));
      } else {
        done = false;
        auto com = agents_commands[agent.bot_id].front();
        agents_commands[agent.bot_id].pop();
        commands.push_back(com);
      }
    }

    ce->Execute(commands);
  }
}


void UdonAI::AgentsGoHome() {
  CHECK(GetPosition(0) == Point(0, 0, 0));

  const int R = ce->GetSystemStatus().R;

  agents[4].PushMoveStraight(Point(0, 0, -(R-2)));
  agents[7].PushMoveStraight(Point(0, 0, -(R-2)));

  bool done = false;
  while (!done) {
    done = true;
    std::vector<Command> commands;
    for (auto& agent : agents) {
      if (!AgentValid(agent.bot_id)) continue;
      if (agents_commands[agent.bot_id].empty()) {
        commands.push_back(Command::make_wait(agent.bot_id));
      } else {
        done = false;
        auto com = agents_commands[agent.bot_id].front();
        agents_commands[agent.bot_id].pop();
        commands.push_back(com);
      }
    }
    ce->Execute(commands);
  }

  std::vector<Command> fusion_com;
  fusion_com.push_back(Command::make_fusion_p(1, Point(0,0,1)));
  fusion_com.push_back(Command::make_fusion_p(2, Point(0,0,1)));
  fusion_com.push_back(Command::make_fusion_s(5, Point(0,0,-1)));
  fusion_com.push_back(Command::make_fusion_s(8, Point(0,0,-1)));
  ce->Execute(fusion_com);


  while(agents[1].GetCurPos().x > 1) {
    std::vector<Command> coms;
    coms.push_back(Command::make_wait(1));
    coms.push_back(Command::make_smove(2, Point(- std::min(agents[1].GetCurPos().x - 1, 15), 0, 0)));
    ce->Execute(coms);
  }

  std::vector<Command> fusion_com2;
  fusion_com2.push_back(Command::make_fusion_p(1, Point(1,0,0)));
  fusion_com2.push_back(Command::make_fusion_s(2, Point(-1,0,0)));
  ce->Execute(fusion_com2);
}

void UdonAI::Run() {
  //CHECK(ce->GetSystemStatus().R <= 30);
  std::cerr<<"R = "<< ce->GetSystemStatus().R <<std::endl;
  RegisterAgents();

  // Go to work places
  AgentsGo();


  // Work
  DoGFill();

  AgentsWork();

  // Fusion
  AgentsFusion();

  AgentsLastLayer();

  // Go home
  AgentsGoHome();

  std::vector<Command> coms = {Command::make_halt(1)};
  ce->Execute(coms);
}

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_tgt_filename.empty()) {
    std::cout << "need to pass --tgt_filename=/path/to/mdl";
    exit(1);
  }

  vvv tgt_model;
  if (!FLAGS_tgt_filename.empty()) {
    tgt_model = ReadMDL(FLAGS_tgt_filename);
  }

  auto udon_ai = std::make_unique<UdonAI>(tgt_model);
  udon_ai->Run();
  udon_ai->Finalize();
}
