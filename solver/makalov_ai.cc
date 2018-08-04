#include "crimea_ai.h"

#include "src/base/base.h"
#include "src/base/flags.h"
#include "src/command_util.h"
#include "src/command.h"
#include "src/command_executer.h"
#include <queue>
#include <unordered_set>

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

struct BIT{
    int N;
    vector<int> bit;

    BIT(int N): N(N), bit(N,0) {};

    void add(int a, int w){
        for(int x=a;x<=N;x += x&-x) bit[x] += w;
    }
    int sum(int a){
        int ret = 0;
        for(int x=a;x > 0; x -= x&-x) ret += bit[x];
        return ret;
    }
};

struct DBIT {
    int H;
    int W;
    vector<BIT> bits;

    DBIT(int H, int W): H(H), W(W), bits(H,BIT(W+1)) {};

    void add(int a, int b, int w){
        bits[a].add(b+1,w);
    }
    int sum(int a, int b){
        int ret = 0;
        REP(i,a+1) ret += bits[i].sum(b+1);
        return ret;
    }
};

struct VTask {
    int id;
    VArea area;
    vector<int> bids;
    VTask(int id, VArea area, vector<int> bids) : id(id), area(area), bids(bids) {};
};

struct VWorker {
    enum State {
        SLEEPING,
        WAITING,
        BLOCKED,
        MOVING,
        READY,
    };
    int id;
    Point pos;
    Point dest;
    queue<Command> command_queue;
    State state;
    State next;
    set<int> cos;
    bool lead;
    VTarget target;
    VWorker(int id) : id(id), state(WAITING), lead(false), target(0,0,0,0) {};
};

class MakalovAI : public CrimeaAI {
    void tryReserve(VWorker &b, VArea &varea) {
        if(b.pos == b.dest){
            DLOG(INFO) << "set ready";
            b.state = b.next;
            return;
        }
        auto &tar = b.dest;
        DLOG(INFO) << "check reserved" << tar.x << " " << tar.y << " " << tar.z;
        if (!varea.checkReserve(b.id, tar.x, tar.z)) {
            DLOG(INFO) << "failed to reserve " << b.id;
            b.state = VWorker::BLOCKED;
            return;
        }
        DLOG(INFO) << "empty start reserve";
        queue<P> que;
        vv memo = vv(R, v(R, -1));
        int count = 0;
        que.push(P(b.pos.x, b.pos.z));
        while(!que.empty()) {

            int curx = que.front().first;
            int curz = que.front().second;
            DVLOG(2) << "searching " << curx << " " << curz;
            que.pop();
            if(memo[curx][curz] != -1) continue;
            memo[curx][curz] = count;
            if(curx == tar.x && curz == tar.z) {
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
        if (memo[tar.x][tar.z] == -1) {
            DLOG(INFO) << "failed to reseve";
            b.state = VWorker::BLOCKED;
            return;
        } else if(memo[tar.x][tar.z]==0) {
            DCHECK(false) << "failed check";
        } else {
            DLOG(INFO) << "set moving";
            b.state = VWorker::MOVING;
            int curx = tar.x;
            int curz = tar.z;
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
    }

    void moveReservedPath(VWorker &b, VArea &varea) {
        DLOG(INFO) << "start moving " << b.id << " " << b.pos << " " << b.dest;
        Point lld1, lld2;
        for (int i=0;i<4;i++) {
            int d=0;
            while(INR(b.pos.x+dx[i],R) && INR(b.pos.z+dy[i],R) && varea.get(b.pos.x+dx[i],b.pos.z+dy[i])==b.id && d < 15 && b.pos != b.dest) {
                varea.free(b.pos.x,b.pos.z);
                d += 1;
                b.pos.x += dx[i];
                b.pos.z += dy[i];
            }
            if (d==0 && b.pos == b.dest) {
                b.state = b.next;
                return;
            }
            if (d==0) continue;
            lld1 = Point(dx[i]*d, 0, dy[i]*d);
            if (d <= 5) {
                for (int j=0;j<4;j++) if (i/2!= j/2) {
                    int d2 =0;
                    while(INR(b.pos.x+dx[j],R) && INR(b.pos.z+dy[j],R) && varea.get(b.pos.x+dx[j],b.pos.z+dy[j])==b.id && d2 < 5 && b.pos != b.dest) {
                        varea.free(b.pos.x,b.pos.z);
                        d2 += 1;
                        b.pos.x += dx[j];
                        b.pos.z += dy[j];
                    }
                    if (d2==0) continue;
                    lld2 = Point(dx[j] * d2, 0 , dy[j]*d2);
                    break;
                }
            }
            if (b.pos == b.dest) {
                DLOG(INFO) << "reached at " << b.dest;
            }
            DLOG(INFO) << "get dir";
            DLOG(INFO) << "move to " << b.pos.x << " " << b.pos.z << " " << lld1 << " " << lld2;
            
            DCHECK(varea.get(b.pos.x,b.pos.z) == b.id) << "invalid move" << b.pos.x << " "  << b.pos.z << " " << b.id;
            if (lld2.Manhattan() == 0) {
                b.command_queue.push(Command::make_smove(b.id, lld1));
            } else {
                b.command_queue.push(Command::make_lmove(b.id, lld1, lld2));
            }
            return;
        }
        DCHECK(false) << "FAILED to construct path";
    }

    // assumption: all y of vbot pos are j;
    void run_parallel(vector<VWorker> &workers, int j, queue<VTarget> &targets) {
        DLOG(INFO) << "exec parallel";
        bool busy = true;

        VArea varea = VArea(R);

        vector<Command> tmpc;
        bool hen = false;
        for (auto &w : workers) {
            if (w.pos.y != j) {
                tmpc.push_back(Command::make_smove(w.id, Point(0, j-w.pos.y, 0)));
                w.pos.y = j;
                hen = true;
            } else {
                tmpc.push_back(Command::make_wait(w.id));
            }
        }
        for (auto &w : workers) {
            w.state = VWorker::WAITING;
            varea.reserve(w.id, w.pos.x, w.pos.z);
        }
        if (hen) {
            ce->Execute(tmpc);
        }

        
        while (busy) {
            busy = false;
            map< int,set<Point> > filled;

            queue<VTarget> unasigned;
            bool has_free_worker = false;
            REP (a,workers.size()) if (workers[a].state==VWorker::WAITING) {
                auto &w = workers[a];
                has_free_worker = true;
                REP(b,workers.size()) if (b != a && workers[b].state==VWorker::BLOCKED && workers[b].next == VWorker::READY && workers[b].lead) {
                    auto &v = workers[b];
                    int vd = (v.dest - v.pos).Manhattan();
                    int wd = (v.dest - w.pos).Manhattan();
                    if (wd < vd) {
                        DLOG(INFO) << "reasign " << v.id << " to " << w.id;
                        w.dest = v.dest;
                        w.state = VWorker::BLOCKED;
                        w.cos = v.cos;
                        w.cos.erase(b);
                        w.cos.insert(a);
                        w.next = v.next;
                        w.target = v.target;
                        w.lead = v.lead;
                        v.state = VWorker::WAITING;
                        v.cos.clear();
                        break;
                    }
                }

            }
            while (!targets.empty() && has_free_worker) {
                busy = true;
                auto t = targets.front();
                targets.pop();
                if (t.ex - t.x == 1 && t.ez - t.z == 1) {
                    DLOG(INFO) << "assgign for single";
                    int tar = -1;
                    int mc = R*R;
                    REP(i,workers.size()) {
                        Point pos = workers[i].pos;
                        int cost = abs(pos.x - t.x) + abs(pos.z - t.z);
                        if (workers[i].state == VWorker::WAITING && cost < mc) {
                            tar = i;
                            mc = cost;
                        } else if (cost == 0 && workers[i].state == VWorker::BLOCKED) {
                            tar = -1;
                            break;
                        }
                    }
                    if (tar != -1) {
                        workers[tar].target = t;
                        workers[tar].dest = Point(t.x, j, t.z);
                        workers[tar].lead = true;
                        workers[tar].cos.clear();
                        workers[tar].state = VWorker::BLOCKED;
                        workers[tar].next = VWorker::READY;
                    } else {
                        unasigned.push(t);
                    }
                } else {
                    DLOG(INFO) << "assgign for gfill";
                    vector<P> peaks = {P(t.x, t.z), P(t.ex-1, t.z), P(t.ex-1, t.ez-1), P(t.x, t.ez-1)};
                    map<int,int> cows;
                    REP(k,4) {
                        int tar = -1;
                        int mc = R*R;
                        REP(i,workers.size()) {
                            Point pos = workers[i].pos;
                            int cost = abs(pos.x - peaks[k].first) + abs(pos.z - peaks[k].second);
                            if (workers[i].state == VWorker::WAITING && cost < mc && cows.find(i) == cows.end()) {
                                tar = i;
                                mc = cost;
                            }
                        }
                        if (tar == -1) {
                            DLOG(INFO) << "not found";
                            break;
                        }
                        cows[tar] = k;
                    }
                    if (cows.size() == 4) {
                        for (auto &p : cows) {
                            int tar = p.first;
                            int k = p.second;
                            DLOG(INFO) << "start assign" << tar << " " << k;
                            workers[tar].target = t;
                            workers[tar].dest = Point(peaks[k].first,j,peaks[k].second);
                            workers[tar].lead = k == 0;
                            workers[tar].cos.clear();
                            workers[tar].next = VWorker::READY;
                            if (k==0) {
                                for (auto &c : cows) {
                                    workers[tar].cos.insert(c.first);
                                }
                            }
                            tryReserve(workers[tar], varea); 
                        }
                    } else {
                        unasigned.push(t);
                    }
                }
            }
            while(!unasigned.empty()) {
                targets.push(unasigned.front());
                unasigned.pop();
            }

            for (auto &b : workers) {
                switch(b.state) {
                    case VWorker::WAITING:
                        DLOG(INFO) << " start sleeping " << b.id;
                        if(b.pos.y+1 < R && b.pos.y <= j && targets.empty()) {
                            b.pos.y++;
                            b.command_queue.push(Command::make_smove(b.id, dP[UP_Y]));
                            b.state = VWorker::SLEEPING;
                            varea.free(b.pos.x, b.pos.z);
                            busy = true;
                        } else if (targets.empty()) {
                            b.dest = Point(0, R-1, b.id - 1);
                            if (b.pos == b.dest) {
                                b.state = VWorker::SLEEPING;
                            } else {
                                b.next = VWorker::SLEEPING;
                                tryReserve(b,varea);
                                busy = true;
                            }
                        }
                        break;
                    case VWorker::BLOCKED:
                        tryReserve(b, varea); 
                        busy = true;
                        break;
                    case VWorker::MOVING:
                        moveReservedPath(b, varea);
                        busy = true;
                        break;
                    default:
                        break;
                }
            }
            for (auto &b: workers) {
                DLOG(INFO) << "vbots " << b.id << " state " << b.state << " pos " << b.pos;
                DCHECK(b.pos.y != j || varea.get(b.pos.x,b.pos.z) == b.id) << "on not reserved pos " << varea.get(b.pos.x,b.pos.z);
            }
            varea.runFree();
            for (auto &b: workers) {
                if (b.lead && b.state == b.READY) {
                    busy = true;
                    DLOG(INFO) << "check ready for other bot " << b.id;
                    bool flag = true;
                    for (int id: b.cos) {
                        flag &= workers[id].state == b.READY;
                    }
                    if (flag) {
                        if(b.cos.size() == 4) {
                            DLOG(INFO) << "exec gfill";
                            for (int id: b.cos) { 
                                auto &w = workers[id];
                                w.state = VWorker::WAITING;
                                int fdx = w.pos.x == w.target.x ? w.target.ex - w.target.x - 1 : w.target.x - w.target.ex + 1;
                                int fdz = w.pos.z == w.target.z ? w.target.ez - w.target.z - 1 : w.target.z - w.target.ez + 1;
                                DLOG(INFO) << "gfill "  << w.id << " " << fdx << " " << fdz;
                                w.command_queue.push(Command::make_gfill(w.id, dP[DOWN_Y], Point(fdx, 0, fdz)));
                            }
                            int color  = j-1 == 0 ? 0 : vox.add_color();
                            FOR(x,b.target.x, b.target.ex) FOR(z,b.target.z, b.target.ez) {
                                filled[color].insert(Point(x,j-1,z));
                            }
                            b.cos.clear();
                        } else {
                            DLOG(INFO) << "run single fill "  << b.id << " " << b.pos.x << " " << b.pos.z;
                            b.state = VWorker::WAITING;
                            b.command_queue.push(Command::make_fill(b.id, dP[DOWN_Y]));
                            filled[j-1 == 0 ? 0 : vox.add_color()].insert(Point(b.pos.x,j-1,b.pos.z));
                        }
                    }
                }
            }


            if (busy) {
                DLOG(INFO) << "running parallel";

                if (ce->GetSystemStatus().harmonics == Harmonics::LOW) {
                    bool found = false;
                    for (auto entry : filled) {
                        int color = entry.first;
                        Vox tmp = vox;
                        for (auto p : entry.second) {
                            tmp.set(true, p.x, p.y, p.z);
                            tmp.set_color(color, p.x, p.y, p.z);
                        }
                        for (auto p : entry.second) {
                            if(p.y!=0 && tmp.get_parent_color(p.x, p.y, p.z) != 0) {
                                found = true;
                                break;
                            }
                        }
                        if (found) break;
                    }
                    if (found) {
                        bool flag = false;
                        for (auto &b : workers) {
                            if (b.command_queue.empty()) {
                                b.command_queue.push(Command::make_flip(b.id));
                                flag = true;
                                break;
                            }
                        }
                        if (!flag) {
                            vector<Command> flips;
                            flips.push_back(Command::make_flip(workers[0].id));
                            FOR(i,1,workers.size()) {
                                flips.push_back(Command::make_wait(workers[i].id));
                            }
                            ce->Execute(flips);
                        }
                    }
                }
                for (auto &entry: filled) {
                    int color = entry.first;
                    for (Point p : entry.second) {
                        vox.set(true, p.x, p.y, p.z);
                        vox.set_color(color, p.x,p.y,p.z);
                        DCHECK(tvox.get(p.x, p.y, p.z)) << "invalid fill at " << p;
                    }
                }

                vector<Command> real_commands;
                for (auto &b : workers) {
                    if (b.command_queue.empty()) {
                        real_commands.push_back(Command::make_wait(b.id));
                    } else {
                        real_commands.push_back(b.command_queue.front());
                        b.command_queue.pop();
                    }
                }
                DCHECK(real_commands.size() == workers.size()) << "missing real command";
                ce->Execute(real_commands);
                for (auto &b: workers) {
                    DCHECK(b.pos == ce->GetBotStatus()[b.id].pos) << "failed to check vpos " << b.id;
                }
                if (ce->GetSystemStatus().harmonics == Harmonics::HIGH) {
                    DLOG(INFO) << "check state";
                    bool flag = false;
                    REP(i,R) REP(k,R) if(vox.get(i,j-1,k) && vox.get_parent_color(i,j-1,k) != 0){
                        flag = true;
                        break;
                    }
                    REP(i,R) REP(k,R) if(vox.get(i,j-2,k) && vox.get_parent_color(i,j-2,k) != 0){
                        flag = true;
                        break;
                    }
                    if (!flag) {
                        DLOG(INFO) << "SET LOW HARMONICS";
                        vector<Command> flips;
                        flips.push_back(Command::make_flip(workers[0].id));
                        FOR(i,1,workers.size()) {
                            flips.push_back(Command::make_wait(workers[i].id));
                        }
                        ce->Execute(flips);
                    }
                }

            }
        }
        DLOG(INFO) << "finished";
    }

    void parallel_fusion(vector<VWorker> &vbots) {
        DLOG(INFO) << "start fusion";
        int y = vbots[0].pos.y;
        VArea varea = VArea(R);

        sort(
            vbots.begin(),
            vbots.end(),
            [](const VWorker& a, const VWorker& b){return (a.pos.x == a.pos.x) ? (a.pos.z < b.pos.z) : (a.pos.x < b.pos.x);}
        );
        for (auto &b: vbots) {
            b.state = VWorker::BLOCKED;
            b.next = VWorker::READY;
            b.dest = Point(0, y, b.id-1);   
            varea.reserve(b.id, b.pos.x, b.pos.z);
        }

        bool busy = true;
        while(busy) {
            DLOG(INFO) << "going to fusion pos";
            busy = false;
            
            for (auto &b: vbots) {
                if(b.state==VWorker::BLOCKED) {
                    tryReserve(b, varea);
                }
            }

            for (auto &b: vbots) {
                if(b.state==VWorker::MOVING) {
                    busy = true;
                    moveReservedPath(b, varea);
                }
            }

            if (busy) {
                vector<Command> commands;
                for (auto &b: vbots) {
                    if (b.command_queue.empty()) {
                        commands.push_back(Command::make_wait(b.id));
                    } else {
                        commands.push_back(b.command_queue.front());
                        b.command_queue.pop();
                    }
                }
                ce->Execute(commands);
                varea.runFree();
            }
        }
        /*
        REP (i,vbots.size()) {
            set<Point> invalid;
            for (auto &bot:vbots) {
                invalid.insert(bot.pos);
            }
            vector<Command> bc = getPath(vbots[i].pos, Point(0,y,vbots[i].id-1), invalid);
            for (auto c : bc) {

                DLOG(INFO) << "try second move";
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
            vbots[i].pos = Point(0,y,vbots[i].id-1);
        }
        */
        int count = vbots.size();
        int div = 1;
        
        sort(vbots.begin(), vbots.end(),[](const VWorker& a, const VWorker& b){return a.pos.z < b.pos.z;});
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

    // fill y = j
    bool parallel_fill(vector<VWorker> &vbots, const int j) {
        DLOG(INFO) << "start filling";
        bvv visited = bvv(R,bv(R,0));
        queue<VTarget> targets;
        DBIT memo = DBIT(R,R);
        REP(i,R) REP(k,R) {
            memo.add(i,k, tvox.get(i,j,k));
        }


        for (int A=30;A >= 1; A--) for(int B=30; B >= 1; B--) FOR(i,1,R-A) FOR(k,1,R-B) {
            int count = memo.sum(i+A,k+B) + memo.sum(i-1,k-1) - memo.sum(i+A,k-1) - memo.sum(i-1,k+B);
            if (count == (A+1)*(B+1) && count >= 8) {
                DLOG(INFO) << "BOX fill area " <<  A << " " << B << " " << i << " " << k;
                targets.push(VTarget(i,k, i+A+1, k+B+1));
                FOR(x,i,i+A+1) FOR(z,k,k+B+1) {
                    memo.add(x,z, -1);
                    visited[x][z] = true;
                    DCHECK(tvox.get(x,j,z)) << "invalid point" << x << " " << z;
                }
                k += B;
            } else {
                k += max(0, ((A+1)*(B+1) - count)/ (A+1) -1);
            }
        }

        for (int i=0;i<R;i++) for(int k=0;k<R;k++) if(tvox.get(i,j,k) && !visited[i][k]) {
            DLOG(INFO) << "single fill area " << " " << i << " " << k;
            targets.push(VTarget(i,k,i+1,k+1));
        }
        if(targets.empty()) return false;
        run_parallel(vbots,j+1, targets);
        return true;
    }

    vector<VWorker> make_bots(int n) {
        vector<VWorker> vbots;
        vbots.push_back(VWorker(1));
        while(1) {
            vector<Command> commands;
            int count = vbots.size();
            REP(i,count) {
                DLOG(INFO) << "fission";
                int child_id = *(ce->GetBotStatus()[vbots[i].id].seeds.begin());
                commands.push_back(Command::make_fission(vbots[i].id, dP[UP_Z], n / count / 2 - 1));
                vbots.push_back(VWorker(child_id));
            }
            ce->Execute(commands);
            commands.clear();
            if ((int) vbots.size() >= n) break;
            int dist = (n / count / 2 -1);
            while (dist > 0) {
                int d  = min(dist, 15);
                REP(i, count) {
                    commands.push_back(Command::make_wait(vbots[i].id));
                    commands.push_back(Command::make_smove(vbots[i+count].id, dP[UP_Z] * d));
                }
                ce->Execute(commands);
                commands.clear();
                dist -= d;
            }
        }
        for (auto &b: vbots) {
            b.pos = ce->GetBotStatus()[b.id].pos;
        }
        return vbots;
    }

  public:
    Vox vvox;
    MakalovAI(const vvv &src_model, const vvv &tgt_model) : CrimeaAI(src_model, tgt_model), vvox(src_model) { };
    ~MakalovAI() override = default;

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
            num = 32;
        } else if (R > 20) {
            num = 16;
        } else if (R > 10) {
            num = 8;
        }

        LOG(INFO) << "R: " << R << " num:" << num;
        vector<VWorker> vbots = make_bots(num);
        
        vector<Command> cs;
        for (auto &bot: vbots) {
            cs.push_back(Command::make_smove(bot.id, Point(0,1,0)));
            bot.pos.y++;
        }
        ce->Execute(cs);
        for (int j=0;j<R-1;j++) {
            DLOG(INFO) << "start filling at " << j;
            if(!parallel_fill(vbots, j)) {
                break;
            }
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
    auto oscar_ai = std::make_unique<MakalovAI>(src_model, tgt_model);
    oscar_ai->Run();
    oscar_ai->Finalize();
}
