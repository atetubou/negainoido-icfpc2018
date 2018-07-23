#ifndef INCLUDED_SIMPLE_SOLVE_H
#define INCLUDED_SIMPLE_SOLVE_H

#include "src/base/base.h"
#include "src/command.h"
#include <vector>

enum class MyVoxelState {
  kALWAYSEMPTY,
  kSHOULDBEFILLED,
  kALREADYFILLED,
};

using ev = std::vector<MyVoxelState>;
using evv = std::vector<ev>;
using evvv = std::vector<evv>;

std::vector<Command> get_commands_for_next(const Point& current, const Point& dest, 
                                           const evvv& voxel_states);

std::vector<Command> SimpleSolve(const vvv& voxels);
std::vector<Command> SimpleSolve(const vvv& voxels, const Point &dest, const bool halt);

#endif
