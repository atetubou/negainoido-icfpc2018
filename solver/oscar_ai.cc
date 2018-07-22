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

static void calc_invalid(queue<P> &invalid, bvv &used, bvv &grounded) {
    int R = used.size();
    while(!invalid.empty()) {
        P cur = invalid.front();
        int j = cur.first;
        int k = cur.second;
        invalid.pop();
        if (j < 0 || j>= R || k<0 || k>=R) continue;
        if(used[j][k])  continue;
        used[j][k] = true;
        grounded[j][k] = false;
        for (int p=0;p<4;p++) {
            int nx = j+dx[p];
            int ny = k+dy[p];
            if (grounded[nx][ny]) {
                invalid.push(P(nx,ny));
            }
        }
    }
}

class OscarAI : public AI
{
    int R;
    vvv tgt_model;
    Vox vox;

    vector<Command> getPath(Point &pos, const Point &dest) {
        LOG(INFO) << "finding " << pos << " " << dest;
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
    
    bool remove_y(int j, bvv &grounded, int sign) {
        bool res = false;
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
    
    bool remove_z(int k, bvv &grounded, int sign) {
        bool res = false;
        for (int i=0;i<R;i++) for (int j=0;j<R;j++) if(grounded[i][j]){
            res = true;
            Point cur = ce->GetBotStatus()[1].pos;
            for (auto c : getPath(cur, Point(i, j,k + sign))) {
                ce->Execute({c});
            }
            ce->Execute({Command::make_void(1, Point(0,0, sign))});
            vox.set(false, i,j,k);

        }
        return res;
    }


  public:
    OscarAI(const vvv &src_model, const vvv &tgt_model) : AI(src_model), tgt_model(tgt_model), vox(src_model) {
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
                    if(j!=0 && vox.get(i+1, j, k) && !used[j][k]) {
                        grounded[j][k] = true;
                    } else {
                        invalid.push(P(j,k));
                    }
                }
                calc_invalid(invalid, used, grounded);
                renew |= remove_x(i, grounded, -1);
            }
            // DOWN_Y
            used = bvv(R,bv(R,false));
            for (int j=1;j<R;j++) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int i=0;i<R;i++) for (int k=0;k<R;k++) if(vox.get(i,j,k)) {
                    if(!used[i][k] && vox.get(i, j+1, k)) {
                        grounded[i][k] = true;
                    } else {
                        invalid.push(P(i,k));
                    }
                }
                calc_invalid(invalid, used, grounded);
                renew |= remove_y(j, grounded, -1);
            }
            // DOWN_Z
            used = bvv(R,bv(R,false));
            for (int k=1;k<R;k++) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int i=0;i<R;i++) for (int j=0;j<R;j++) if(vox.get(i,j,k)) {
                    if(!used[i][j] &&j!=0 &&vox.get(i, j, k+1)) {
                        grounded[i][j] = true;
                    } else {
                        invalid.push(P(i,j));
                    }
                }
                calc_invalid(invalid, used, grounded);
                renew |= remove_z(k, grounded, -1);
            }
            
            // UP_X
            used = bvv(R,bv(R,false));
            for (int i=R-2;i>=0;i--) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int j=0;j<R;j++) for (int k=0;k<R;k++) if(vox.get(i,j,k)) {
                    if(!used[j][k] && j!=0 &&vox.get(i-1, j, k)) {
                        grounded[j][k] = true;
                    } else {
                        invalid.push(P(j,k));
                    }
                }
                calc_invalid(invalid, used, grounded);
                renew |= remove_x(i, grounded, 1);
            }
            // UP_Y
            used = bvv(R,bv(R,false));
            for (int j=R-2;j>=0;j--) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int i=0;i<R;i++) for (int k=0;k<R;k++) if(vox.get(i,j,k)) {
                    if(!used[i][k] &&( j==0 || vox.get(i, j-1, k))) {
                        grounded[i][k] = true;
                    } else {
                        invalid.push(P(i,k));
                    }
                }
                calc_invalid(invalid, used, grounded);
                renew |= remove_y(j, grounded, 1);
            }
            // UP_Z
            used = bvv(R,bv(R,false));
            for (int k=R-2;k>=0;k--) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int i=0;i<R;i++) for (int j=0;j<R;j++) if(vox.get(i,j,k)) {
                    if(!used[i][j] && j!=0 && vox.get(i, j, k-1)) {
                        grounded[i][j] = true;
                    } else {
                        invalid.push(P(i,j));
                    }
                }
                calc_invalid(invalid, used, grounded);
                renew |= remove_z(k, grounded, 1);
            }
        }

        for (auto c : getPath(pos, Point(0,0,0))) {
            ce->Execute({c});
        }

        CHECK(ce->GetBotStatus()[1].pos == Point(0,0,0));
        for (int i=0;i<R;i++) for(int j=0;j<R;j++) for(int k=0;k<R;k++) {
            CHECK(ce->GetSystemStatus().matrix[i][j][k] == 0) << "Failed to delete" << Point(i,j,k);
        } 
        // build all by simple_solve

        priority_queue<pair<int, Point>, vector<pair<int, Point>>, greater<pair<int, Point>>> pque;

        const int R = tgt_model.size();

        for (size_t x = 0; x < tgt_model.size(); x++) {
            for (size_t z = 0; z < tgt_model[x][0].size(); z++) {
                if (tgt_model[x][0][z] == 1) {
                    pque.push(make_pair(x + z, Point(x, 0, z)));
                }
            }
        }
        if (pque.empty()) {
            LOG(INFO) << "empty target.";
            ce->Execute({Command::make_halt(1)});
            return;
        }

        vector<Point> visit_order;
        visit_order.emplace_back(0, 0, 0);

        vvv visited(R, vv(R, v(R, 0)));

        while (!pque.empty()) {
            const Point cur = pque.top().second;
            pque.pop();

            if (visited[cur.x][cur.y][cur.z]) {
                continue;
            }

            visited[cur.x][cur.y][cur.z] = 1;
            visit_order.push_back(cur);

            for (int i = 0; i < 6; i++) {
                int nx = cur.x + dx[i];
                int ny = cur.y + dy[i];
                int nz = cur.z + dz[i];
                if (nx >= 0 && ny >= 0 && nz >= 0 && nx < R && ny < R && nz < R) {
                    if (tgt_model[nx][ny][nz] == 1 && !visited[nx][ny][nz]) {
                        pque.push(make_pair(nx + ny + nz, Point(nx, ny, nz)));
                    }
                }
            }
        }

        visit_order.emplace_back(0, 0, 0);

        evvv voxel_states(R, evv(R, ev(R, MyVoxelState::kALWAYSEMPTY)));
        for (int x = 0; x < R; ++x) {
            for (int y = 0; y < R; ++y) {
                for (int z = 0; z < R; ++z) {
                    if (tgt_model[x][y][z]) {
                        voxel_states[x][y][z] = MyVoxelState::kSHOULDBEFILLED;
                    }
                }
            }
        }

        LOG(INFO) << "start path construction";

        int total_move = 0;
        vector<Command> result_buff;

        for (size_t i = 0; i + 1 < visit_order.size(); ++i) {
            const auto cur = visit_order[i];
            const auto &next = visit_order[i + 1];

            const auto commands = get_commands_for_next(cur, next, voxel_states);

            total_move += commands.size();

            result_buff.push_back(commands[0]);
            if (i > 0) {
                const Point &nd = commands[0].smove_.lld;
                result_buff.push_back(Command::make_fill(1, Point(-nd.x, -nd.y, -nd.z)));
                result_buff = MergeSMove(result_buff);
                for (auto c : result_buff) {
                    ce->Execute({c});
                }
                result_buff.clear();
                voxel_states[cur.x][cur.y][cur.z] = MyVoxelState::kALREADYFILLED;
            }

            for (size_t i = 1; i < commands.size(); ++i)
            {
                result_buff.push_back(commands[i]);
            }
        }

        result_buff.push_back(Command::make_halt(1));
        result_buff = MergeSMove(result_buff);
        for (auto c : result_buff) {
            ce->Execute({c});
            result_buff.clear();
        }

        LOG(INFO) << "done path construction R=" << R
                  << " total_move=" << total_move
                  << " move_per_voxel=" << static_cast<double>(total_move) / (visit_order.size() - 2);

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
    if (FLAGS_src_filename != "-") {
        src_model = ReadMDL(FLAGS_src_filename);
        R = src_model.size();
    }

    vvv tgt_model;
    if(FLAGS_tgt_filename != "-") {
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
