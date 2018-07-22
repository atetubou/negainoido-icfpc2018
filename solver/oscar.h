#ifndef INCLUDE_OSCAR_H
#define INCLUDE_OSCAR_H

#include "src/base/base.h"
#include <vector>
#include <map>

class Vox {
    std::vector<bool> voxels;
    std::vector<int> colors;
    int R;
    std::map<int,int> g2d;
    int color_count;
    public:
    Vox(const vvv &voxels);
    bool get(int x, int y, int z);
    int get_color(int x, int y, int z);
    void set_color(int v, int x, int y, int z);
    int get_color(Point &p);
    void set_color(int v, Point &p);
    vvv convert();
    void dfs(int color, int x, int y, int z, int dir, int base);
    void set_colors();
};

#endif