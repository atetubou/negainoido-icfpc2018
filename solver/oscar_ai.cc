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
#include "crimea_ai.h"
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

class OscarAI : public CrimeaAI
{
  public:
    OscarAI(const vvv &src_model, const vvv &tgt_model) : CrimeaAI(src_model, tgt_model) { };
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
        DLOG(INFO) << "removed src";
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

        DLOG(INFO) << "finalvent";
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
        DLOG(INFO) << FLAGS_src_filename;
        src_model = ReadMDL(FLAGS_src_filename);
        R = src_model.size();
    }

    vvv tgt_model;
    if(!FLAGS_tgt_filename.empty() && FLAGS_tgt_filename != "-") {
        DLOG(INFO) << FLAGS_tgt_filename;
        tgt_model = ReadMDL(FLAGS_tgt_filename);
        R = tgt_model.size();
    }

    if (FLAGS_src_filename == "-") {
        DLOG(INFO) << "Start with empty src";
        src_model = vvv(R, vv(R, v(R, 0)));
    }
    if (FLAGS_tgt_filename == "-") {
        DLOG(INFO) << "Start with empty tgt";
        tgt_model = vvv(R, vv(R, v(R, 0)));
    }
    DLOG(INFO) << "R: " << R;
    auto oscar_ai = std::make_unique<OscarAI>(src_model, tgt_model);
    oscar_ai->Run();
    oscar_ai->Finalize();
}
