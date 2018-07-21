#include "command.h"

// static
Json::Value Command::CommandsToJson(const std::vector<Command>& commands) {
  Json::Value turn;
  for (const auto& c : commands) {
    Json::Value command;
    command["bot_id"] = c.id;
    switch(c.type) {
    case HALT:
      command["command"] = "Halt";
      break;
    case WAIT:
      command["command"] = "Wait";
      break;
    case FLIP:
      command["command"] = "Flip";
      break;
    case SMOVE:
      command["command"] = "SMove";
      command["dx"] = c.smove_lld.x;
      command["dy"] = c.smove_lld.y;
      command["dz"] = c.smove_lld.z;
      break;
    case LMOVE:
      command["command"] = "LMove";
      command["dx1"] = c.lmove_sld1.x;
      command["dy1"] = c.lmove_sld1.y;
      command["dz1"] = c.lmove_sld1.z;
      command["dx2"] = c.lmove_sld2.x;
      command["dy2"] = c.lmove_sld2.y;
      command["dz2"] = c.lmove_sld2.z;
      break;
    case FISSION:
      command["command"] = "Fission";
      command["dx"] = c.fission_nd.x;
      command["dy"] = c.fission_nd.y;
      command["dz"] = c.fission_nd.z;
      break;
    case FILL:
      command["command"] = "Fill";
      command["dx"] = c.fill_nd.x;
      command["dy"] = c.fill_nd.y;
      command["dz"] = c.fill_nd.z;
      break;
    case FUSION_P:
      command["command"] = "FusionP";
      command["dx"] = c.fusion_p_nd.x;
      command["dy"] = c.fusion_p_nd.y;
      command["dz"] = c.fusion_p_nd.z;
      break;
    case FUSION_S:
      command["command"] = "FusionS";
      command["dx"] = c.fusion_s_nd.x;
      command["dy"] = c.fusion_s_nd.y;
      command["dz"] = c.fusion_s_nd.z;
      break;
    }
    turn.append(std::move(command));
  }
  return turn;
}

Command Command::JsonToCommand(const Json::Value command) {

    Command c = Command();
    std::string type = command["command"].asString();

    c.id = command["bot_id"].asInt();

    c.type = Command::HALT;
    if (type == "Halt") {
        c.type = Command::HALT;
    } else if (type == "Wait") {
        c.type = Command::WAIT;
    } else if (type == "Flip") {
        c.type = Command::FLIP;
    } else if (type == "SMove") {
        c.type = SMOVE;
        c.smove_lld.x = command["dx"].asInt();
        c.smove_lld.y = command["dy"].asInt();
        c.smove_lld.z = command["dz"].asInt();
    } else if (type == "LMove") {
        c.type = LMOVE;
        c.lmove_sld1.x = command["dx1"].asInt();
        c.lmove_sld1.y = command["dy1"].asInt();
        c.lmove_sld1.z = command["dz1"].asInt();
        c.lmove_sld2.x = command["dx2"].asInt();
        c.lmove_sld2.y = command["dy2"].asInt();
        c.lmove_sld2.z = command["dz2"].asInt();
    } else if (type == "Fission") {
        c.type = FISSION;
        c.fission_nd.x = command["dx"].asInt();
        c.fission_nd.y = command["dy"].asInt();
        c.fission_nd.z = command["dz"].asInt();
    } else if (type == "Fill") {
        c.type = FILL;
        c.fill_nd.x = command["dx"].asInt();
        c.fill_nd.y = command["dy"].asInt();
        c.fill_nd.z = command["dz"].asInt();
    } else if (type == "FusionP") {
        c.type = FUSION_P;
        c.fusion_p_nd.x = command["dx"].asInt();
        c.fusion_p_nd.y = command["dy"].asInt();
        c.fusion_p_nd.z = command["dz"].asInt();
    } else if (type == "FusionS") {
        c.type = FUSION_S;
        c.fusion_s_nd.x = command["dx"].asInt();
        c.fusion_s_nd.y = command["dy"].asInt();
        c.fusion_s_nd.z = command["dz"].asInt();
    }
    return c;
}


Command Command::make_halt(int id) {
  Command ret;
  ret.id = id;
  ret.type = HALT;
  return ret;
}

Command Command::make_wait(int id) {
  Command ret;
  ret.id = id;
  ret.type = WAIT;
  return ret;
}

Command Command::make_flip(int id) {
  Command ret;
  ret.id = id;
  ret.type = FLIP;
  return ret;
}

Command Command::make_smove(int id, Point lld) {
  Command ret;
  ret.id = id;
  ret.type = SMOVE;
  ret.smove_lld = lld;
  return ret;
}

Command Command::make_lmove(int id, Point sld1, Point sld2) {
  Command ret;
  ret.id = id;
  ret.type = LMOVE;
  ret.lmove_sld1 = sld1;
  ret.lmove_sld2 = sld2;
  return ret;
}

Command Command::make_fission(int id, Point nd, uint32_t m) {
  Command ret;
  ret.id = id;
  ret.type = FISSION;
  ret.fission_nd = nd;
  ret.fission_m = m;
  return ret;
}

Command Command::make_fill(int id, Point nd) {
  Command ret;
  ret.id = id;
  ret.type = FILL;
  ret.fill_nd = nd;
  return ret;
}

Command Command::make_fusion_p(int id, Point nd) {
  Command ret;
  ret.id = id;
  ret.type = FUSION_P;
  ret.fusion_p_nd = nd;
  return ret;
}

Command Command::make_fusion_s(int id, Point nd) {
  Command ret;
  ret.id = id;
  ret.type = FUSION_S;
  ret.fusion_s_nd = nd;
  return ret;
}

std::vector<Command> MergeSMove(absl::Span<const Command> commands) {
  std::vector<Command> ret;

  for (size_t i = 0; i < commands.size();) {
    if (commands[i].type != Command::SMOVE ||
        commands[i].smove_lld.Manhattan() != 1) {
      ret.push_back(commands[i]);
      ++i;
      continue;
    }
    
    size_t j = 0;
    for (; i + j < commands.size() && j < 15; ++j) {
      if (commands[i + j].type != Command::SMOVE) {
        break;
      }
      if (commands[i].smove_lld != commands[i + j].smove_lld) {
        break;
      }
    }

    
    Command c = commands[i];
    c.smove_lld.x *= j;
    c.smove_lld.y *= j;
    c.smove_lld.z *= j;

    ret.push_back(c);
    i += j;
  }

  return ret;
}
