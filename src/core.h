#include "src/command_util.h"


bool is_completed(vvv&N, vvv&M) {
    int R = M.size();
    for (int x=0; x<R; ++x)
        for (int y=0; y<R; ++y)
            for (int z=0; z<R; ++z)
        if (N[x][y][z] != M[x][y][z]) return false;
    return true;
}

int mlen(
        const Point& p,
        const Point& q) {
    return abs(p.x - q.x) +
        abs(p.y - q.y) +
        abs(p.z - q.z);
}

int clen(
        const Point& p,
        const Point& q) {
    return max(abs(p.x - q.x), abs(p.y - q.y), abs(p.z - q.z));
}


bool is_near(
        const Point& p,
        const Point& q) {
    return clen(p, q) == 1 and mlen(p, q) <= 2 and mlen(p, q) > 0;
}
