#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <deque>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "src/base/base.h"
#include "src/base/flags.h"
#include "src/command_util.h"
#include "src/command_executer.h"

#include "solver/AI.h"
#include "oscar.h"
#include "simple_solve.h"

using namespace std;

static int dx[] = {-1, 1, 0, 0, 0, 0};
static int dy[] = {0, 0, -1, 1, 0, 0};
static int dz[] = {0, 0, 0, 0, -1, 1};

#define DOWN_X 0
#define UP_X 1
#define DOWN_Y 2
#define UP_Y 3
#define DOWN_Z 4
#define UP_Z 5

class OscarAI : public AI
{
    vvv tgt_model;
    Vox vox;

  public:
    OscarAI(const vvv &src_model, const vvv &tgt_model) : AI(src_model), tgt_model(tgt_model), vox(src_model) { }
    ~OscarAI() override = default;

    void Run() override {
        vox.set_colors();

        priority_queue<pair<int, Point>, vector<pair<int, Point>>, greater<pair<int, Point>>> pque;

        const int R = tgt_model.size();

        for (size_t x = 0; x < tgt_model.size(); x++) {
            for (size_t z = 0; z < tgt_model[x][0].size(); z++) {
                if (tgt_model[x][0][z] == 1) {
                    pque.push(make_pair(x + z, Point(x, 0, z)));
                }
            }
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
    if (!FLAGS_src_filename.empty()) {
        src_model = ReadMDL(FLAGS_src_filename);
        R = src_model.size();
    }

    vvv tgt_model;
    if(!FLAGS_tgt_filename.empty()) {
        tgt_model = ReadMDL(FLAGS_tgt_filename);
        R = tgt_model.size();
    }

    if (FLAGS_src_filename.empty()) {
        LOG(INFO) << "Start with empty src";
        src_model = vvv(R, vv(R, v(R, 0)));
    }
    if (FLAGS_tgt_filename.empty()) {
        LOG(INFO) << "Start with empty tgt";
        tgt_model = vvv(R, vv(R, v(R, 0)));
    }
    LOG(INFO) << "R: " << R;
    auto oscar_ai = std::make_unique<OscarAI>(src_model, tgt_model);
    oscar_ai->Run();
    oscar_ai->Finalize();
}
