#include "command.h"
#include "glog/logging.h"

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

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
    case VOID:
      command["command"] = "Void";
      command["dx"] = c.void_nd.x;
      command["dy"] = c.void_nd.y;
      command["dz"] = c.void_nd.z;
      break;
    case FISSION:
      command["command"] = "Fission";
      command["dx"] = c.fission_nd.x;
      command["dy"] = c.fission_nd.y;
      command["dz"] = c.fission_nd.z;
      command["m"] = c.fission_m;
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
    case GFILL:
      command["command"] = "GFill";
      command["dx1"] = c.gfill_nd.x;
      command["dy1"] = c.gfill_nd.y;
      command["dz1"] = c.gfill_nd.z;
      command["dx2"] = c.gfill_fd.x;
      command["dy2"] = c.gfill_fd.y;
      command["dz2"] = c.gfill_fd.z;
      break;
    case GVOID:
      command["command"] = "GVoid";
      command["dx1"] = c.gvoid_nd.x;
      command["dy1"] = c.gvoid_nd.y;
      command["dz1"] = c.gvoid_nd.z;
      command["dx2"] = c.gvoid_fd.x;
      command["dy2"] = c.gvoid_fd.y;
      command["dz2"] = c.gvoid_fd.z;
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

Command Command::make_void(int id, Point nd) {
  Command ret;
  ret.id = id;
  ret.type = VOID;
  ret.void_nd = nd;
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

Command Command::make_gfill(int id, Point nd, Point fd) {
  Command ret;
  ret.id = id;
  ret.type = GFILL;
  ret.gfill_nd = nd;
  ret.gfill_fd = fd;
  return ret;
}

Command Command::make_gvoid(int id, Point nd, Point fd) {
  Command ret;
  ret.id = id;
  ret.type = GVOID;
  ret.gvoid_nd = nd;
  ret.gvoid_fd = fd;
  return ret;
}

static bool is_same_direction(Point l, Point r) {
  if (l.x != 0 && r.x == 0) return false;
  if (l.y != 0 && r.y == 0) return false;
  if (l.z != 0 && r.z == 0) return false;
  return true;
}

std::vector<Command> MergeSMove(absl::Span<const Command> commands) {
  std::vector<Command> ret;

  for (size_t i = 0; i < commands.size();) {
    if (commands[i].type != Command::SMOVE) {
      ret.push_back(commands[i]);
      ++i;
      continue;
    }

    size_t len = 0;
    size_t j = 0;
    for (; i + j < commands.size(); ++j) {
      if (commands[i + j].type != Command::SMOVE) {
        break;
      }
    }
    Command tmp = commands[i];
    for (size_t k = i+1; k < i+j; k++) {
      if (tmp.type == Command::SMOVE) {
        if (tmp.smove_lld.Manhattan() == 0 || is_same_direction(tmp.smove_lld, commands[k].smove_lld)) {
          tmp.smove_lld += commands[k].smove_lld;
          if (tmp.smove_lld.Manhattan() >= 15) {
            Point lld  = Point(
              sgn(tmp.smove_lld.x) * 15,
              sgn(tmp.smove_lld.y) * 15,
              sgn(tmp.smove_lld.z) * 15
            );

            tmp.smove_lld -= lld;
            ret.push_back(Command::make_smove(tmp.id, lld));
          }
        } else {
          if (tmp.smove_lld.Manhattan() <= 5 && commands[k].smove_lld.Manhattan() <= 5) {
            tmp = Command::make_lmove(tmp.id, tmp.smove_lld, commands[k].smove_lld);
          } else {
            ret.push_back(tmp);
            tmp = commands[k];
          }
        }
      // tmp.type == Command::LMOVE
      } else {
        if (is_same_direction(tmp.lmove_sld2, commands[k].smove_lld)) {
          tmp.lmove_sld2 += commands[k].smove_lld;
          if (tmp.lmove_sld2.Manhattan() >= 5) {
            Point sld2 = Point(
              sgn(tmp.lmove_sld2.x) * 5,
              sgn(tmp.lmove_sld2.y) * 5,
              sgn(tmp.lmove_sld2.z) * 5
            );
            ret.push_back(Command::make_lmove(tmp.id, tmp.lmove_sld1, sld2));
            tmp.type = Command::SMOVE;
            tmp.smove_lld = tmp.lmove_sld2 - sld2;
            tmp.lmove_sld1 = Point(0,0,0);
            tmp.lmove_sld2 = Point(0,0,0);
          }
        } else {
          ret.push_back(tmp);
          tmp = commands[k];
        }
      }
    }
    if (tmp.type != Command::SMOVE || tmp.smove_lld.Manhattan() > 0) {
      if (tmp.type == Command::LMOVE && tmp.lmove_sld2.Manhattan() == 0) {
        ret.push_back(Command::make_smove(tmp.id, tmp.lmove_sld1));
      } else {
        ret.push_back(tmp);
      }
    }

    i += j;
  }

  for (auto com : ret) {
    if (com.type == Command::LMOVE) {
      CHECK(com.lmove_sld1.Manhattan() != 0 && com.lmove_sld2.Manhattan() != 0) << com.lmove_sld1 << " " << com.lmove_sld2;
    }
  }

  return ret;
}
