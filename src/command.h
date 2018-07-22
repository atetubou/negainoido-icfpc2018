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

  struct Smove {
    Point lld;
  };

  struct Lmove {
    Point sld1;
    Point sld2;
  };

  struct Void {
    Point nd;
  };

  struct Fission {
    Point nd;
    uint32_t m;
  };

  struct Fill {
    Point nd;
  };

  struct FusionP {
    Point nd;
  };

  struct FusionS {
    Point nd;
  };

  struct Gfill {
    Point nd;
    Point fd;
  };

  struct Gvoid {
    Point nd;
    Point fd;
  };


  // Members
  Type type;

  uint32_t id;

  union {
    Smove smove_;
    Lmove lmove_;
    Void void_;
    Fission fission_;
    Fill fill_;
    FusionP fusion_p_;
    FusionS fusion_s_;
    Gfill gfill_;
    Gvoid gvoid_;
  };

  Command() : type(HALT), id(0) {};

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
