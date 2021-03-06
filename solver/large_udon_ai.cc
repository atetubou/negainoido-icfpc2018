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

class LargeUdonAI;

class Agent {
public:
  LargeUdonAI* udonAI;

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

  void PushCommand(Command);
  void PushMoveStraight(Point);

  void Dfs(Point & cur);

};

bool visited[250][250][250];
const int MAX_BLOCK_SIZE = 30;
class LargeUdonAI : public AI {
public:
  int R;
  std::vector<Agent> agents;
  static constexpr size_t num_bots = 8;


  vvv model;
  LargeUdonAI(const vvv &model_) : AI(model_.size()) {

    R = model_.size();
    std::cerr << "R = " << R << std::endl;

    model = vvv(R, vv(R, v(R, 0)));

    for (int i = 0; i < R; i++)
      for (int j = 0; j < R; j++)
        for (int k = 0; k < R; k++)
          model[i][j][k] = model_[i][j][k];

    DevideRegion();
  }
  ~LargeUdonAI() override = default;
  void Run();

  void OldRun();

  void RegisterAgents();
  void AgentsGo();
  void DoGFill();
  void DoAllMove(Point);
  void DoAllMoveForce(Point);
  void DoPartialMoveForce(Point, std::set<int>);
  void DoAllMoveForceAux(Point , std::set<int>);
  void Interleave(std::vector<std::vector<Command>> coms);

  void AgentsWork();
  void AgentsFusion();
  void AgentsLastLayer();
  void AgentsGoHome();

  Point GetPosition(int bot_id);
  bool AgentValid(int bot_id);
  bool OnBoarder(Point);

  bool InSpace(Point p);
  void DevideRegion();
  void RegionBfs(int color, Point& cur);

  CommandExecuter::SystemStatus GetSystemStatus() {
    return ce->GetSystemStatus();
  }

  std::queue<Command> agents_commands[20];

  std::pair<int, int> DFSRegion[250][250][250];
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

int Id2Region(int id) {
  if (id == 1 || id == 6) {
    return 0;
  } else if (id == 3 || id == 2) {
    return 1;
  } else if (id == 4 || id == 5) {
    return 2;
  } else if (id == 7 || id == 8) {
    return 3;
  }
  CHECK(false);
  return -1;
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
  //std::cerr<<"InArea: p" << p << " "<< (udonAI->InSpace(p) && udonAI->DFSRegion[p.x][p.y][p.z].second == bot_id )<< std::endl;
  return udonAI->InSpace(p) &&
    udonAI->DFSRegion[p.x][p.y][p.z].second == Id2Region(bot_id);
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
    Point(0,0,1),
    Point(-1,0,0),
    Point(0,0,-1),
    Point(1,0,0),
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

  //std::cerr<<"Bot" << bot_id << " "<< area_min << " "<< area_max<<std::endl;

  int R = udonAI->R;

  for(int x = 0; x < R; x++)
    for(int y = 0; y < R; y++)
      for(int z = 0; z < R; z++)
        visited[x][y][z] = false;


  // Go Area Ma
  std::stack<Point> s;
  Point cur = GetCurPos();

  for (int xx = 0; xx <= R; xx++) {
    Dfs(cur);

    if (cur.y != 0) {
      PushCommand(Command::make_smove(bot_id, Point(0,-1,0)));
      cur.y--;
    }
  }
}

void Agent::LastLayer() {
  CHECK(bot_id != 3 && bot_id != 4 && bot_id != 6 && bot_id != 7);

  //std::cerr<<"Bot" << bot_id << " "<< area_min << " "<< area_max<<std::endl;

  int R = udonAI->R;

  for(int x = 0; x < R; x++)
    for(int y = 0; y < R; y++)
      for(int z = 0; z < R; z++)
        visited[x][y][z] = false;

  Point cur = GetCurPos();
  Dfs(cur);
}



Command Agent::Work() {
  return Command::make_wait(bot_id);
}

Command Agent::Fusion(std::vector<Agent>& agents) {
  return Command::make_wait(bot_id);
}

Point LargeUdonAI::GetPosition(int bot_id) {
  return ce->GetBotStatus()[bot_id].pos;
}

bool LargeUdonAI::AgentValid(int bot_id) {
  return ce->GetBotStatus()[bot_id].active;
}

bool LargeUdonAI::InSpace(Point p) {
  return (0 <= p.x && p.x < R &&
          0 <= p.y && p.y < R &&
          0 <= p.z && p.z < R);
}

void LargeUdonAI::RegisterAgents() {
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
  const Point s_pos[9] = {
    Point(-1,-1,-1), // dummy for 1-indexed
    Point(0, 0, 0), // 1
    Point(32, 0, 0), // 2
    Point(32, 30, 0), // 3
    Point(32, 30, 32), // 4
    Point(32,  0, 32), // 5
    Point(0, 30, 0), // 6
    Point(0, 30, 32), // 7
    Point(0, 0, 32), // 8
  };

  int len = std::min(R-1, 30);

  for (size_t i = 1; i <= 8; i++) {
    Agent agent;
    agent.udonAI = this;
    agent.bot_id = i;
    agent.fission_pos = s_pos[i];
    agent.start_pos = s_pos[i];

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
  agents[4 - 1].area_min = Point(len/2 + 1, 0, len/2 +1);
  agents[4 - 1].area_max = Point(len-1, len-1, len-1);

  agents[3 - 1].area_min = Point(len/2 + 1, 0, 0);
  agents[3 - 1].area_max = Point( len-1, len-1, len/2);

  agents[6 - 1].area_min = Point(0, 0, 0);
  agents[6 - 1].area_max = Point(len/2, len-1, len/2);

  agents[7 - 1].area_min = Point(0, 0, len/2 + 1);
  agents[7 - 1].area_max = Point(len/2, len-1, len - 1);


  agents[5 - 1].area_min = Point(len/2 + 1, 0, len/2 +1);
  agents[5 - 1].area_max = Point(len-1, len-1, len-1);

  agents[2 - 1].area_min = Point(len/2 + 1, 0, 0);
  agents[2 - 1].area_max = Point( len-1, len-1, len/2);

  agents[1 - 1].area_min = Point(0, 0, 0);
  agents[1 - 1].area_max = Point(len/2, len-1, len/2);

  agents[8 - 1].area_min = Point(0, 0, len/2 + 1);
  agents[8 - 1].area_max = Point(len/2, len-1, len - 1);

}

void LargeUdonAI::AgentsGo() {
  bool done = false;
  while (!done) {
    done = true;
    std::vector<Command> coms;
    for (auto& agent : agents) {

      if (!AgentValid(agent.bot_id)) {
        //std::cerr << "Bot " << agent.bot_id << ": ___" << std::endl;
        continue;
      }
      auto com = agent.Go(agents);
      done &= agent.GoFinished();

      //std::cerr << "Bot " << agent.bot_id << ": " << agent.GetCurPos() << std::endl;

      coms.push_back(com);
    }

    //std::cerr << Command::CommandsToJson(coms);std::cerr << Command::CommandsToJson(coms);
    ce->Execute(coms);
  }
}

void LargeUdonAI::DoGFill() {
  std::vector<Point> nds(num_bots+1);
  std::vector<Point> nd_pos(num_bots+1);
  const auto& bot_status = ce->GetBotStatus();

  Point origin = bot_status[1].pos;;
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

  for(int i = 1; i <= 8; i++) {
    std::cerr << i << " " << nds[i] << " "<< nd_pos[i]<< std::endl;
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

void LargeUdonAI::AgentsWork() {
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


void LargeUdonAI::AgentsFusion() {
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

void LargeUdonAI::AgentsLastLayer() {
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

void LargeUdonAI::Interleave(std::vector<std::vector<Command>> coms) {
  std::cerr << "Inter" << std::endl;
  bool done = false;
  int cnt = 0;
  while (!done) {
    done = true;
    std::vector<Command> commands;
    for (auto& agent : agents) {
      if (!AgentValid(agent.bot_id)) continue;

      std::cerr << agent.bot_id << " is active" << std::endl;
      if (coms[agent.bot_id - 1].size() <= cnt) {
        commands.push_back(Command::make_wait(agent.bot_id));
      } else {
        done = false;
        auto com = coms[agent.bot_id-1][cnt];
        commands.push_back(com);
      }
    }
    cnt++;
    //std::cerr << Command::CommandsToJson(commands);
    ce->Execute(commands);
  }
  std::cerr << "Interend" << std::endl;
}


void LargeUdonAI::AgentsGoHome() {
  CHECK(GetPosition(0) == Point(0, 0, 0));

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


void LargeUdonAI::DoAllMove(Point p) {
  CHECK((p.x != 0 && p.y == 0 && p.z == 0) ||
        (p.x == 0 && p.y != 0 && p.z == 0) ||
        (p.x == 0 && p.y == 0 && p.z != 0));

  int l = MLen(p);
  Point r = sign_vec(p);

  while (l > 0) {
    int dl = std::min(l, 15);
    Point dvec = r * dl;

    std::vector<Command> coms;
    for (int i = 1; i <= 8; i++) {
      coms.push_back(Command::make_smove(i, dvec));
    }
    ce->Execute(coms);
    l -= dl;
  }
}
bool LargeUdonAI::OnBoarder(Point p) {
  return std::min({p.x, p.y, p.z}) == 0 ||
    std::max({p.x, p.y, p.z}) == R-1;
}
void LargeUdonAI::DoPartialMoveForce(Point p, std::set<int> ids) {
  CHECK((p.x != 0 && p.y == 0 && p.z == 0) ||
        (p.x == 0 && p.y != 0 && p.z == 0) ||
        (p.x == 0 && p.y == 0 && p.z != 0));

  int len = CLen(p);
  Point vec = sign_vec(p);

  while(len > 0) {
    int l = std::min(len, 28);
    DoAllMoveForceAux(vec * l, ids);
    len -= l;
  }
}
void LargeUdonAI::DoAllMoveForce(Point p) {
  std::set<int> ids = {1,2,3,4,5,6,7,8};
  DoPartialMoveForce(p, ids);
}



void LargeUdonAI::DoAllMoveForceAux(Point p, std::set<int> ids) {
  static std::queue<bool> fill_q[10];
  static bool init = true;
  if (init) {
    for(auto & q : fill_q) {
      q.push(false);
    }
    init = false;
  }

  std::cerr << "Force" << std::endl;
  Point dvec = sign_vec(p);
  constexpr int num_workers = 8;
  Point pos[num_workers];
  Point goals[num_workers];
  std::vector<std::vector<Command>> coms(num_workers);
  for (auto & agent : agents) {
          //      if(gfill_cnt > 2) return;
    pos[agent.bot_id - 1] = agent.GetCurPos();
    goals[agent.bot_id - 1] = agent.GetCurPos() + p;
  }

  for (auto & agent : agents) {
    if(!AgentValid(agent.bot_id)) continue;

    bool in_ids = ids.find(agent.bot_id) != ids.end();
    if (!in_ids)
      continue;

    while(pos[agent.bot_id - 1] != goals[agent.bot_id - 1]) {

      Point cur = pos[agent.bot_id - 1];
      Point nex = cur + dvec;

      CHECK(InSpace(nex)) << p << " " << cur << " " << nex;

      if (ce->GetSystemStatus().matrix[nex.x][nex.y][nex.z] == VOID) {
        coms[agent.bot_id - 1].push_back(Command::make_smove(agent.bot_id, dvec));
        pos[agent.bot_id - 1] += dvec;
        fill_q[agent.bot_id - 1].push(false);
      } else {
        coms[agent.bot_id - 1].push_back(Command::make_void(agent.bot_id, dvec));
        coms[agent.bot_id - 1].push_back(Command::make_smove(agent.bot_id, dvec));
        fill_q[agent.bot_id - 1].push(true);
        pos[agent.bot_id - 1] += dvec;
      }
      bool flg = fill_q[agent.bot_id - 1].front();
      fill_q[agent.bot_id - 1].pop();
      if(flg) {
        coms[agent.bot_id - 1].push_back(Command::make_fill(agent.bot_id, Point(0,0,0)-dvec));
      }

    }

    //coms[agent.bot_id  -1] = MergeSMove(coms[agent.bot_id  -1]);
  }

  int x = 1;
  for(auto comvec : coms) {
    std::cerr <<x++ << "  " << Command::CommandsToJson(comvec);
  }

  Interleave(coms);
  std::cerr << "Force end" << std::endl;

}

std::vector<Command> CreateMoveLine(int id, Point p) {
  Point dvec = sign_vec(p);
  std::vector<Command> coms;
  for (int i = 0; i < CLen(p); i++) {
    coms.push_back(Command::make_smove(id, dvec));
  }
  return MergeSMove(coms);
}

void LargeUdonAI::Run() {

  RegisterAgents();
  AgentsGo();

  // Gfill
  int gfill_cnt = 0;
  int dy = 0;
  for (int y = 0; y * MAX_BLOCK_SIZE < R; y++) {
    DoGFill();
    gfill_cnt++;
    std::cerr<<"Counter ------" <<gfill_cnt<<std::endl;
    int dz = 0;
    for (int z = 0; z * MAX_BLOCK_SIZE < R; z++) {
      int dx = 0;
      for (int x = 0; x * MAX_BLOCK_SIZE < R; x++) {
        int xlen = std::min((R - 1) - (32 + x * MAX_BLOCK_SIZE), MAX_BLOCK_SIZE);
        if(xlen < 0) break;
        DoAllMoveForce(Point(xlen, 0, 0));
        DoGFill();
        gfill_cnt++;
        std::cerr<<"Counter ------" <<gfill_cnt<<std::endl;
        dx += xlen;
      }
      DoAllMoveForce(Point(-dx, 0, 0));
      std::cerr << "Force" << std::endl;
      for(auto& agent : agents) {
        std::cerr << "Bot " << agent.bot_id << " " << agent.GetCurPos() <<std::endl;
      }
      int zlen = std::min((R - 1) - (32 + z * MAX_BLOCK_SIZE), MAX_BLOCK_SIZE);
      if(zlen < 0) break;
      DoAllMoveForce(Point(0, 0, zlen));
      DoGFill();
      dz += zlen;
    }
    DoAllMoveForce(Point(0, 0, -dz));

    int ylen = std::min((R - 2) - (30 + y * MAX_BLOCK_SIZE), MAX_BLOCK_SIZE);
    if(ylen < 0) break;
    DoAllMoveForce(Point(0, ylen, 0));
    dy += ylen;
  }

  // Fusion
  std::set<int> uppers = {3, 4, 6, 7};
  DoPartialMoveForce(Point(0, 1, 0), uppers);

  std::set<int> unders = {1, 2, 8, 5};
  DoPartialMoveForce(Point(0, 30, 0), unders);

  std::vector<Command> commands;

  // Fusion
  for (auto & agent : agents) {
    if (unders.find(agent.bot_id) != unders.end()) {
      auto com = Command::make_fusion_s(agent.bot_id, Point(0, 1, 0));
      commands.push_back(com);
    } else {
      auto com = Command::make_fusion_p(agent.bot_id, Point(0, -1, 0));
      commands.push_back(com);
    }
  }
  ce->Execute(commands);


  // Fill a hole
  std::vector<Command> fill_one = {
    Command::make_wait(3),
    Command::make_fill(4, Point(0, -1, 0)),
    Command::make_wait(6),
    Command::make_wait(7),
  };
  ce->Execute(fill_one);



  // Go Work place
  std::vector<std::vector<Command>> deliver_coms(10);
  deliver_coms[3 - 1] = CreateMoveLine(3, Point(R-1 - agents[3-1].GetCurPos().x, 0, 0));
  deliver_coms[7 - 1] = CreateMoveLine(7, Point(0, 0, R-1 - agents[7-1].GetCurPos().z));

  deliver_coms[4 - 1] = CreateMoveLine(4, Point(R-1 - agents[4-1].GetCurPos().x, 0, 0));
  auto com_vec_4 = CreateMoveLine(4, Point(0, 0, R-1 - agents[4-1].GetCurPos().z));
  deliver_coms[4 - 1].insert(deliver_coms[4-1].end(), com_vec_4.begin(), com_vec_4.end());
  Interleave(deliver_coms);


  AgentsWork();

  // Fusio
  std::vector<std::vector<Command>> end_coms(10);
  end_coms[7 - 1] = CreateMoveLine(7, Point(0, 0, -(R-2)));
  end_coms[4 - 1] = CreateMoveLine(4, Point(0, 0, -(R-2)));
  Interleave(end_coms);

  std::vector<Command> fusion_coms = {
    Command::make_fusion_p(3, Point(0, 0, 1)),
    Command::make_fusion_s(4, Point(0, 0,-1)),
    Command::make_fusion_p(6, Point(0, 0, 1)),
    Command::make_fusion_s(7, Point(0, 0,-1)),
  };
  ce->Execute(fusion_coms);

  std::set<int> s_3 = {3};
  DoPartialMoveForce(Point(-(R-2), 0, 0), s_3);

  std::vector<Command> last_coms = {
    Command::make_fusion_s(3, Point(-1, 0, 0)),
    Command::make_fusion_p(6, Point( 1,0, 0)),
  };
  ce->Execute(last_coms);

  ce->Execute({Command::make_halt(6)});

}




void LargeUdonAI::OldRun() {
  //CHECK(ce->GetSystemStatus().R <= 30);
  std::cerr<<"R = "<< R <<std::endl;
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

void LargeUdonAI::RegionBfs(int color, Point& cur) {
  const Point dxz[] = {
    Point(0,0,1),
    Point(-1,0,0),
    Point(0,0,-1),
    Point(1,0,0),
  };


  for(int x = 0; x < R; x++)
    for(int z = 0; z < R; z++)
      visited[x][cur.y][z] = false;

  //std::cerr<<"cur" << cur << " " << Id2Region(color) << std::endl;

  std::queue<std::pair<Point, int>> q;
  q.push(std::make_pair(cur, 0));
  while(!q.empty()) {
    //std::cerr<<"q size" << q.size() <<  std::endl;
    auto t = q.front();
    q.pop();
    Point p = t.first;
    int d = t.second;
    if(visited[p.x][p.y][p.z]) continue;
    if(DFSRegion[p.x][p.y][p.z].first < d) continue;
    DFSRegion[p.x][p.y][p.z].first = d;
    DFSRegion[p.x][p.y][p.z].second = Id2Region(color);
    visited[p.x][p.y][p.z] = true;

    for (int i = 0; i < 4; i++) {
      auto nx = p + dxz[i];
      if (InSpace(nx) &&
          !visited[nx.x][nx.y][nx.z] &&
          model[nx.x][nx.y][nx.z] == VOID)
        q.push(std::make_pair(nx, d+1));
    }
  }
}

void LargeUdonAI::DevideRegion() {
  for (int i = 0; i < R; i++)
    for (int j = 0; j < R; j++)
      for (int k = 0; k < R; k++)
        DFSRegion[i][j][k] = std::make_pair(R*R*R, -1);

  Point start_ps[] = {
    Point(  0, 0,   0),
    Point(R-1, 0,   0),
    Point(R-1, 0, R-1),
    Point(  0, 0, R-1),
  };

  for (int y = R-1; y >= 0; y--) {
    for (int i = 0; i < 4; ++i) {
      Point cur(start_ps[i].x, y, start_ps[i].z);
      int cols[] = {6, 3, 4, 7};
      RegionBfs(cols[i],cur);
    }

    /*
    for (int i = 0; i < R; i++) {
      for (int k = 0; k < R; k++) {
        std::cerr << DFSRegion[i][y][k].second << " ";
      }
      std::cerr << std::endl;;
    }
    for (int i = 0; i < R; i++) {
      for (int k = 0; k < R; k++) {
        std::cerr << (model[i][y][k] == FULL) << " ";
      }
      std::cerr << std::endl;;
    }
    */
  }
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

  auto udon_ai = std::make_unique<LargeUdonAI>(tgt_model);
  udon_ai->Run();
  udon_ai->Finalize();
}
