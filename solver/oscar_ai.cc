#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <deque>
#include <queue>
#include <algorithm>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "src/base/flags.h"
#include "src/command_util.h"
#include "src/command.h"
#include "src/command_executer.h"

#include "solver/AI.h"
#include "oscar.h"
#include "simple_solve.h"

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

static void calc_invalid(queue<P> &invalid, bvv &used, bvv &grounded, bvv &visited) {
    int R = used.size();
    while(!invalid.empty()) {
        P cur = invalid.front();
        int j = cur.first;
        int k = cur.second;
        invalid.pop();
        if (j < 0 || j>= R || k<0 || k>=R) continue;
        if(visited[j][k])  continue;
        visited[j][k] = true;
        used[j][k] = true;
        if (grounded[j][k]) {
            grounded[j][k] = false;
        } else {
            for (int p=0;p<4;p++) {
                int nx = j+dx[p];
                int ny = k+dy[p];
                if (0<= nx && nx < R && 0 <= ny && ny < R && grounded[nx][ny]) {
                    invalid.push(P(nx,ny));
                }
            }
        }
    }
}

class OscarAI : public AI
{
    int R;
    Vox tvox;
    Vox vox;

    vector<Command> getPath(const Point &pos, const Point &dest) {
        //LOG(INFO) << "finding " << pos << " " << dest;
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
                
                if(nx>=0 && nx < R && ny >= 0 && ny < R && nz>= 0 && nz < R) {
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

    bool remove_x(int i, bvv &grounded, int sign) {
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
                LOG(INFO) << "try s delete x " << p << " " << q;
                Point cur = ce->GetBotStatus()[1].pos;
                for (auto c : getPath(cur, Point(i+sign, j,k))) {
                    ce->Execute({c});
                }
                LOG(INFO) << "try fisision";
                ce->Execute({Command::make_fission(1, dP[UP_Y], 1)});
                ce->Execute({Command::make_fission(1, dP[UP_Z], 0), Command::make_fission(2, dP[UP_Z], 0)});
                vector<Command> moving;
                LOG(INFO) << "try moving";
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
                
                LOG(INFO) << "try moving";
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
    
    bool fill_y(const int j, bvv &grounded, int sign) {
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
                LOG(INFO) << "try s fill y " << p << " " << q;
                p--; q--;
                Point cur = ce->GetBotStatus()[1].pos;
                for (auto c : getPath(cur, Point(i, j + sign,k))) {
                    ce->Execute({c});
                }
                LOG(INFO) << "try fisision";
                ce->Execute({Command::make_fission(1, dP[UP_X], 1)});
                ce->Execute({Command::make_fission(1, dP[UP_Z], 0), Command::make_fission(2, dP[UP_Z], 0)});
                vector<Command> moving;
                LOG(INFO) << "try moving";
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
                LOG(INFO) << "try gfill";
                ce->Execute({
                    Command::make_gfill(1, dP[UP_Y] * -sign,  Point(p,0,q)),
                    Command::make_gfill(2, dP[UP_Y] * -sign,  Point(-p,0,q)),
                    Command::make_gfill(3, dP[UP_Y] * -sign,  Point(-p,0,-q)),
                    Command::make_gfill(4, dP[UP_Y] * -sign,  Point(p,0,-q)),

                });
                for (int s=0;s<p+1;s++) for (int t=0;t<q+1;t++) {
                    grounded[i+s][k+t] = false;
                    vox.set(true, i+s, j,k+t);
                }
                
                LOG(INFO) << "try moving";
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
            res = true;
            Point cur = ce->GetBotStatus()[1].pos;
            for (auto c : getPath(cur, Point(i, j+sign,k))) {
                ce->Execute({c});
            }
            ce->Execute({Command::make_fill(1, Point(0,-sign,0))});
            vox.set(true, i,j,k);
        }
        if(res) {
            LOG(INFO) << "updated y" << j; 
        }
        return res;
    }
    
    bool remove_y(const int j, bvv &grounded, int sign) {
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
                LOG(INFO) << "try s delete y " << p << " " << q;
                p--; q--;
                Point cur = ce->GetBotStatus()[1].pos;
                for (auto c : getPath(cur, Point(i, j + sign,k))) {
                    ce->Execute({c});
                }
                LOG(INFO) << "try fisision";
                ce->Execute({Command::make_fission(1, dP[UP_X], 1)});
                ce->Execute({Command::make_fission(1, dP[UP_Z], 0), Command::make_fission(2, dP[UP_Z], 0)});
                vector<Command> moving;
                LOG(INFO) << "try moving";
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
                LOG(INFO) << "try gvoid";
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
                
                LOG(INFO) << "try moving";
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
                LOG(INFO) << "try fusion 1 " << ce->GetBotStatus()[1].pos;
                LOG(INFO) << "try fusion 2 " << ce->GetBotStatus()[2].pos;
                LOG(INFO) << "try fusion 3 " << ce->GetBotStatus()[3].pos;
                LOG(INFO) << "try fusion 4 " << ce->GetBotStatus()[4].pos;
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
    
    bool remove_z(const int k, bvv &grounded, int sign) {

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
                LOG(INFO) << "try s delete z " << p << " " << q;
                p--; q--;
                Point cur = ce->GetBotStatus()[1].pos;
                for (auto c : getPath(cur, Point(i, j,k+sign))) {
                    ce->Execute({c});
                }
                LOG(INFO) << "try fisision";
                ce->Execute({Command::make_fission(1, dP[UP_X], 1)});
                ce->Execute({Command::make_fission(1, dP[UP_Y], 0), Command::make_fission(2, dP[UP_Y], 0)});
                vector<Command> moving;
                LOG(INFO) << "try moving";
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
                LOG(INFO) << "try gvoid";
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
                
                LOG(INFO) << "try moving";
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
                LOG(INFO) << "try fusion 1 " << ce->GetBotStatus()[1].pos;
                LOG(INFO) << "try fusion 2 " << ce->GetBotStatus()[2].pos;
                LOG(INFO) << "try fusion 3 " << ce->GetBotStatus()[3].pos;
                LOG(INFO) << "try fusion 4 " << ce->GetBotStatus()[4].pos;
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


  public:
    OscarAI(const vvv &src_model, const vvv &tgt_model) : AI(src_model), tvox(tgt_model), vox(src_model) {
        R = src_model.size();
     }
    ~OscarAI() override = default;

    void Run() override {
        Point pos = Point(0,0,0);
        // remove all 
        bool renew = true;
        while (renew) {
            // DOWN_X
            renew = false;
            bvv used = bvv(R,bv(R,false));
            for (int i=1;i<R;i++) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int j=0;j<R;j++) for (int k=0;k<R;k++) if(vox.get(i,j,k)) {
                    if(j!=0 && vox.get(i+1, j, k) && !used[j][k] && !vox.get(i-1,j,k)) {
                        grounded[j][k] = true;
                    } else {
                        invalid.push(P(j,k));
                    }
                }
                bvv visited = bvv(R, bv(R, false));
                calc_invalid(invalid, used, grounded, visited);
                renew |= remove_x(i, grounded, -1);
            }
            // DOWN_Y
            used = bvv(R,bv(R,false));
            for (int j=1;j<R;j++) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int i=0;i<R;i++) for (int k=0;k<R;k++) if(vox.get(i,j,k)) {
                    if(!used[i][k] && vox.get(i, j+1, k) && !vox.get(i,j-1,k)) {
                        grounded[i][k] = true;
                    } else {
                        invalid.push(P(i,k));
                    }
                }
                bvv visited = bvv(R, bv(R, false));
                calc_invalid(invalid, used, grounded, visited);
                renew |= remove_y(j, grounded, -1);
            }
            // DOWN_Z
            used = bvv(R,bv(R,false));
            for (int k=1;k<R;k++) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int i=0;i<R;i++) for (int j=0;j<R;j++) if(vox.get(i,j,k)) {
                    if(!used[i][j] &&j!=0 &&vox.get(i, j, k+1) && !vox.get(i,j,k-1)) {
                        grounded[i][j] = true;
                    } else {
                        invalid.push(P(i,j));
                    }
                }
                bvv visited = bvv(R, bv(R, false));
                calc_invalid(invalid, used, grounded, visited);
                renew |= remove_z(k, grounded, -1);
            }
            
            // UP_X
            used = bvv(R,bv(R,false));
            for (int i=R-2;i>=0;i--) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int j=0;j<R;j++) for (int k=0;k<R;k++) if(vox.get(i,j,k)) {
                    if(!used[j][k] && j!=0 &&vox.get(i-1, j, k) && !vox.get(i+1,j,k)) {
                        grounded[j][k] = true;
                    } else {
                        invalid.push(P(j,k));
                    }
                }
                bvv visited = bvv(R, bv(R, false));
                calc_invalid(invalid, used, grounded, visited);
                renew |= remove_x(i, grounded, 1);
            }
            // UP_Y
            used = bvv(R,bv(R,false));
            for (int j=R-2;j>=0;j--) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int i=0;i<R;i++) for (int k=0;k<R;k++) if(vox.get(i,j,k)) {
                    if(!used[i][k] &&( j==0 || vox.get(i, j-1, k)) && !vox.get(i,j+1,k)) {
                        grounded[i][k] = true;
                    } else {
                        invalid.push(P(i,k));
                    }
                }
                bvv visited = bvv(R, bv(R, false));
                calc_invalid(invalid, used, grounded, visited);
                renew |= remove_y(j, grounded, 1);
            }
            // UP_Z
            used = bvv(R,bv(R,false));
            for (int k=R-2;k>=0;k--) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int i=0;i<R;i++) for (int j=0;j<R;j++) if(vox.get(i,j,k)) {
                    if(!used[i][j] && j!=0 && vox.get(i, j, k-1) && !vox.get(i,j,k+1)) {
                        grounded[i][j] = true;
                    } else {
                        invalid.push(P(i,j));
                    }
                }
                bvv visited = bvv(R, bv(R, false));
                calc_invalid(invalid, used, grounded, visited);
                renew |= remove_z(k, grounded, 1);
            }


        }

        for (int i=0;i<R;i++) for(int j=0;j<R;j++) for(int k=0;k<R;k++) {
            CHECK(ce->GetSystemStatus().matrix[i][j][k] == 0 || tvox.get(i,j,k)) << "Failed to delete" << Point(i,j,k);
        } 
        LOG(INFO) << "removed src";
        // build super simple way
        bool updated = true;
        bvv grounded = bvv(R, bv(R,false));
        for (int i=0;i<R;i++) for (int k=0;k<R;k++) if (!vox.get(i,0,k) && tvox.get(i,0,k)) {
            grounded[i][k] = true;
        }
        fill_y(0,grounded, 1);
        while(updated) {
            updated = false;
            for (int j=1;j<R;j++) {
                grounded = bvv(R, bv(R,false));
                for (int i=0;i<R;i++) for (int k=0;k<R;k++) if (vox.get(i,j-1,k) && !vox.get(i,j,k) && !vox.get(i,j+1,k) && tvox.get(i,j,k)) {
                    grounded[i][k] = true;
                }
                updated |= fill_y(j,grounded,1);
            }
        }

        LOG(INFO) << "finalvent";
        for (auto c : getPath(ce->GetBotStatus()[1].pos, Point(0,0,0))) {
            ce->Execute({c});
        }
        ce->Execute({Command::make_halt(1)});

        for (int i=0;i<R;i++) for(int j=0;j<R;j++) for(int k=0;k<R;k++) {
            CHECK(ce->GetSystemStatus().matrix[i][j][k] == VoxelState::VOID || tvox.get(i,j,k)) << "Failed to delete" << Point(i,j,k);
            CHECK(!tvox.get(i,j,k) || ce->GetSystemStatus().matrix[i][j][k] == VoxelState::FULL) << "Failed to construct" << Point(i,j,k);
        } 

        CHECK(ce->GetBotStatus()[1].pos == Point(0,0,0));
    }
};

int main(int argc, char *argv[])
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_src_filename.empty() && FLAGS_tgt_filename.empty())
    {
        std::cout << "need to pass --mdl_filename=/path/to/mdl";
        exit(1);
    }


    int R = 1;
    vvv src_model;
    if (!FLAGS_src_filename.empty() && FLAGS_src_filename != "-") {
        LOG(INFO) << FLAGS_src_filename;
        src_model = ReadMDL(FLAGS_src_filename);
        R = src_model.size();
    }

    vvv tgt_model;
    if(!FLAGS_tgt_filename.empty() && FLAGS_tgt_filename != "-") {
        LOG(INFO) << FLAGS_tgt_filename;
        tgt_model = ReadMDL(FLAGS_tgt_filename);
        R = tgt_model.size();
    }

    if (FLAGS_src_filename == "-") {
        LOG(INFO) << "Start with empty src";
        src_model = vvv(R, vv(R, v(R, 0)));
    }
    if (FLAGS_tgt_filename == "-") {
        LOG(INFO) << "Start with empty tgt";
        tgt_model = vvv(R, vv(R, v(R, 0)));
    }
    LOG(INFO) << "R: " << R;
    auto oscar_ai = std::make_unique<OscarAI>(src_model, tgt_model);
    oscar_ai->Run();
    oscar_ai->Finalize();
}
