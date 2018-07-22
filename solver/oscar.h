#ifndef INCLUDE_OSCAR_H
#define INCLUDE_OSCAR_H

#include "src/base/base.h"
#include <vector>
#include <map>

class Vox {
    int color_count;
    void add_color(int dir);
    public:
    std::vector<bool> voxels;
    std::vector<int> colors;
    std::map<int,int> max_pos;
    std::map<int,int> min_pos;
    int R;
    std::map<int,int> g2d;
    Vox(const vvv &voxels);
    bool get(int x, int y, int z);
    void set(bool v, int x, int y, int z);
    int get_color(int x, int y, int z);
    void set_color(int v, int x, int y, int z);
    int get_color(Point &p);
    void set_color(int v, Point &p);
    vvv convert();
};

#endif