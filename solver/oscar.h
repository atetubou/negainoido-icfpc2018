#ifndef INCLUDE_OSCAR_H
#define INCLUDE_OSCAR_H

#include "src/base/base.h"
#include <vector>
#include <map>
#include <set>

class Vox {
    int merge(int l, int r);

    public:
    std::vector<bool> voxels;
    std::vector<int> colors;
    std::vector<int> depth;
    std::vector<int> par;
    std::map< int, std::set<int> > edges;
    int R;
    Vox(const vvv &voxels);
    bool get(int x, int y, int z);
    void set(bool v, int x, int y, int z);
    int get_color(int x, int y, int z);
    void set_color(int v, int x, int y, int z);
    int get_color(Point &p);
    void set_color(int v, Point &p);
    int add_color();
    int get_parent(int c);
    int get_parent_color(int x, int y, int z);
    vvv convert();
};

#endif