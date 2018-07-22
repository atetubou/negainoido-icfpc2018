#ifndef SOLVER_SQUARE_DELETER_H_
#define SOLVER_SQUARE_DELETER_H_

#include <vector>

#include "json/value.h"

#include "src/base/base.h"

std::vector<std::pair<int, int>> Getdxzs(int R, int n);

bool IsAllEmpty(int y, int lx, int lz, int hx, int hz, const vvv& voxels);

Json::Value SquareDelete(const vvv& voxels, bool use_flip);

#endif  // SOLVER_SQUARE_DELETER_H_
