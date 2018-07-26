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

#define FOR(i,a,b) for(int i=(a);i<(int)(b);i++)
#define REP(i,n)  FOR(i,0,n)
#define INR(x,R) (0 <= (x) && (x) < (R))


using namespace std;
using cvv = vector<vector<Command>>;
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
    void tryReserve(VBot &b, VArea &varea) {
        if(b.reserved.empty()) {
            b.state = VBot::SLEEPING;
            return;
        }
        auto &tar = b.reserved.front();
        DLOG(INFO) << "check reserved" << tar.x << " " << tar.ex << " " << tar.z << " " << tar.ez;
        if (!varea.reserve(b.id, b.reserved.front())) {
            DLOG(INFO) << "failed to reserve " << b.id;
            return;
        }
        DLOG(INFO) << "empty start reserve";
        queue<P> que;
        vv memo = vv(R, v(R, -1));
        int count = 0;
        int fx =-1,fz =-1;
        que.push(P(b.pos.x, b.pos.z));
        while(!que.empty()) {

            int curx = que.front().first;
            int curz = que.front().second;
            DVLOG(2) << "searching " << curx << " " << curz;
            que.pop();
            if(memo[curx][curz] != -1) continue;
            memo[curx][curz] = count;
            if(b.inNextTarget(curx,curz)) {
                fx= curx;
                fz=curz;
                break;
            }
            count++;

            REP (i,4) {
                int nx = curx + dx[i];
                int nz = curz + dy[i];
                if (varea.checkReserve(b.id, nx, nz)) {
                    que.push(P(nx,nz));
                }
            }
        }
        if (fx==-1 || fz== -1 || memo[fx][fz] == -1) {
            DLOG(INFO) << "failed to reseve";
            b.state = VBot::WAITING;
            return;
        } else if(memo[fx][fz]==0) {
            DLOG(INFO) << "set working";
            b.state = VBot::WORKING;
        } else {
            DLOG(INFO) << "set moving";
            b.state = VBot::MOVING;
            int curx = fx;
            int curz = fz;
            bool res = varea.reserve(b.id, curx, curz);
            DCHECK(res);
            while(curx != b.pos.x || curz != b.pos.z) {
                int md = -1;
                int mc = memo[curx][curz];
                for (int i=0;i<4;i++) {
                    int nx = curx+dx[i];
                    int nz = curz+dy[i];
                    if (nx>=0 && nx < R && nz >=0 && nz < R && memo[nx][nz] != -1 && memo[nx][nz] < mc) {
                        md = i;
                        mc = memo[nx][nz];
                    }
                }
                DCHECK(md != -1) << "back trace failed at " << curx << " " << curz;
                curx += dx[md];
                curz += dy[md];
                varea.reserve(b.id, curx, curz);
                DLOG(INFO) << "reserved path " << curx << " " << curz;
            }
        }
        DLOG(INFO) << "reserve area" << b.id;
        varea.reserve(b.id, tar);
    }

    void moveReservedPath(VBot &b, VArea &varea, set<Point> &filled) {
        DLOG(INFO) << "start moving";
        for (int i=0;i<4;i++) {
            int nx = b.pos.x;
            int nz = b.pos.z;
            int d=0;
            while(INR(nx+dx[i],R) && INR(nz+dy[i],R) && varea.get(nx+dx[i],nz+dy[i])==b.id && d < 15 && !b.inNextTarget(nx, nz)) {
                varea.free(nx,nz);
                d += 1;
                nx += dx[i];
                nz += dy[i];
            }
            if (d==0 && b.inNextTarget(nx,nz)) {
                workVBot(b, varea, filled);
                return;
            }
            if (d==0) continue;
            DLOG(INFO) << "get dir";
            DLOG(INFO) << "move to " << b.pos.x << " " << b.pos.z << " to " << nx << " " << nz;
            b.pos.x = nx;
            b.pos.z = nz;
            if (b.inNextTarget(nx,nz)) {
                DLOG(INFO) << "reached";
                b.state = VBot::WORKING;
            }
            DCHECK(varea.get(nx,nz) == b.id) << "invalid move" << nx << " "  << nz << " " << b.id;
            b.command_queue.push(Command::make_smove(b.id, Point(dx[i]*d, 0, dy[i]*d)));
            return;
        }
        DCHECK(false) << "FAILED to construct path";
    }

    void workVBot(VBot &b, VArea &varea, set<Point> &filled) {
        DLOG(INFO) << "let's work";
        int cx = b.pos.x;
        int cy = b.pos.y-1;
        int cz = b.pos.z;
        if (!vox.get(cx,cy,cz) && tvox.get(cx,cy,cz)) {
            filled.insert(Point(cx,cy,cz));
            b.command_queue.push(Command::make_fill(b.id, Point(0, -1, 0)));
            return;
        }
        auto tar = b.reserved.front();
        FOR(i,tar.x,tar.ex) FOR(j,tar.z,tar.ez)  {
            if (!vox.get(i,cy,j) && tvox.get(i,cy,j)) {
                b.pos.x = i;
                b.pos.z = j;
                DCHECK(varea.get(i,j)==b.id) << "invalid move" << i << " "  << j << " " << b.id << " " << varea.get(i,j);
                if(i==cx) {
                    b.command_queue.push(Command::make_smove(b.id, Point(0,0,j-cz)));
                    return;
                }
                if(j==cz) {
                    b.command_queue.push(Command::make_smove(b.id, Point(i-cx,0,0)));
                    return;
                }
                b.command_queue.push(Command::make_lmove(b.id, Point(i-cx,0,0), Point(0,0,j-cz)));
                return;
            }
        }
        varea.freeArea(b,tar);
        b.state = VBot::WAITING;
        b.reserved.pop();
    }

    // assumption: all y of vbot pos are j;
    void run_parallel(vector<VBot> &vbots, int j) {
        DLOG(INFO) << "exec parallel";
        bool busy = true;

        VArea varea = VArea(R);
        for (auto &b:vbots) {
            varea.reserve(b.id, b.pos.x, b.pos.z);
            b.state = VBot::WAITING;
        }
        
        while (busy) {
            busy = false;
            set<Point> filled;

            for (auto &b : vbots) {
                if(!b.command_queue.empty()) continue;
                switch(b.state) {
                    case VBot::SLEEPING:
                        break;
                    case VBot::MOVING:
                        busy = true;
                        moveReservedPath(b, varea, filled);
                        break;
                    case VBot::WAITING:
                        DLOG(INFO) << " now waiting " << b.id;
                        tryReserve(b, varea); 
                        if (b.state == VBot::MOVING) {
                            DLOG(INFO) << " start moving " << b.id;
                            moveReservedPath(b, varea, filled);
                            busy = true;
                        } else if (b.state==VBot::WORKING) {
                            DLOG(INFO) << " start working " << b.id;
                            workVBot(b, varea, filled);
                            busy = true;
                        } else if (b.state==VBot::SLEEPING) {
                            DLOG(INFO) << " start sleeping " << b.id;
                            if(b.pos.y+1 < R) {
                                b.pos.y++;
                                b.command_queue.push(Command::make_smove(b.id, dP[UP_Y]));
                                busy = true;
                            }
                        }
                        break;
                    case VBot::WORKING:
                        busy = true;
                        workVBot(b, varea, filled);
                        DLOG(INFO) << " work end" << b.id;
                        break;
                    default:
                        LOG(FATAL) << "INVALID STATE";
                }
            }
            for (auto &b: vbots) {
                DLOG(INFO) << "vbots " << b.id << " state " << b.state << " pos " << b.pos;
                DCHECK(b.pos.y != j || varea.get(b.pos.x,b.pos.z) == b.id) << "on not reserved pos " << varea.get(b.pos.x,b.pos.z);
            }
            varea.runFree();


            if (busy) {
                DLOG(INFO) << "running parallel";

                if (ce->GetSystemStatus().harmonics == Harmonics::LOW) {
                    
                    bool found = false;
                    for (auto &p: filled) if (p.y!=0 && !vox.get(p.x, p.y-1, p.z)) {
                        DLOG(INFO) << " found ungrounded point " << p;
                        found = true;
                        break;
                    }
                    if (found) {
                        bool flag = false;
                        for (auto &b : vbots) {
                            if (b.command_queue.empty()) {
                                b.command_queue.push(Command::make_flip(b.id));
                                flag = true;
                                break;
                            }
                        }
                        if (!flag) {
                            vector<Command> flips;
                            flips.push_back(Command::make_flip(vbots[0].id));
                            FOR(i,1,vbots.size()) {
                                flips.push_back(Command::make_wait(vbots[i].id));
                            }
                            ce->Execute(flips);
                        }
                    }
                }
                for (auto &p: filled) {
                    vox.set(true, p.x, p.y, p.z);
                    vox.set_color(vox.add_color(), p.x,p.y,p.z);
                }

                vector<Command> real_commands;
                for (auto &b : vbots) {
                    if (b.command_queue.empty()) {
                        real_commands.push_back(Command::make_wait(b.id));
                    } else {
                        real_commands.push_back(b.command_queue.front());
                        b.command_queue.pop();
                    }
                }
                DCHECK(real_commands.size() == vbots.size()) << "missing real command";
                ce->Execute(real_commands);
                for (auto &b: vbots) {
                    DCHECK(b.pos == ce->GetBotStatus()[b.id].pos) << "failed to check vpos " << b.id;
                }

            }
        }
        if (ce->GetSystemStatus().harmonics == Harmonics::HIGH) {
            bool flag = false;
            REP(i,R) REP(k,R) if(vox.get(i,j-1,k) && vox.get_parent_color(i,j-1,k) != 0){
                flag = true;
                break;
            }
            if (!flag) {
                DLOG(INFO) << "SET LOW HARMONICS";
                vector<Command> flips;
                flips.push_back(Command::make_flip(vbots[0].id));
                FOR(i,1,vbots.size()) {
                    flips.push_back(Command::make_wait(vbots[i].id));
                }
                ce->Execute(flips);
            }
        }
        DLOG(INFO) << "finished";
    }

    void parallel_fusion(vector<VBot> &vbots) {
        int y = vbots[0].pos.y;
        set<Point> invalid;
        for (auto &bot:vbots) {
            invalid.insert(bot.pos);
        }
        REP (i,vbots.size()) {
            vector<Command> bc = getPath(vbots[i].pos, Point(0,y,i), invalid);
            for (auto c : bc) {
                vector<Command> cs;
                c.id = vbots[i].id;
                REP (j,vbots.size()) {
                    if(i==j) {
                        cs.push_back(c);
                    } else {
                        cs.push_back(Command::make_wait(vbots[j].id));
                    }
                }
                ce->Execute(cs);
            }
            invalid.erase(vbots[i].pos);
            vbots[i].pos = Point(0,y,i);
            invalid.insert(vbots[i].pos);
        }
        int count = vbots.size();
        int div = 1;
        
        while (div < count) {
            vector<Command> cs;
            for (int i=0;i<count;i+=2*div) {
                cs.push_back(Command::make_fusion_p(vbots[i].id, dP[UP_Z]));
                cs.push_back(Command::make_fusion_s(vbots[i+div].id, dP[UP_Z] * -1));
                DLOG(INFO) << "fusion at " << vbots[i].pos << " " << vbots[i+div].pos;
            }
            DLOG(INFO) << "fusion";
            ce->Execute(cs);
            div *= 2;
            cs.clear();
            if (div >= count) break;
            for (int i=0;i<count;i+=2*div) {
                cs.push_back(Command::make_wait(vbots[i].id));
                cs.push_back(Command::make_smove(vbots[i+div].id,Point(0,0,-(div-1))));
                vbots[i+div].pos.z -= div-1;
            }
            DLOG(INFO) << "move";
            ce->Execute(cs);
        }
        DLOG(INFO) << "fusion end";
    }

    void parallel_fill(vector<VBot> &vbots, const int j, const int L) {
        CHECK(L < 6) << "L size error";
        DLOG(INFO) << "start filling";
        for (auto &bot: vbots) {
            while(!bot.reserved.empty()) bot.reserved.pop();
        }
        bvv visited = bvv(R,bv(R,0));
        for (int i=0;i<R;i+=L) for(int k=0;k<R;k+=L) {
            bool find = false;
            for (int s=i;s<R && s-i < L;s++) for (int t=k;t<R && t-k < L;t++) if(tvox.get(s,j,t)) {
                find = true;
                s=R;t=R;
            }
            if (find) {
                int tar=0;
                auto vt = VTarget(i, k, min(i+L, R), min(k+L, R));
                REP(i,vbots.size()) {
                    if(vt.in(vbots[i].pos.x, vbots[i].pos.z)) {
                        tar = i;
                        break;
                    }
                    if(vbots[i].reserved.size() < vbots[tar].reserved.size()) {
                        tar = i;
                    }
                }
                vbots[tar].reserved.push(vt);
            }
        }
        run_parallel(vbots,j+1);
    }

    vector<VBot> make_bots(int n) {
        vector<VBot> vbots;
        vbots.push_back(VBot(1));
        while(1) {
            vector<Command> commands;
            int count = vbots.size();
            REP(i,count) {
                int child_id = *(ce->GetBotStatus()[vbots[i].id].seeds.begin());
                commands.push_back(Command::make_fission(vbots[i].id, dP[UP_Z], n / count / 2 - 1));
                vbots.push_back(VBot(child_id));
            }
            ce->Execute(commands);
            commands.clear();
            if ((int) vbots.size() >= n) break;
            REP(i, count) {
                commands.push_back(Command::make_wait(vbots[i].id));
                commands.push_back(Command::make_smove(vbots[i+count].id, dP[UP_Z] * (n / count -1)));
            }
            ce->Execute(commands);
        }
        for (auto &b: vbots) {
            b.pos = ce->GetBotStatus()[b.id].pos;
        }
        return vbots;
    }

  public:
    KevinAI(const vvv &src_model, const vvv &tgt_model) : CrimeaAI(src_model, tgt_model) { };
    ~KevinAI() override = default;

    void Run() override {
        Point pos = Point(0,0,0);

        for (int i=0;i<R;i++) for(int j=0;j<R;j++) for(int k=0;k<R;k++) {
            CHECK(ce->GetSystemStatus().matrix[i][j][k] == 0 || tvox.get(i,j,k)) << "Failed to delete" << Point(i,j,k);
        } 

        DLOG(INFO) << "removed src";
        // build super simple way
        for (auto c : getPath(ce->GetBotStatus()[1].pos, Point(0,0,0))) {
            ce->Execute({c});
        }
        /*
        if (ce->GetSystemStatus().harmonics == Harmonics::LOW) {
            ce->Execute({Command::make_flip(1)});
        }
        */

        int num = 1;
        if (R > 30) {
            num = 16;
        } else if (R > 20) {
            num = 8;
        } else if (R > 10) {
            num = 4;
        }

        LOG(INFO) << "R: " << R << " num:" << num;
        vector<VBot> vbots = make_bots(num);
        
        vector<Command> cs;
        for (auto &bot: vbots) {
            cs.push_back(Command::make_smove(bot.id, Point(0,1,0)));
            bot.pos.y++;
        }
        ce->Execute(cs);
        for (int j=0;j<R-1;j++) {
            DLOG(INFO) << "start filling at " << j;
            parallel_fill(vbots, j, 5);
        }
        DLOG(INFO) << "filling end";
        if (ce->GetSystemStatus().harmonics != Harmonics::LOW) {
            vector<Command> cs;
            cs.push_back(Command::make_flip(1));
            FOR(i,1, vbots.size()) {
                cs.push_back(Command::make_wait(i+1));
            }
            ce->Execute(cs);
        }
        parallel_fusion(vbots);

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
