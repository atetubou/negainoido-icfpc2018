#ifndef COMMAND_H
#define COMMAND_H

#include <vector>

#include "absl/types/span.h"

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
    VOID,
    FUSION_P,
    FUSION_S,
    GFILL,
    GVOID,
  };

  Type type;

  uint32_t id;

  // Smove
  Point smove_lld;
  // Lmove
  Point lmove_sld1;
  Point lmove_sld2;
  // Void
  Point void_nd;
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

  // functions to make Command structs
  static Command make_halt(int id);
  static Command make_wait(int id);
  static Command make_flip(int id);
  static Command make_smove(int id, Point lld);
  static Command make_lmove(int id, Point sld1, Point sld2);
  static Command make_fission(int id, Point nd, uint32_t m);
  static Command make_fill(int id, Point nd);
  static Command make_void(int id, Point nd);
  static Command make_fusion_p(int id, Point nd);
  static Command make_fusion_s(int id, Point nd);

  static Command make_gfill(int id, Point nd, Point fd);
  static Command make_gvoid(int id, Point nd, Point fd);
};

std::vector<Command> MergeSMove(absl::Span<const Command> commands);

#endif
