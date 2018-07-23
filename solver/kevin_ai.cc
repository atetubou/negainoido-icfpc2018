#include "crimea_ai.h"

#include "src/base/base.h"
#include "src/base/flags.h"
#include "src/command_util.h"
#include "src/command.h"
#include "src/command_executer.h"
#include <queue>

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


using namespace std;
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

class KevinAI : public CrimeaAI {
    bool safe_fill_y(const int j, bvv &grounded, int sign) {
        bool res = false;
        int fd = min(R, 6);
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
                bool shouldHigh = false;
                if (ce->GetSystemStatus().harmonics == Harmonics::LOW) {
                    shouldHigh = true;
                    for (int s=0;s<p;s++) for (int t=0;t<q;t++) if(vox.get(i+s,j-sign,k+t)) {
                        shouldHigh = false;
                        s = p; t = q;
                    }
                }
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
                if (shouldHigh) {
                    DLOG(INFO) << "need to high";
                    ce->Execute({
                        Command::make_flip(1),
                        Command::make_wait(2),
                        Command::make_wait(3),
                        Command::make_wait(4)
                    });
                }
                DLOG(INFO) << "try gfill";
                ce->Execute({
                    Command::make_gfill(1, dP[UP_Y] * -sign,  Point(p,0,q)),
                    Command::make_gfill(2, dP[UP_Y] * -sign,  Point(-p,0,q)),
                    Command::make_gfill(3, dP[UP_Y] * -sign,  Point(-p,0,-q)),
                    Command::make_gfill(4, dP[UP_Y] * -sign,  Point(p,0,-q)),

                });
                int color = vox.add_color();
                for (int s=0;s<p+1;s++) for (int t=0;t<q+1;t++) {
                    grounded[i+s][k+t] = false;
                    vox.set(true, i+s, j,k+t);
                    vox.set_color(color, i+s, j, k+t);
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
            res = true;
            Point cur = ce->GetBotStatus()[1].pos;
            for (auto c : getPath(cur, Point(i, j+sign,k))) {
                ce->Execute({c});
            }
            vox.set(true, i,j,k);
            vox.set_color(vox.add_color(), i, j, k);
            if (ce->GetSystemStatus().harmonics == Harmonics::LOW && vox.get_parent_color(i,j,k) != 0) {
                ce->Execute({Command::make_flip(1)});
            }
            ce->Execute({Command::make_fill(1, Point(0,-sign,0))});
        }
        if(res) {
            DLOG(INFO) << "updated y" << j; 
        }
        for (int i=0;i<R;i++) for(int k=0;k<R;k++) {
            DCHECK(ce->GetSystemStatus().matrix[i][j][k] != VoxelState::VOID || !vox.get(i,j,k)) << "wrong empty";
            DCHECK(ce->GetSystemStatus().matrix[i][j][k] != VoxelState::FULL || vox.get(i,j,k)) << "wrong full";
            DCHECK(!vox.get(i,j,k) || vox.get_color(i,j,k) != -1) << "wrong empty color";
            DCHECK(vox.get(i,j,k) || vox.get_color(i,j,k) == -1) << "wrong full color";
        }
        return res;
    }

  public:
    KevinAI(const vvv &src_model, const vvv &tgt_model) : CrimeaAI(src_model, tgt_model) { };
    ~KevinAI() override = default;

    void Run() override {
        Point pos = Point(0,0,0);
        // remove all 
        bool renew = true;
        while (renew) {
            renew = false;
            // UP_Y
            bvv used = bvv(R,bv(R,false));
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
        }
        bool find = false;
        for (int i=0;i<R;i++) for(int j=0;j<R;j++) for(int k=0;k<R;k++) {
            if(ce->GetSystemStatus().matrix[i][j][k] == 0 || tvox.get(i,j,k)) {
                find = true;
                i = R; j=R; k=R;
            }
        } 

        if(find) {
            ce->Execute({Command::make_flip(1)});
            // UP_Y
            for (int j=R-2;j>=0;j--) {
                queue<P> invalid;
                bvv grounded = bvv(R, bv(R, false));
                for (int i=0;i<R;i++) for (int k=0;k<R;k++) if(vox.get(i,j,k)) {
                    grounded[i][k] = true;
                }
                renew |= remove_y(j, grounded, 1);
            }
            ce->Execute({Command::make_flip(1)});
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
        for (int j=1;j<R;j++) {
            grounded = bvv(R, bv(R,false));
            bool find = false;
            for (int i=0;i<R;i++) for (int k=0;k<R;k++) if (!vox.get(i,j+1,k) && tvox.get(i,j,k)) {
                grounded[i][k] = true;
                if (!(vox.get(i,j-1,k) && !vox.get(i,j,k))) {
                    find = true;
                }
            }
            safe_fill_y(j,grounded,1);
            if (ce->GetSystemStatus().harmonics == Harmonics::HIGH) {
                int find = false;
                for (int i=0;i<R;i++) for (int k=0;k<R;k++) if (vox.get(i,j,k) && vox.get_parent_color(i,j,k)!= 0) {
                    find = true;
                    break;
                }
                if(!find) {
                    DLOG(INFO) << "seems safe";
                    ce->Execute({Command::make_flip(1)});
                }
            }
        }
        if (ce->GetSystemStatus().harmonics != Harmonics::LOW) {
            ce->Execute({Command::make_flip(1)});
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
        VLOG(2) << FLAGS_src_filename;
        src_model = ReadMDL(FLAGS_src_filename);
        R = src_model.size();
    }

    vvv tgt_model;
    if(!FLAGS_tgt_filename.empty() && FLAGS_tgt_filename != "-") {
        VLOG(2) << FLAGS_tgt_filename;
        tgt_model = ReadMDL(FLAGS_tgt_filename);
        R = tgt_model.size();
    }

    if (FLAGS_src_filename == "-") {
        VLOG(2) << "Start with empty src";
        src_model = vvv(R, vv(R, v(R, 0)));
    }
    if (FLAGS_tgt_filename == "-") {
        VLOG(2) << "Start with empty tgt";
        tgt_model = vvv(R, vv(R, v(R, 0)));
    }
    VLOG(2) << "R: " << R;
    auto oscar_ai = std::make_unique<KevinAI>(src_model, tgt_model);
    oscar_ai->Run();
    oscar_ai->Finalize();
}
