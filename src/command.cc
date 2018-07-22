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
      command["dx"] = c.smove_.lld.x;
      command["dy"] = c.smove_.lld.y;
      command["dz"] = c.smove_.lld.z;
      break;
    case LMOVE:
      command["command"] = "LMove";
      command["dx1"] = c.lmove_.sld1.x;
      command["dy1"] = c.lmove_.sld1.y;
      command["dz1"] = c.lmove_.sld1.z;
      command["dx2"] = c.lmove_.sld2.x;
      command["dy2"] = c.lmove_.sld2.y;
      command["dz2"] = c.lmove_.sld2.z;
      break;
    case VOID:
      command["command"] = "Void";
      command["dx"] = c.void_.nd.x;
      command["dy"] = c.void_.nd.y;
      command["dz"] = c.void_.nd.z;
      break;
    case FISSION:
      command["command"] = "Fission";
      command["dx"] = c.fission_.nd.x;
      command["dy"] = c.fission_.nd.y;
      command["dz"] = c.fission_.nd.z;
      command["m"] = c.fission_.m;
      break;
    case FILL:
      command["command"] = "Fill";
      command["dx"] = c.fill_.nd.x;
      command["dy"] = c.fill_.nd.y;
      command["dz"] = c.fill_.nd.z;
      break;
    case FUSION_P:
      command["command"] = "FusionP";
      command["dx"] = c.fusion_p_.nd.x;
      command["dy"] = c.fusion_p_.nd.y;
      command["dz"] = c.fusion_p_.nd.z;
      break;
    case FUSION_S:
      command["command"] = "FusionS";
      command["dx"] = c.fusion_s_.nd.x;
      command["dy"] = c.fusion_s_.nd.y;
      command["dz"] = c.fusion_s_.nd.z;
      break;
    case GFILL:
      command["command"] = "GFill";
      command["dx1"] = c.gfill_.nd.x;
      command["dy1"] = c.gfill_.nd.y;
      command["dz1"] = c.gfill_.nd.z;
      command["dx2"] = c.gfill_.fd.x;
      command["dy2"] = c.gfill_.fd.y;
      command["dz2"] = c.gfill_.fd.z;
      break;
    case GVOID:
      command["command"] = "GVoid";
      command["dx1"] = c.gvoid_.nd.x;
      command["dy1"] = c.gvoid_.nd.y;
      command["dz1"] = c.gvoid_.nd.z;
      command["dx2"] = c.gvoid_.fd.x;
      command["dy2"] = c.gvoid_.fd.y;
      command["dz2"] = c.gvoid_.fd.z;
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
        c.smove_.lld.x = command["dx"].asInt();
        c.smove_.lld.y = command["dy"].asInt();
        c.smove_.lld.z = command["dz"].asInt();
    } else if (type == "LMove") {
        c.type = LMOVE;
        c.lmove_.sld1.x = command["dx1"].asInt();
        c.lmove_.sld1.y = command["dy1"].asInt();
        c.lmove_.sld1.z = command["dz1"].asInt();
        c.lmove_.sld2.x = command["dx2"].asInt();
        c.lmove_.sld2.y = command["dy2"].asInt();
        c.lmove_.sld2.z = command["dz2"].asInt();
    } else if (type == "Fission") {
        c.type = FISSION;
        c.fission_.nd.x = command["dx"].asInt();
        c.fission_.nd.y = command["dy"].asInt();
        c.fission_.nd.z = command["dz"].asInt();
    } else if (type == "Fill") {
        c.type = FILL;
        c.fill_.nd.x = command["dx"].asInt();
        c.fill_.nd.y = command["dy"].asInt();
        c.fill_.nd.z = command["dz"].asInt();
    } else if (type == "FusionP") {
        c.type = FUSION_P;
        c.fusion_p_.nd.x = command["dx"].asInt();
        c.fusion_p_.nd.y = command["dy"].asInt();
        c.fusion_p_.nd.z = command["dz"].asInt();
    } else if (type == "FusionS") {
        c.type = FUSION_S;
        c.fusion_s_.nd.x = command["dx"].asInt();
        c.fusion_s_.nd.y = command["dy"].asInt();
        c.fusion_s_.nd.z = command["dz"].asInt();
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
  ret.smove_.lld = lld;
  return ret;
}

Command Command::make_lmove(int id, Point sld1, Point sld2) {
  Command ret;
  ret.id = id;
  ret.type = LMOVE;
  ret.lmove_.sld1 = sld1;
  ret.lmove_.sld2 = sld2;
  return ret;
}

Command Command::make_fission(int id, Point nd, uint32_t m) {
  Command ret;
  ret.id = id;
  ret.type = FISSION;
  ret.fission_.nd = nd;
  ret.fission_.m = m;
  return ret;
}

Command Command::make_fill(int id, Point nd) {
  Command ret;
  ret.id = id;
  ret.type = FILL;
  ret.fill_.nd = nd;
  return ret;
}

Command Command::make_void(int id, Point nd) {
  Command ret;
  ret.id = id;
  ret.type = VOID;
  ret.void_.nd = nd;
  return ret;
}

Command Command::make_fusion_p(int id, Point nd) {
  Command ret;
  ret.id = id;
  ret.type = FUSION_P;
  ret.fusion_p_.nd = nd;
  return ret;
}

Command Command::make_fusion_s(int id, Point nd) {
  Command ret;
  ret.id = id;
  ret.type = FUSION_S;
  ret.fusion_s_.nd = nd;
  return ret;
}

Command Command::make_gfill(int id, Point nd, Point fd) {
  Command ret;
  ret.id = id;
  ret.type = GFILL;
  ret.gfill_.nd = nd;
  ret.gfill_.fd = fd;
  return ret;
}

Command Command::make_gvoid(int id, Point nd, Point fd) {
  Command ret;
  ret.id = id;
  ret.type = GVOID;
  ret.gvoid_.nd = nd;
  ret.gvoid_.fd = fd;
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

    size_t j = 0;
    for (; i + j < commands.size(); ++j) {
      if (commands[i + j].type != Command::SMOVE) {
        break;
      }
    }
    Command tmp = commands[i];
    for (size_t k = i+1; k < i+j; k++) {
      if (tmp.type == Command::SMOVE) {
        if (tmp.smove_.lld.Manhattan() == 0 || is_same_direction(tmp.smove_.lld, commands[k].smove_.lld)) {
          tmp.smove_.lld += commands[k].smove_.lld;
          if (tmp.smove_.lld.Manhattan() >= 15) {
            Point lld  = Point(
              sgn(tmp.smove_.lld.x) * 15,
              sgn(tmp.smove_.lld.y) * 15,
              sgn(tmp.smove_.lld.z) * 15
            );

            tmp.smove_.lld -= lld;
            ret.push_back(Command::make_smove(tmp.id, lld));
          }
        } else {
          if (tmp.smove_.lld.Manhattan() <= 5 && commands[k].smove_.lld.Manhattan() <= 5) {
            tmp = Command::make_lmove(tmp.id, tmp.smove_.lld, commands[k].smove_.lld);
          } else {
            ret.push_back(tmp);
            tmp = commands[k];
          }
        }
      // tmp.type == Command::LMOVE
      } else {
        if (is_same_direction(tmp.lmove_.sld2, commands[k].smove_.lld)) {
          tmp.lmove_.sld2 += commands[k].smove_.lld;
          if (tmp.lmove_.sld2.Manhattan() >= 5) {
            Point sld2 = Point(
              sgn(tmp.lmove_.sld2.x) * 5,
              sgn(tmp.lmove_.sld2.y) * 5,
              sgn(tmp.lmove_.sld2.z) * 5
            );
            ret.push_back(Command::make_lmove(tmp.id, tmp.lmove_.sld1, sld2));
            tmp.type = Command::SMOVE;
            tmp.smove_.lld = tmp.lmove_.sld2 - sld2;
            tmp.lmove_.sld1 = Point(0,0,0);
            tmp.lmove_.sld2 = Point(0,0,0);
          }
        } else {
          ret.push_back(tmp);
          tmp = commands[k];
        }
      }
    }
    if (tmp.type != Command::SMOVE || tmp.smove_.lld.Manhattan() > 0) {
      if (tmp.type == Command::LMOVE && tmp.lmove_.sld2.Manhattan() == 0) {
        ret.push_back(Command::make_smove(tmp.id, tmp.lmove_.sld1));
      } else {
        ret.push_back(tmp);
      }
    }

    i += j;
  }

  for (auto com : ret) {
    if (com.type == Command::LMOVE) {
      CHECK(com.lmove_.sld1.Manhattan() != 0 && com.lmove_.sld2.Manhattan() != 0) << com.lmove_.sld1 << " " << com.lmove_.sld2;
    }
  }

  return ret;
}
