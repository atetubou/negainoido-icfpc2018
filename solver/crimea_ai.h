#ifndef INCLUDED_CRIMEA_AI_H
#define INCLUDED_CRIMEA_AI_H

#include "src/base/base.h"
#include "src/command_util.h"
#include "src/command.h"
#include "src/command_executer.h"

#include "solver/AI.h"
#include "oscar.h"

using bv = std::vector<bool>;
using bvv = std::vector<bv>;
using bvvv = std::vector<bvv>;
using P = std::pair<int,int>;


#define DOWN_X 0
#define UP_X 1
#define DOWN_Y 2
#define UP_Y 3
#define DOWN_Z 4
#define UP_Z 5

class CrimeaAI : public AI {
    protected:
    int R;
    Vox tvox;
    Vox vox;

    std::vector<Command> getPath(const Point &pos, const Point &dest);

    bool remove_x(int i, bvv &grounded, int sign);   

    bool fill_y(const int j, bvv &grounded, int sign);
    
    bool remove_y(const int j, bvv &grounded, int sign);
    
    bool remove_z(const int k, bvv &grounded, int sign);


  public:
    CrimeaAI(const vvv &src_model, const vvv &tgt_model) : AI(src_model), tvox(tgt_model), vox(src_model) {
        R = src_model.size();
     }
    ~CrimeaAI() override = default;

};

#endif



