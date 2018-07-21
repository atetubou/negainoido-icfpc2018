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

Command Command::JsonToCommand(const Json::Value turn) {
    Command c = Command();
    c.type = Command::HALT;
    return c;
}
