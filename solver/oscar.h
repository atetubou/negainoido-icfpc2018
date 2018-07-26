#ifndef INCLUDE_OSCAR_H
#define INCLUDE_OSCAR_H

#include "src/base/base.h"
#include <vector>
#include <map>
#include <set>
#include <queue>

struct VTarget {
    int x;
    int z;
    int ex;
    int ez;

    VTarget(int x, int z, int ex, int ez) : x(x), z(z), ex(ex), ez(ez) {};
    bool in(int x, int z);
};

struct VBot {
    enum State {
        SLEEPING,
        WAITING,
        WORKING,
        MOVING,
    };
    int id;
    Point pos;
    State state;
    std::queue<VTarget> reserved;
    std::queue<Command> command_queue;
    VBot(int id) : id(id), state(SLEEPING) {};
    bool inNextTarget(int x, int z);
};

struct VArea {
    int R;
    std::vector<int> area;
    std::set< std::pair<int,int> > release;
    VArea(int R) : R(R), area(R*R, -1), release() {};

    bool checkReserve(int id, int x, int y);
    bool reserve(int id, const VTarget &tar);
    bool reserve(int id, int x, int y);
    int get(int x, int y);
    void free(int x, int y);
    void freeArea(const VBot &vbot, const VTarget &tar);
    void runFree();
};

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