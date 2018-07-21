#ifndef COMMAND_H
#define COMMAND_H

#include <vector>

#include "command_util.h"

#include "json/json.h"

struct Command {
  enum Type {
    HALT,
    WAIT,
    FLIP,
    SMOVE,
    LMOVE,
    FISSION,
    FILL,
    FUSION_P,
    FUSION_S,
  };

  Type type;

  uint32_t id;

  // Smove
  Point smove_lld;
  // Lmove
  Point lmove_sld1;
  Point lmove_sld2;
  // FISSION
  Point fission_nd;
  uint32_t fission_m;
  // FILL
  Point fill_nd;
  // FUSION_P
  Point fusion_p_nd;
  // FUSION_S
  Point fusion_s_nd;

  static Json::Value CommandsToJson(const std::vector<Command>& commands);
  static Command JsonToCommand(const Json::Value);
  static Command make_fill(int id, Point nd);
  static Command make_halt(int id);
  static Command make_smove(int id, Point lld);
};

#endif
