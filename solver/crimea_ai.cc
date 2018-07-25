#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <deque>
#include <queue>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "src/command_util.h"
#include "src/command.h"
#include "src/command_executer.h"

#include "solver/crimea_ai.h"


using namespace std;
static int dx[] = {-1, 1, 0, 0, 0, 0};
static int dy[] = {0, 0, -1, 1, 0, 0};
static int dz[] = {0, 0, 0, 0, -1, 1};

static Point dP[] = {
    Point(-1,0,0),
    Point(1,0,0),
    Point(0,-1,0),
    Point(0,1,0),
    Point(0,0,-1),
    Point(0,0,1)
};

vector<Command> CrimeaAI::getPath(const Point &pos, const Point &dest) {
    return getPath(pos, dest, set<Point>());
}

vector<Command> CrimeaAI::getPath(const Point &pos, const Point &dest, const set<Point> invalid) {
    DVLOG(2) << "finding " << pos << " " << dest;
    if (pos==dest) return {};
    queue<Point> que;
    que.push(pos);
    vvv hist = vvv(R,vv(R, v(R,-1)));
    int count = 0;
    while(!que.empty()) {
        Point cur = que.front();
        que.pop();

        if (hist[cur.x][cur.y][cur.z]!= -1) continue;
        hist[cur.x][cur.y][cur.z] = count++;

        if (cur==dest) break;

        for (int d=0;d<6;d++) {
            int nx = cur.x + dx[d];
            int ny = cur.y + dy[d];
            int nz = cur.z + dz[d];
            
            if(nx>=0 && nx < R && ny >= 0 && ny < R && nz>= 0 && nz < R && invalid.find(Point(nx,ny,nz)) == invalid.end()) {
                if(ce->GetSystemStatus().matrix[nx][ny][nz]==VoxelState::VOID) {
                    que.push(Point(nx,ny,nz));
                }
            }
        }
    }
    CHECK(hist[dest.x][dest.y][dest.z] != -1) << "Failed to find path";
    vector<Command> rev_ret;
    rev_ret.reserve(hist[dest.x][dest.y][dest.z]);
    Point cur = dest;
    while(cur!=pos) {
        int mind = -1;
        int minh = R*R*R;
        for(int d=0;d<6;d++) {
            int nx = cur.x + dx[d];
            int ny = cur.y + dy[d];
            int nz = cur.z + dz[d];
            
            if(nx>=0 && nx < R && ny >= 0 && ny < R && nz>= 0 && nz < R) {
                if(hist[nx][ny][nz]!=-1 && minh > hist[nx][ny][nz]) {
                    minh = hist[nx][ny][nz];
                    mind = d;
                }
            }
        }
        CHECK(mind!=-1) << "not found";
        rev_ret.push_back(Command::make_smove(1, Point(-dx[mind], -dy[mind], -dz[mind])));
        cur += Point(dx[mind],dy[mind],dz[mind]);
    }

    reverse(rev_ret.begin(), rev_ret.end());
    return MergeSMove(rev_ret);
}

bool CrimeaAI::remove_x(int i, bvv &grounded, int sign) {
    bool res = false;
    int fd = min(R, 5);

    for (int j=0;j<R-fd;j++) for (int k=0;k<R-fd;k++) if(grounded[j][k]) {
        int p=0; int q=0;
        while(p < fd && grounded[j+p][k]) p++;
        while(q < fd && grounded[j][k+q]) q++;
        if (p==1 || q == 1) continue;
        bool flag = true;
        for (int s=0;s<p;s++) for (int t=0;t<q;t++) if(!grounded[j+s][k+t] && vox.get(i,j+s,k+t)) {
            flag = false;
            break;
        }
        if (flag && (p> 2 || q > 2)) {
            p--; q--;
            DLOG(INFO) << "try s delete x " << p << " " << q;
            Point cur = ce->GetBotStatus()[1].pos;
            for (auto c : getPath(cur, Point(i+sign, j,k))) {
                ce->Execute({c});
            }
            DLOG(INFO) << "try fisision";
            ce->Execute({Command::make_fission(1, dP[UP_Y], 1)});
            ce->Execute({Command::make_fission(1, dP[UP_Z], 0), Command::make_fission(2, dP[UP_Z], 0)});
            vector<Command> moving;
            DLOG(INFO) << "try moving";
            moving.push_back(Command::make_wait(1));
            if (p==1) {
                moving.push_back(Command::make_wait(2));
                moving.push_back(Command::make_smove(3, dP[UP_Z] * (q-1)));
                moving.push_back(Command::make_smove(4, dP[UP_Z] * (q-1)));
            } else if (q==1) {
                moving.push_back(Command::make_smove(2, dP[UP_Y] * (p-1)));
                moving.push_back(Command::make_smove(3, dP[UP_Y] * (p-1)));
                moving.push_back(Command::make_wait(4));
            } else {
                moving.push_back(Command::make_smove(2, dP[UP_Y] * (p-1)));
                moving.push_back(Command::make_lmove(3, dP[UP_Y] * (p-1), dP[UP_Z] * (q-1)));
                moving.push_back(Command::make_smove(4, dP[UP_Z] * (q-1)));
            }
            ce->Execute(moving);
            ce->Execute({
                Command::make_gvoid(1, dP[UP_X] * -sign,  Point(0,p,q)),
                Command::make_gvoid(2, dP[UP_X] * -sign,  Point(0,-p,q)),
                Command::make_gvoid(3, dP[UP_X] * -sign,  Point(0,-p,-q)),
                Command::make_gvoid(4, dP[UP_X] * -sign,  Point(0,p,-q)),

            });
            for (int s=0;s<p+1;s++) for (int t=0;t<q+1;t++) {
                grounded[j+s][k+t] = false;
                vox.set(false, i, j+s,k+t);
            }
            
            DLOG(INFO) << "try moving";
            moving.clear();
            moving.push_back(Command::make_wait(1));
            if (p==1) {
                moving.push_back(Command::make_wait(2));
                moving.push_back(Command::make_smove(3, Point(0,0,-q+1)));
                moving.push_back(Command::make_smove(4, Point(0,0,-q+1)));
            } else if (q==1) {
                moving.push_back(Command::make_smove(2, Point(0,-p+1,0)));
                moving.push_back(Command::make_smove(3, Point(0,-p+1,0)));
                moving.push_back(Command::make_wait(4));
            } else {
                moving.push_back(Command::make_smove(2, Point(0,-p+1,0)));
                moving.push_back(Command::make_lmove(3, Point(0,-p+1,0), Point(0,0,-q+1)));
                moving.push_back(Command::make_smove(4, Point(0,0,-q+1)));
            }
            ce->Execute(moving);
            ce->Execute({
                Command::make_fusion_p(1, dP[UP_Z]),
                Command::make_fusion_p(2, dP[UP_Z]),
                Command::make_fusion_s(3, dP[UP_Z] * -1),
                Command::make_fusion_s(4, dP[UP_Z] * -1),
            });
            ce->Execute({
                Command::make_fusion_p(1, dP[UP_Y]),
                Command::make_fusion_s(2, dP[UP_Y] * -1),
            });
            q++;
            res = true;
        }

        k += q;
    }


    for (int j=0;j<R;j++) for (int k=0;k<R;k++) if(grounded[j][k]){
        res = true;
        Point cur = ce->GetBotStatus()[1].pos;
        for (auto c : getPath(cur, Point(i+sign, j,k))) {
            ce->Execute({c});
        }
        ce->Execute({Command::make_void(1, Point(-sign,0,0))});
        vox.set(false, i,j,k);
    }
    return res;
}
    
bool CrimeaAI::fill_y(const int j, bvv &grounded, int sign) {
    bool res = false;
    int fd = min(R, 5);
    for (int i=0;i<R-fd;i++) for (int k=0;k<R-fd;k++) if(grounded[i][k]) {
        int p=0; int q=0;
        while(p < fd && grounded[i+p][k]) p++;
        while(q < fd && grounded[i][k+q]) q++;
        if (p==1 || q == 1) continue;
        bool flag = true;
        for (int s=0;s<p;s++) for (int t=0;t<q;t++) if(!tvox.get(i+s,j,k+t)) {
            flag = false;
            break;
        }
        if (flag && (p> 2 || q > 2)) {
            DLOG(INFO) << "try s fill y " << p << " " << q;
            p--; q--;
            Point cur = ce->GetBotStatus()[1].pos;
            for (auto c : getPath(cur, Point(i, j + sign,k))) {
                ce->Execute({c});
            }
            DLOG(INFO) << "try fisision";
            ce->Execute({Command::make_fission(1, dP[UP_X], 1)});
            ce->Execute({Command::make_fission(1, dP[UP_Z], 0), Command::make_fission(2, dP[UP_Z], 0)});
            vector<Command> moving;
            DLOG(INFO) << "try moving";
            moving.push_back(Command::make_wait(1));
            if (p==1) {
                moving.push_back(Command::make_wait(2));
                moving.push_back(Command::make_smove(3, dP[UP_Z] * (q-1)));
                moving.push_back(Command::make_smove(4, dP[UP_Z] * (q-1)));
            } else if (q==1) {
                moving.push_back(Command::make_smove(2, dP[UP_X] * (p-1)));
                moving.push_back(Command::make_smove(3, dP[UP_X] * (p-1)));
                moving.push_back(Command::make_wait(4));
            } else {
                moving.push_back(Command::make_smove(2, dP[UP_X] * (p-1)));
                moving.push_back(Command::make_lmove(3, dP[UP_X] * (p-1), dP[UP_Z] * (q-1)));
                moving.push_back(Command::make_smove(4, dP[UP_Z] * (q-1)));
            }
            ce->Execute(moving);
            DLOG(INFO) << "try gfill";
            ce->Execute({
                Command::make_gfill(1, dP[UP_Y] * -sign,  Point(p,0,q)),
                Command::make_gfill(2, dP[UP_Y] * -sign,  Point(-p,0,q)),
                Command::make_gfill(3, dP[UP_Y] * -sign,  Point(-p,0,-q)),
                Command::make_gfill(4, dP[UP_Y] * -sign,  Point(p,0,-q)),

            });
            for (int s=0;s<p+1;s++) for (int t=0;t<q+1;t++) {
                grounded[i+s][k+t] = false;
                vox.set(true, i+s, j,k+t);
                vox.set_color(0, i+s, j, k+t);
            }
            
            DLOG(INFO) << "try moving";
            moving.clear();
            moving.push_back(Command::make_wait(1));
            if (p==1) {
                moving.push_back(Command::make_wait(2));
                moving.push_back(Command::make_smove(3, Point(0,0,-q+1)));
                moving.push_back(Command::make_smove(4, Point(0,0,-q+1)));
            } else if (q==1) {
                moving.push_back(Command::make_smove(2, Point(-p+1,0,0)));
                moving.push_back(Command::make_smove(3, Point(-p+1,0,0)));
                moving.push_back(Command::make_wait(4));
            } else {
                moving.push_back(Command::make_smove(2, Point(-p+1,0,0)));
                moving.push_back(Command::make_lmove(3, Point(-p+1,0,0), Point(0,0,-q+1)));
                moving.push_back(Command::make_smove(4, Point(0,0,-q+1)));
            }
            ce->Execute(moving);
            ce->Execute({
                Command::make_fusion_p(1, dP[UP_Z]),
                Command::make_fusion_p(2, dP[UP_Z]),
                Command::make_fusion_s(3, dP[UP_Z] * -1),
                Command::make_fusion_s(4, dP[UP_Z] * -1),
            });
            ce->Execute({
                Command::make_fusion_p(1, dP[UP_X]),
                Command::make_fusion_s(2, dP[UP_X] * -1),
            });
            q++;
            res = true;
        }
    }

    for (int i=0;i<R;i++) for (int k=0;k<R;k++) if(grounded[i][k]){
        DLOG(INFO) << "single fill for" << Point(i,j,k);
        res = true;
        Point cur = ce->GetBotStatus()[1].pos;
        for (auto c : getPath(cur, Point(i, j+sign,k))) {
            ce->Execute({c});
        }
        ce->Execute({Command::make_fill(1, Point(0,-sign,0))});
        vox.set(true, i,j,k);
        vox.set_color(0, i, j, k);
    }
    if(res) {
        DLOG(INFO) << "updated y" << j; 
    }
    return res;
}
    
bool CrimeaAI::remove_y(const int j, bvv &grounded, int sign) {
        bool res = false;
        int fd = min(R, 5);
        for (int i=0;i<R-fd;i++) for (int k=0;k<R-fd;k++) if(grounded[i][k]) {
            int p=0; int q=0;
            while(p < fd && grounded[i+p][k]) p++;
            while(q < fd && grounded[i][k+q]) q++;
            if (p==1 || q == 1) continue;
            bool flag = true;
            for (int s=0;s<p;s++) for (int t=0;t<q;t++) if(!grounded[i+s][k+t] && vox.get(i+s,j,k+t)) {
                flag = false;
                break;
            }
            if (flag && (p> 2 || q > 2)) {
                DLOG(INFO) << "try s delete y " << p << " " << q;
                p--; q--;
                Point cur = ce->GetBotStatus()[1].pos;
                for (auto c : getPath(cur, Point(i, j + sign,k))) {
                    ce->Execute({c});
                }
                DLOG(INFO) << "try fisision";
                ce->Execute({Command::make_fission(1, dP[UP_X], 1)});
                ce->Execute({Command::make_fission(1, dP[UP_Z], 0), Command::make_fission(2, dP[UP_Z], 0)});
                vector<Command> moving;
                DLOG(INFO) << "try moving";
                moving.push_back(Command::make_wait(1));
                if (p==1) {
                    moving.push_back(Command::make_wait(2));
                    moving.push_back(Command::make_smove(3, dP[UP_Z] * (q-1)));
                    moving.push_back(Command::make_smove(4, dP[UP_Z] * (q-1)));
                } else if (q==1) {
                    moving.push_back(Command::make_smove(2, dP[UP_X] * (p-1)));
                    moving.push_back(Command::make_smove(3, dP[UP_X] * (p-1)));
                    moving.push_back(Command::make_wait(4));
                } else {
                    moving.push_back(Command::make_smove(2, dP[UP_X] * (p-1)));
                    moving.push_back(Command::make_lmove(3, dP[UP_X] * (p-1), dP[UP_Z] * (q-1)));
                    moving.push_back(Command::make_smove(4, dP[UP_Z] * (q-1)));
                }
                ce->Execute(moving);
                DLOG(INFO) << "try gvoid";
                ce->Execute({
                    Command::make_gvoid(1, dP[UP_Y] * -sign,  Point(p,0,q)),
                    Command::make_gvoid(2, dP[UP_Y] * -sign,  Point(-p,0,q)),
                    Command::make_gvoid(3, dP[UP_Y] * -sign,  Point(-p,0,-q)),
                    Command::make_gvoid(4, dP[UP_Y] * -sign,  Point(p,0,-q)),

                });
                for (int s=0;s<p+1;s++) for (int t=0;t<q+1;t++) {
                    grounded[i+s][k+t] = false;
                    vox.set(false, i+s, j,k+t);
                }
                
                DLOG(INFO) << "try moving";
                moving.clear();
                moving.push_back(Command::make_wait(1));
                if (p==1) {
                    moving.push_back(Command::make_wait(2));
                    moving.push_back(Command::make_smove(3, Point(0,0,-q+1)));
                    moving.push_back(Command::make_smove(4, Point(0,0,-q+1)));
                } else if (q==1) {
                    moving.push_back(Command::make_smove(2, Point(-p+1,0,0)));
                    moving.push_back(Command::make_smove(3, Point(-p+1,0,0)));
                    moving.push_back(Command::make_wait(4));
                } else {
                    moving.push_back(Command::make_smove(2, Point(-p+1,0,0)));
                    moving.push_back(Command::make_lmove(3, Point(-p+1,0,0), Point(0,0,-q+1)));
                    moving.push_back(Command::make_smove(4, Point(0,0,-q+1)));
                }
                ce->Execute(moving);
                DVLOG(2) << "try fusion 1 " << ce->GetBotStatus()[1].pos;
                DVLOG(2) << "try fusion 2 " << ce->GetBotStatus()[2].pos;
                DVLOG(2) << "try fusion 3 " << ce->GetBotStatus()[3].pos;
                DVLOG(2) << "try fusion 4 " << ce->GetBotStatus()[4].pos;
                ce->Execute({
                    Command::make_fusion_p(1, dP[UP_Z]),
                    Command::make_fusion_p(2, dP[UP_Z]),
                    Command::make_fusion_s(3, dP[UP_Z] * -1),
                    Command::make_fusion_s(4, dP[UP_Z] * -1),
                });
                ce->Execute({
                    Command::make_fusion_p(1, dP[UP_X]),
                    Command::make_fusion_s(2, dP[UP_X] * -1),
                });
                q++;
                res = true;
            }

            k += q;
        }

        for (int i=0;i<R;i++) for (int k=0;k<R;k++) if(grounded[i][k]){
            res = true;
            Point cur = ce->GetBotStatus()[1].pos;
            for (auto c : getPath(cur, Point(i, j+sign,k))) {
                ce->Execute({c});
            }
            ce->Execute({Command::make_void(1, Point(0,-sign,0))});
            vox.set(false, i,j,k);

        }
        return res;
}
    
bool CrimeaAI::remove_z(const int k, bvv &grounded, int sign) {

    bool res = false;
    bool fd = min(R,5);
    for (int i=0;i<R-fd;i++) for (int j=0;j<R-fd;j++) if(grounded[i][j]) {
        int p=0; int q=0;
        while(p < fd && grounded[i+p][j]) p++;
        while(q < fd && grounded[i][j+q]) q++;
        if (p==1 || q == 1) continue;
        bool flag = true;
        for (int s=0;s<p;s++) for (int t=0;t<q;t++) if(!grounded[i+s][j+t] && vox.get(i+s,j+t,k)) {
            flag = false;
            break;
        }
        if (flag && (p> 2 || q > 2)) {
            DLOG(INFO) << "try s delete z " << p << " " << q;
            p--; q--;
            Point cur = ce->GetBotStatus()[1].pos;
            for (auto c : getPath(cur, Point(i, j,k+sign))) {
                ce->Execute({c});
            }
            DLOG(INFO) << "try fisision";
            ce->Execute({Command::make_fission(1, dP[UP_X], 1)});
            ce->Execute({Command::make_fission(1, dP[UP_Y], 0), Command::make_fission(2, dP[UP_Y], 0)});
            vector<Command> moving;
            DLOG(INFO) << "try moving";
            moving.push_back(Command::make_wait(1));
            if (p==1) {
                moving.push_back(Command::make_wait(2));
                moving.push_back(Command::make_smove(3, dP[UP_Y] * (q-1)));
                moving.push_back(Command::make_smove(4, dP[UP_Y] * (q-1)));
            } else if (q==1) {
                moving.push_back(Command::make_smove(2, dP[UP_X] * (p-1)));
                moving.push_back(Command::make_smove(3, dP[UP_X] * (p-1)));
                moving.push_back(Command::make_wait(4));
            } else {
                moving.push_back(Command::make_smove(2, dP[UP_X] * (p-1)));
                moving.push_back(Command::make_lmove(3, dP[UP_X] * (p-1), dP[UP_Y] * (q-1)));
                moving.push_back(Command::make_smove(4, dP[UP_Y] * (q-1)));
            }
            ce->Execute(moving);
            DLOG(INFO) << "try gvoid";
            ce->Execute({
                Command::make_gvoid(1, dP[UP_Z] * -sign,  Point(p,q,0)),
                Command::make_gvoid(2, dP[UP_Z] * -sign,  Point(-p,q,0)),
                Command::make_gvoid(3, dP[UP_Z] * -sign,  Point(-p,-q,0)),
                Command::make_gvoid(4, dP[UP_Z] * -sign,  Point(p,-q,0)),

            });
            for (int s=0;s<p+1;s++) for (int t=0;t<q+1;t++) {
                grounded[i+s][j+t] = false;
                vox.set(false, i+s, j+t,k);
            }
            
            DLOG(INFO) << "try moving";
            moving.clear();
            moving.push_back(Command::make_wait(1));
            if (p==1) {
                moving.push_back(Command::make_wait(2));
                moving.push_back(Command::make_smove(3, Point(0,-q+1,0)));
                moving.push_back(Command::make_smove(4, Point(0,-q+1,0)));
            } else if (q==1) {
                moving.push_back(Command::make_smove(2, Point(-p+1,0,0)));
                moving.push_back(Command::make_smove(3, Point(-p+1,0,0)));
                moving.push_back(Command::make_wait(4));
            } else {
                moving.push_back(Command::make_smove(2, Point(-p+1,0,0)));
                moving.push_back(Command::make_lmove(3, Point(-p+1,0,0), Point(0,-q+1,0)));
                moving.push_back(Command::make_smove(4, Point(0,-q+1,0)));
            }
            ce->Execute(moving);
            DLOG(INFO) << "try fusion 1 " << ce->GetBotStatus()[1].pos;
            DLOG(INFO) << "try fusion 2 " << ce->GetBotStatus()[2].pos;
            DLOG(INFO) << "try fusion 3 " << ce->GetBotStatus()[3].pos;
            DLOG(INFO) << "try fusion 4 " << ce->GetBotStatus()[4].pos;
            ce->Execute({
                Command::make_fusion_p(1, dP[UP_Y]),
                Command::make_fusion_p(2, dP[UP_Y]),
                Command::make_fusion_s(3, dP[UP_Y] * -1),
                Command::make_fusion_s(4, dP[UP_Y] * -1),
            });
            ce->Execute({
                Command::make_fusion_p(1, dP[UP_X]),
                Command::make_fusion_s(2, dP[UP_X] * -1),
            });
            q++;
            res = true;
        }

        j += q;
    }
    for (int i=0;i<R;i++) for (int j=0;j<R;j++) if(grounded[i][j]){
        res = true;
        Point cur = ce->GetBotStatus()[1].pos;
        CHECK(ce->GetSystemStatus().matrix[i][j][k + sign] == VoxelState::VOID) << Point(i,j,k+sign);
        for (auto c : getPath(cur, Point(i, j,k + sign))) {
            ce->Execute({c});
        }
        ce->Execute({Command::make_void(1, Point(0,0, -sign))});
        vox.set(false, i,j,k);

    }
    return res;
}