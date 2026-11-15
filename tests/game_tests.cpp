#include "../game.h"
#include "../records.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
using namespace snake;
void require(bool result, const char* message) {
    if (!result) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
const std::array<Cell,4> directions{{{1,0},{0,1},{-1,0},{0,-1}}};
// Test driver uses BFS against walls and body to play real ticks to naturally spawned food.
bool goToFood(Game& g) {
    struct Visit { Cell p, first; };
    std::vector<Visit> queue{{g.body.front(), {0,0}}};
    bool seen[size][size]{};
    seen[g.body.front().y][g.body.front().x]=true;
    for (std::size_t i=0; i<queue.size(); ++i) {
        auto visit=queue[i];
        for (Cell d : directions) {
            if (i==0 && d==Cell{-g.direction.x,-g.direction.y}) continue;
            Cell p{visit.p.x+d.x, visit.p.y+d.y};
            if (!inside(p) || seen[p.y][p.x] || g.wall(p) || g.occupied(p)) continue;
            Cell first=i==0 ? d : visit.first;
            seen[p.y][p.x]=true;
            if (p==g.food) { g.turn(first); g.tick(); return true; }
            queue.push_back({p,first});
        }
    }
    return false;
}
void maps() {
    for (int m=0;m<3;++m) {
        Game g(1); g.restart(static_cast<Mode>(m));
        for (int r=0;r<static_cast<int>(g.config().rounds.size());++r) {
            g.roundIndex=r; g.startRound();
            bool walls[size][size]{};
            for (Cell w:g.round().walls) {
                require(inside(w) && !walls[w.y][w.x], "valid unique wall"); walls[w.y][w.x]=true;
            }
            for (Cell b:g.body) require(!g.wall(b),"safe body spawn");
            for (int x=11;x<16;++x) require(!g.wall({x,10}),"forward clearance");
            auto available=g.foodCandidates();
            require(available.size()==400-g.round().walls.size()-g.body.size(),"all empty cells connected at start");
            for (int i=0;i<1000;++i) {
                g.spawnFood();
                require(inside(g.food) && !g.wall(g.food) && !g.occupied(g.food),"food in empty cell");
                require(std::find(available.begin(),available.end(),g.food)!=available.end(),"reachable food");
            }
            if (m) require(g.round().target==r+5,"5/6/7 thresholds");
            if (m==2) require(g.round().interval==100-r*20,"speed increases");
        }
    }
}
void progression() {
    for (int m=1;m<=2;++m) {
        for (unsigned seed=1;seed<=30;++seed) {
            Game g(seed); g.restart(static_cast<Mode>(m));
            int steps=0;
            while (g.state!=State::Complete) {
                require(++steps<20000,"progress terminates");
                if (g.state==State::RoundClear) {
                    int score=g.score, previous=g.roundIndex;
                    g.nextRound();
                    require(g.roundIndex==previous+1 && g.score==score,"advance preserves score");
                    require(g.body.size()==3 && g.body.front()==Cell{10,10} && g.direction==Cell{1,0},"reset safe snake");
                    require(g.eaten==0,"reset round food count");
                } else {
                    require(g.state==State::Playing,"driver remains alive");
                    require(goToFood(g),"path to food available");
                }
            }
            require(g.score==180 && g.roundIndex==2 && g.eaten==7,"three rounds total 180");
            Cell head=g.body.front(); g.tick(); require(g.body.front()==head,"complete freezes");
            g.restart(g.mode);
            require(g.mode==static_cast<Mode>(m) && g.score==0 && g.roundIndex==0 && g.body.size()==3,"replay current mode from start");
        }
    }
}
void collisionsAndPause() {
    Game g(1);
    g.state=State::Paused; auto body=g.body; g.tick(); require(g.body==body,"pause freezes");
    g.togglePause(); require(g.state==State::Playing,"resume");
    g.turn({-1,0}); require(g.direction==Cell{1,0},"reverse rejected");
    g.turn({0,-1}); g.turn({-1,0}); require(g.direction==Cell{0,-1},"one turn per tick");
    g.restart(Mode::Maze); g.body={{5,5},{5,6},{5,7}}; g.direction={0,-1};
    g.tick(); require(g.state==State::Lost,"obstacle collision");
    require(g.roundIndex==0 && g.score==0,"loss preserves round and score");
    g.restart(Mode::Classic); g.body={{19,10},{18,10},{17,10}};
    g.tick(); require(g.state==State::Lost,"boundary collision");
    g.restart(Mode::Classic); g.body={{10,10},{11,10},{11,11},{10,11},{9,11}};
    g.tick(); require(g.state==State::Lost,"body collision");
    g.restart(Mode::Classic); g.body={{10,10},{10,11},{11,11},{11,10}};
    g.food={0,0}; g.tick(); require(g.state==State::Playing,"departing tail legal");
    g.restart(Mode::Classic);
    // A body enclosure must not allow food to spawn outside the head's reachable region.
    g.body={{10,10},{9,10},{9,9},{10,9},{11,9},{12,9},{12,10},{12,11},{11,11},{10,11},{9,11}};
    auto cells=g.foodCandidates(); require(cells.size()==1 && cells[0]==Cell{11,10},"body-aware reachability");
    g.spawnFood(); require(g.food==Cell{11,10},"spawn confined reachable cell");
    g.restart(Mode::Classic); g.food={11,10}; g.tick();
    require(g.score==10 && g.eaten==1 && g.body.size()==4 && g.state==State::Playing,"classic scoring growth continuous play");
}
void persistence() {
    std::istringstream missing(""); require(readRecords(missing)==Records{},"missing values zero");
    std::istringstream bad("classic nope\nmaze -5\nspeed 99999999999999999999\n");
    require(readRecords(bad)==Records{},"corrupt scores zero");
    std::istringstream mixed("classic 120\nmaze 40 junk\nspeed 180\nunknown 900\n");
    require(readRecords(mixed)==Records{{120,0,180}},"independent validation");
    auto path=std::filesystem::temp_directory_path()/"snake-game-records-test.txt";
    { std::ofstream out(path); writeRecords(out,Records{{30,100,180}}); require(bool(out),"write record file"); }
    { std::ifstream in(path); require(readRecords(in)==Records{{30,100,180}},"reload records after closing file"); }
    std::filesystem::remove(path);
}
int main() {
    maps(); collisionsAndPause(); progression(); persistence();
    std::cout << "PASS: 7 maps; 7000 food spawns; 60 full three-round games; collisions, pause, replay, records\n";
}
