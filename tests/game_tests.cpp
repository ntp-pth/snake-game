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
            Cell p=g.destination(visit.p,d);
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
    for (int m=0;m<5;++m) {
        Game g(1); g.restart(static_cast<Mode>(m)); g.begin();
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
            if (m==1 || m==2) require(g.round().target==r+5,"5/6/7 thresholds");
            if (m==2) require(g.round().interval==100-r*20,"speed increases");
        }
    }
}
void progression() {
    for (int m=1;m<=2;++m) {
        for (unsigned seed=1;seed<=30;++seed) {
            Game g(seed); g.restart(static_cast<Mode>(m)); g.begin();
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
            g.restart(g.mode); g.begin();
            require(g.mode==static_cast<Mode>(m) && g.score==0 && g.roundIndex==0 && g.body.size()==3,"replay current mode from start");
        }
    }
}
void collisionsAndPause() {
    Game g(1); g.begin();
    g.state=State::Paused; auto body=g.body; g.tick(); require(g.body==body,"pause freezes");
    g.togglePause(); require(g.state==State::Playing,"resume");
    g.turn({-1,0}); require(g.direction==Cell{1,0},"reverse rejected");
    g.turn({0,-1}); g.turn({-1,0}); require(g.direction==Cell{0,-1},"one turn per tick");
    g.restart(Mode::Maze); g.begin(); g.body={{5,5},{5,6},{5,7}}; g.direction={0,-1};
    g.tick(); require(g.state==State::Lost,"obstacle collision");
    require(g.roundIndex==0 && g.score==0,"loss preserves round and score");
    g.restart(Mode::Classic); g.begin(); g.body={{19,10},{18,10},{17,10}};
    g.tick(); require(g.state==State::Lost,"boundary collision");
    g.restart(Mode::Classic); g.begin(); g.body={{10,10},{11,10},{11,11},{10,11},{9,11}};
    g.tick(); require(g.state==State::Lost,"body collision");
    g.restart(Mode::Classic); g.begin(); g.body={{10,10},{10,11},{11,11},{11,10}};
    g.food={0,0}; g.tick(); require(g.state==State::Playing,"departing tail legal");
    g.restart(Mode::Classic); g.begin();
    // A body enclosure must not allow food to spawn outside the head's reachable region.
    g.body={{10,10},{9,10},{9,9},{10,9},{11,9},{12,9},{12,10},{12,11},{11,11},{10,11},{9,11}};
    auto cells=g.foodCandidates(); require(cells.size()==1 && cells[0]==Cell{11,10},"body-aware reachability");
    g.spawnFood(); require(g.food==Cell{11,10},"spawn confined reachable cell");
    g.restart(Mode::Classic); g.begin(); g.food={11,10}; g.tick();
    require(g.score==10 && g.eaten==1 && g.body.size()==4 && g.state==State::Playing,"classic scoring growth continuous play");
}
void controlsAndWrapping() {
    Game g(42);
    for (int mode=0; mode<5; ++mode) {
        g.restart(static_cast<Mode>(mode));
        auto body=g.body; Cell food=g.food;
        for (int i=0;i<100;++i) g.tick();
        require(g.state==State::Ready && g.body==body && g.food==food,"no automatic start after selecting mode");
        g.resume(); g.togglePause(); g.turn({0,-1});
        require(g.state==State::Ready && g.direction==Cell{1,0},"resume and movement do not start ready game");
        g.begin(); require(g.state==State::Playing,"explicit start");
        g.pause(); g.pause(); g.tick();
        require(g.state==State::Paused && g.body==body,"stop is idempotent and freezes snake");
        g.begin(); require(g.state==State::Paused,"start does not restart a paused game");
        g.resume(); g.resume(); g.tick();
        require(g.state==State::Playing && g.body.front()==Cell{11,10},"resume keeps position direction");
        g.leaveMode(); body=g.body; g.tick(); g.begin(); g.resume(); g.togglePause();
        require(g.state==State::Lobby && g.body==body,"exit mode cannot resume abandoned game");
    }
    for (Mode mode : {Mode::WrapEdges,Mode::WrapCorners}) {
        for (Cell d:directions) {
            g.restart(mode); g.begin();
            // Both layouts have open portals on these edge positions.
            int lane=mode==Mode::WrapEdges ? 10 : 1;
            Cell head=d.x ? Cell{d.x>0 ? 19 : 0,lane} : Cell{lane,d.y>0 ? 19 : 0};
            g.body={head,{head.x-d.x,head.y-d.y},{head.x-2*d.x,head.y-2*d.y}};
            g.direction=d; Cell exit=g.destination(head,d); g.food=exit;
            g.tick();
            require(g.state==State::Playing && g.body.front()==exit,"wrap across each of four edges");
            require((d.x && exit.y==lane && exit.x==(d.x>0 ? 0 : 19)) ||
                    (d.y && exit.x==lane && exit.y==(d.y>0 ? 0 : 19)),"wrap preserves row or column");
            require(g.score==10 && g.body.size()==4,"eat at portal exit");
            g.body={head,{exit.x,exit.y},{10,10}}; g.direction=d; g.food={5,5}; g.tick();
            require(g.state==State::Lost,"body collision at opposite-edge exit");
        }
    }
    g.state=State::Lost; g.score=100; auto lastMode=g.mode;
    g.begin(); require(g.state==State::Playing && g.score==0 && g.mode==lastMode,"start replays lost game with mouse command");
    g.state=State::Complete; g.score=180;
    g.begin(); require(g.state==State::Playing && g.score==0 && g.roundIndex==0,"start replays completed game");
    g.restart(Mode::WrapEdges); g.begin();
    require(g.round().walls.size()==16,"four 2x2 corner blocks only");
    for (Cell c:g.round().walls) require((c.x<2 || c.x>=18) && (c.y<2 || c.y>=18),"walls confined to corners");
    g.body={{2,0},{3,0},{4,0}}; g.direction={-1,0}; g.tick();
    require(g.state==State::Lost,"corner obstacle collision");
    g.restart(Mode::WrapCorners); g.begin();
    require(g.round().walls.size()==64,"only middle edges blocked");
    g.body={{10,1},{10,2},{10,3}}; g.direction={0,-1}; g.tick();
    require(g.state==State::Lost,"closed middle edge collision");
    g.restart(Mode::WrapEdges); g.begin();
    // A full-height body barrier blocks normal access; the opposite edge remains reachable.
    g.body={{0,10}};
    for (int y=0;y<size;++y) g.body.push_back({1,y});
    auto cells=g.foodCandidates();
    require(std::find(cells.begin(),cells.end(),Cell{19,10})!=cells.end(),"food flood fill follows portals");
    g.restart(Mode::WrapCorners); g.begin();
    for (int i=0;i<300;++i) require(goToFood(g),"natural food reachable in wrap corner mode");
    require(g.state==State::Playing && g.score>0,"wrap mode remains continuous");
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
    std::istringstream legacy("classic 30\nmaze 100\nspeed 180\n");
    require(readRecords(legacy)==Records{{30,100,180,0,0}},"legacy three-mode records migrate without loss");
    std::stringstream all;
    writeRecords(all,Records{{10,20,30,40,50}});
    require(readRecords(all)==Records{{10,20,30,40,50}},"five independent record slots round trip");
    std::istringstream badNew("classic 30\nwrap_edges nope\nwrap_corners -4\n");
    require(readRecords(badNew)==Records{{30,0,0,0,0}},"corrupt new-mode records leave legacy intact");
}
int main() {
    maps(); collisionsAndPause(); progression(); controlsAndWrapping(); persistence();
    std::cout << "PASS: 9 maps; 9000 food spawns; 60 full three-round games; controls, four-way wrapping, collisions, replay, five-mode records\n";
}
