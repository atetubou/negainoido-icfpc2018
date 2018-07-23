#include <iostream>
#include <memory>
#include <queue>
#include <iomanip>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "json/json.h"

#include "solver/AI.h"
#include "src/command.h"
// #include "solver/square_delete.h"

#include "src/base/flags.h"

class SquareDeleterAI : public AI {
  vvv voxels;
  vvv dist_from_ground; // Actually, it's not a distance
  const int R;

    struct Panel {
    int lx;
    int hx;
    int lz;
    int hz;

    Panel() : lx(0), hx(0), lz(0), hz(0) {}
    Panel(int lx, int hx, int lz, int hz): lx(lx), hx(hx), lz(lz), hz(hz) {}
      
    Point botPoint(int bot_id) const {
      if (bot_id == 1) {
        return Point(lx, -1, lz - 1);
      } else if (bot_id == 2) {
        return Point(hx + 1, -1, lz);
      } else if (bot_id == 3) {
        return Point(hx, -1, hz + 1);
      } else if (bot_id == 4) {
        return Point(lx - 1, -1, hz);
      }
      LOG(FATAL) << "bot id should in [1, 4], but " << bot_id;
      return Point(lx, -1, hz);
    }

    friend std::ostream &operator<<(std::ostream &os, const Panel &panel) {
      return os << "(" << panel.lx << ", " << panel.hx << ", " << panel.lz << ", " << panel.hz << ")";
    }

    friend bool operator!=(const Panel &panel1, const Panel &panel2) {
      return panel1.lx != panel2.lx ||
        panel1.hx != panel2.hx ||
        panel1.lz != panel2.lz ||
        panel1.hz != panel2.hz;
    }
  };


public:  
  SquareDeleterAI(const vvv &src_model) : AI(src_model), voxels(src_model), R(src_model.size()) {
    ComputeDistanceFromGround();
    
  }

  ~SquareDeleterAI() override = default;

  void Run() override {
    const int R = voxels.size();
    const int maxy = ComputeMaxY();

    for (int y = 0; y < maxy; ) {
      int ny = std::min(y + 15, maxy);
      ce->Execute({Command::make_smove(1, Point(0, ny - y, 0))});
      y = ny;
    }

    // Fission 1
    ce->Execute({Command::make_fission(1, Point(1, 0, 0), 1)});

    const int square_size = std::min(15, (R - 1));
    ce->Execute({
        Command::make_wait(1),
          Command::make_smove(2, Point(square_size, 0, 0))
          });

    ce->Execute({
        Command::make_fission(1, Point(0, 0, 1), 0),
          Command::make_fission(2, Point(0, 0, 1), 0)
          });

    ce->Execute({
        Command::make_wait(1),
          Command::make_wait(2),
          Command::make_smove(3, Point(0, 0, square_size)),
          Command::make_smove(4, Point(0, 0, square_size))
          });

    // Change shape
    /*
      from
      xooox
      ooooo
      ooooo
      ooooo
      xooox

      to
      oxooo
      oooox
      ooooo
      xoooo
      oooxo
     */
    OutputBotPoints();
    ce->Execute({
        Command::make_smove(1, Point(1, 0, 0)),
        Command::make_smove(2, Point(0, 0, 1)),
          Command::make_smove(3, Point(-1, 0, 0)),
          Command::make_smove(4, Point(0, 0, -1))
      });
    
    auto panels = GetPanels(square_size);
    auto origin_panel = panels[0];
    auto current_panel = panels[0];
    for (int b = 1; b <= 4; b++) {
      const auto bots = ce->GetBotStatus();
      LOG_IF(FATAL, current_panel.botPoint(b).x != bots[b].pos.x)<< current_panel.botPoint(b).x << " " <<  bots[b].pos.x << " " << b;
      LOG_IF(FATAL, current_panel.botPoint(b).z != bots[b].pos.z)<< current_panel.botPoint(b).z << " " <<  bots[b].pos.z << " " << b;
    }
    
    {
      for (int y = maxy; y >= 1; y--) {
        if (isHighHarmonicsNeeded(y - 1, panels)) {
          if (ce->GetSystemStatus().harmonics == Harmonics::LOW) {
            ce->Execute({
                Command::make_flip(1),
                  Command::make_wait(2),
                  Command::make_wait(3),
                  Command::make_wait(4)});
          }
        } else {
          if (ce->GetSystemStatus().harmonics == Harmonics::HIGH) {
            ce->Execute({
                Command::make_flip(1),
                  Command::make_wait(2),
                  Command::make_wait(3),
                  Command::make_wait(4)});
          }          
        }
        
        for (const auto& panel : panels) {
          if (!IsAllEmpty(y - 1, panel)) {
            HorizontalMove(current_panel, panel, square_size, panels);
            current_panel = panel;
            GVoids(panel, square_size);
          }
        }

        ce->Execute(SMoves(Point(0, -1, 0)));
        std::reverse(panels.begin(), panels.end());
      }
    }
    HorizontalMove(current_panel, origin_panel, square_size, panels);
    OutputBotPoints();
    
    // Revert shape
    ce->Execute({
        Command::make_smove(1, Point(-1, 0, 0)),
        Command::make_smove(2, Point(0, 0, -1)),
          Command::make_smove(3, Point(1, 0, 0)),
          Command::make_smove(4, Point(0, 0, 1))
          });
    OutputBotPoints();

    // Fussion
    ce->Execute({
        Command::make_wait(1),
          Command::make_wait(2),
          Command::make_smove(3, Point(0, 0, -square_size)),
          Command::make_smove(4, Point(0, 0, -square_size))
          });

    OutputBotPoints();

    ce->Execute({
        Command::make_fusion_p(1, Point(0, 0, 1)),
          Command::make_fusion_p(2, Point(0, 0, 1)),
          Command::make_fusion_s(3, Point(0, 0, -1)),
          Command::make_fusion_s(4, Point(0, 0, -1)),
          });

    ce->Execute({
        Command::make_wait(1),
          Command::make_smove(2, Point(-square_size, 0, 0))
          });
    
    ce->Execute({
        Command::make_fusion_p(1, Point(1, 0, 0)),
          Command::make_fusion_s(2, Point(-1, 0, 0))
          });

    // Move to the origin
    auto bots = ce->GetBotStatus();

    while (bots[1].pos.x > 0) {
      const int dx = std::min(bots[1].pos.x, 15);
      ce->Execute({Command::make_smove(1, Point(-dx, 0, 0))});
    }
    while (bots[1].pos.z > 0) {
      const int dz = std::min(bots[1].pos.z, 15);
      ce->Execute({Command::make_smove(1, Point(0, 0, -dz))});
    }
    ce->Execute({Command::make_halt(1)});
  }

  
private:

  void ComputeDistanceFromGround() {
    const int R = voxels.size();
    const int INF = 1E9;
    dist_from_ground = vvv(R, vv(R, v(R, INF)));

    std::queue<int> quex, quey, quez;
    for (int x = 0; x < R; x++) {
      for (int z = 0; z < R; z++) {
        if (voxels[x][0][z]) {
          dist_from_ground[x][0][z] = 0;
          quex.push(x);
          quey.push(0);
          quez.push(z);
        }
      }
    }

    const int dx[] = {-1,0,0,1,0,0};
    const int dy[] = {0,-1,0,0,1,0};
    const int dz[] = {0,0,-1,0,0,1};

    while (!quex.empty()) {
      int cx = quex.front(); quex.pop();
      int cy = quey.front(); quey.pop();
      int cz = quez.front(); quez.pop();
      int cd = dist_from_ground[cx][cy][cz];
      LOG_IF(FATAL, cd < cy);
      for (int k = 0; k < 6; k++) {
        int nx = cx + dx[k];
        int ny = cy + dy[k];
        int nz = cz + dz[k];
        int nd = std::max(dist_from_ground[cx][cy][cz], ny);
        if (0 <= nx && nx < R && 0 <= ny && ny < R && 0 <= nz && nz < R &&
            voxels[nx][ny][nz] && dist_from_ground[nx][ny][nz] > nd) {
          dist_from_ground[nx][ny][nz] = nd;
          quex.push(nx);
          quey.push(ny);
          quez.push(nz);
        }
      }
    }
  }
  
  void OutputBotPoints() {
    // const auto bots = ce->GetBotStatus();
    // for (int b = 1; b <= CommandExecuter::kMaxNumBots; b++) {
    //   if (bots[b].active) {
    //     LOG(INFO) << "Bot " << b << ": " << bots[b].pos;
    //   }
    // }
  }

  std::vector<Command> SMoves(const Point &ld) {
    return {
      Command::make_smove(1, ld),
        Command::make_smove(2, ld),
        Command::make_smove(3, ld),
        Command::make_smove(4, ld)
        };
  }

  void GVoids(const Panel &panel, const int square_size) {
    /*
      oxooo
      oooox
      ooooo
      xoooo
      oooxo
     */
    const Point p1 = panel.botPoint(1) + Point(0, -1, 1);
    const Point p2 = panel.botPoint(2) + Point(-1, -1, 0);
    const Point p3 = panel.botPoint(3) + Point(0, -1, -1);
    const Point p4 = panel.botPoint(4) + Point(1, -1, 0);
    OutputBotPoints();
    ce->Execute({
        Command::make_gvoid(1, Point(0, -1, 1), p3 - p1),
          Command::make_gvoid(2, Point(-1, -1, 0), p4 - p2),
          Command::make_gvoid(3, Point(0, -1, -1), p1 - p3),
          Command::make_gvoid(4, Point(1, -1, 0), p2 - p4)
          });
    
  }

  int ComputeMaxY() {
    int maxy = 0;
    for (int y = R - 1; y >= 0; --y) {
      for (int x = 0; x < R; ++x) {
        for (int z = 0; z < R; ++z) {
          if (voxels[x][y][z]) {
            maxy = y + 1;
            break;
          }
        }
      }
      if (maxy) {
        break;
      }
    }
    return maxy;
  }

  bool isHighHarmonicsNeeded(int y, const std::vector<Panel> &panels) {
    if (y == 0) {
      return false;
    }
    
    const int dx[] = {0, 1, 0, -1};
    const int dz[] = {1, 0, -1, 0};
    std::vector<std::vector<char>> visited(voxels.size(), std::vector<char>(voxels.size(), 0));
    for (const auto &panel: panels) {
      const int lx = panel.lx;
      const int lz = panel.lz;
      const int hx = panel.hx;
      const int hz = panel.hz;
      for (int x = lx; x <= hx; x++) {
        for (int z = lz; z <= hz; z++) {
          if (!visited[x][z] && voxels[x][y][z]) {
            bool ok = false;
            std::queue<int> quex;
            std::queue<int> quez;
            quex.push(x);
            quez.push(z);
            while (!quex.empty()) {
              int cx = quex.front(); quex.pop();
              int cz = quez.front(); quez.pop();
              if (dist_from_ground[cx][y][cz] == dist_from_ground[cx][y - 1][cz] + 1) {
                ok = true;
              }
              
              if (dist_from_ground[cx][y][cz] == dist_from_ground[cx][y - 1][cz]) {
                ok = false;
                break;
              }

              
              for (int i= 0; i < 4; i++) {
                int nx = cx + dx[i];
                int nz = cz + dz[i];
                if (lx <= nx && nx <= hx && lz <= nz && nz <= hz && voxels[nx][y][nz] && !visited[nx][nz]) {
                  visited[nx][nz]= 1;
                  quex.push(nx);
                  quez.push(nz);
                }
              }
            }
            if (!ok) {
              return true;
            }
          }
        }
      }
    }
    return false;
  }

  bool IsAllEmpty(int y, const Panel &panel) {
    for (int x = panel.lx; x <= panel.hx; ++x) {
      for (int z = panel.lz; z <= panel.hz; ++z) {
        if (voxels[x][y][z]) {
          return false;
        }
      }
    }
    return true;
  }


  std::vector<Panel> GetPanels(const int square_size) {
    const int R = voxels.size();

    std::vector<Panel> panels;
    auto checkRange = [R](int lx, int lz) {
      return
      1 <= lx && lx < R - 1  &&
      1 <= lz && lz < R - 1;
    };

    int lx = 1;
    int lz = 1;
      
    int dx[] = {1, 0, -1, 0};
    int dz[] = {0, 1, 0, 1};
    int didx = 0;

    panels.push_back(Panel(1, square_size, 1, square_size));
    while (true) {
      // LOG(INFO) << lx << " " << lz << " " << square_size ; // 
      
      if (!checkRange(lx + dx[didx] * square_size,
                      lz + dz[didx] * square_size)) {
        if (dz[didx] == 1) {
          break;
        } else {
          didx = (didx + 1) % 4;
          continue;
        }
      }

      lx += dx[didx] * square_size;
      lz += dz[didx] * square_size;
      int next_hx = std::min(lx + square_size - 1, R - 2);
      int next_hz = std::min(lz + square_size - 1, R - 2);
      panels.emplace_back(Panel(lx, next_hx, lz, next_hz));

      if (dz[didx] == 1) {
        didx = (didx + 1) % 4;
      }
    }
    return panels;
  }

  void HorizontalMove(Panel src_panel, Panel dst_panel, const int square_size, const std::vector<Panel> &panels) {
    while (src_panel.lx < dst_panel.lx) {
      int next_lx = src_panel.lx + square_size;
      int next_lz = src_panel.lz;
      const auto &next_panel = FindPanel(panels, next_lx, next_lz);
      src_panel = PanelMove(src_panel, next_panel);
    }

    while (src_panel.lx > dst_panel.lx) {
      int next_lx = src_panel.lx - square_size;;
      int next_lz = src_panel.lz;
      const auto &next_panel = FindPanel(panels, next_lx, next_lz);
      src_panel = PanelMove(src_panel, next_panel);
    }

    while (src_panel.lz < dst_panel.lz) {
      int next_lx = src_panel.lx;
      int next_lz = src_panel.lz + square_size;
      const auto &next_panel = FindPanel(panels, next_lx, next_lz);
      src_panel = PanelMove(src_panel, next_panel);
    }

    while (src_panel.lz > dst_panel.lz) {
      int next_lx = src_panel.lx;
      int next_lz = src_panel.lz - square_size;
      const auto &next_panel = FindPanel(panels, next_lx, next_lz);
      src_panel = PanelMove(src_panel, next_panel);
    }
  }

  Panel PanelMove(const Panel &curr_panel, const Panel &next_panel) {
    ce->Execute({
        Command::make_smove(1, next_panel.botPoint(1) - curr_panel.botPoint(1)),
          Command::make_smove(2, next_panel.botPoint(2) - curr_panel.botPoint(2)),
          Command::make_smove(3, next_panel.botPoint(3) - curr_panel.botPoint(3)),
          Command::make_smove(4, next_panel.botPoint(4) - curr_panel.botPoint(4))
          });
    return next_panel;
  }

  Panel FindPanel(const std::vector<Panel> &panels, int next_lx, int next_lz) {
    for (const auto &panel : panels) {
      if (panel.lx == next_lx && panel.lz == next_lz) {
        return panel;
      }
    }

    LOG(FATAL) << " panel should be detected";
    return panels[0];
  }
  
};

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const vvv voxels = ReadMDL(FLAGS_src_filename);
  auto square_deleter = std::make_unique<SquareDeleterAI>(voxels);
  square_deleter->Run();
  square_deleter->Finalize();
  return 0;
}
