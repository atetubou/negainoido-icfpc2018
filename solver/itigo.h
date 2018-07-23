#ifndef ITIGO_H
#define ITIGO_H

#include "solver/AI.h"

class Itigo : public AI {
  const vvv model;
  const int R;
  const bool use_flip;

  std::vector<int> bids;

  Point low, high;
  int Rx = 0;
  int Ry = 0;
  int Rz = 0;
  int given_x_num = 0;
  int given_z_num = 0;
public:
  Itigo(const vvv &model, bool use_flip,
        int given_x_num, int given_z_num);

  ~Itigo() override = default;

  void ExecuteOrWait(const std::vector<Command> commands);

  void FissionAndGo(int bid, const Point& goal, int m);

  void DebugSeed() const;

  void DoHaiti(int x_num, int z_num);

  int DoMove(const std::vector<std::pair<int, Point>>& targets);

  void CalcRegion();

  void Run() override;
};

#endif
