#pragma once
#include <algorithm>
#include <array>
#include <deque>
#include <random>
#include <string>
#include <vector>

namespace snake {
constexpr int size = 20;
struct Cell {
    int x, y;
    bool operator==(Cell b) const { return x == b.x && y == b.y; }
    bool operator!=(Cell b) const { return !(*this == b); }
};
inline bool inside(Cell c) { return c.x >= 0 && c.y >= 0 && c.x < size && c.y < size; }
enum class Mode { Classic, Maze, Speed, WrapEdges, WrapCorners };
enum class State { Ready, Playing, Paused, RoundClear, Lost, Complete, Lobby };
struct Round {
    std::wstring name;
    int interval, target;
    std::vector<Cell> walls;
};
struct ModeConfig {
    std::wstring name;
    std::vector<Round> rounds;
    bool wrap = false;
};
inline const std::array<ModeConfig, 5>& configs() {
    static const auto data = [] {
        std::array<ModeConfig, 5> c{{
            {L"Cổ điển", {{L"Đồng cỏ", 100, 0, {}}}},
            {L"Mê cung", {{L"Hai hàng rào", 120, 5, {}}, {L"Lối rẽ", 120, 6, {}}, {L"Đường zích zắc", 120, 7, {}}}},
            {L"Tốc độ", {{L"Khởi động", 100, 5, {}}, {L"Tăng tốc", 80, 6, {}}, {L"Bứt phá", 60, 7, {}}}},
            {L"Xuyên biên", {{L"Bốn góc chắn", 100, 0, {}}}, true},
            {L"Xuyên góc", {{L"Bốn góc mở", 100, 0, {}}}, true}
        }};
        auto line = [](Round& r, int x, int y, int dx, int dy, int n) {
            for (int i = 0; i < n; ++i) r.walls.push_back({x + dx*i, y + dy*i});
        };
        line(c[1].rounds[0], 5, 4, 1, 0, 10);
        line(c[1].rounds[0], 5, 15, 1, 0, 10);
        for (int x : {5, 10, 15}) {
            line(c[1].rounds[1], x, 3, 0, 1, 5);
            line(c[1].rounds[1], x, 12, 0, 1, 5);
        }
        line(c[1].rounds[2], 3, 4, 1, 0, 13);
        line(c[1].rounds[2], 6, 8, 1, 0, 12);
        line(c[1].rounds[2], 2, 13, 1, 0, 13);
        line(c[1].rounds[2], 6, 17, 1, 0, 12);
        line(c[2].rounds[0], 6, 5, 1, 0, 6);
        line(c[2].rounds[1], 5, 5, 0, 1, 4);
        line(c[2].rounds[1], 14, 12, 0, 1, 4);
        line(c[2].rounds[2], 3, 5, 1, 0, 5);
        line(c[2].rounds[2], 12, 14, 1, 0, 5);
        line(c[2].rounds[2], 15, 4, 0, 1, 4);
        for (int y=0; y<size; ++y) for (int x=0; x<size; ++x) {
            bool corner = (x<2 || x>=size-2) && (y<2 || y>=size-2);
            if (corner) c[3].rounds[0].walls.push_back({x,y});
            if ((x==0 || x==size-1 || y==0 || y==size-1) && !corner)
                c[4].rounds[0].walls.push_back({x,y});
        }
        return c;
    }();
    return data;
}
class Game {
    std::mt19937 random;
    bool turned = false;
public:
    Mode mode = Mode::Classic;
    State state = State::Ready;
    int roundIndex = 0, score = 0, eaten = 0;
    std::deque<Cell> body;
    Cell direction{1, 0}, food{-1, -1};
    explicit Game(unsigned seed = std::random_device{}()) : random(seed) { restart(mode); }
    const ModeConfig& config() const { return configs()[static_cast<int>(mode)]; }
    const Round& round() const { return config().rounds.at(roundIndex); }
    bool wall(Cell c) const { return std::find(round().walls.begin(), round().walls.end(), c) != round().walls.end(); }
    bool occupied(Cell c) const { return std::find(body.begin(), body.end(), c) != body.end(); }
    void startRound() {
        body = {{10, 10}, {9, 10}, {8, 10}};
        direction = {1, 0}; turned = false; eaten = 0;
        state = State::Ready;
        spawnFood();
    }
    void restart(Mode next) { mode = next; roundIndex = 0; score = 0; startRound(); }
    bool active() const { return state==State::Playing || state==State::Paused || state==State::RoundClear; }
    void begin() {
        if (state==State::Lost || state==State::Complete) restart(mode);
        if (state==State::Ready) state=State::Playing;
        else if (state==State::RoundClear) nextRound();
    }
    void pause() { if (state==State::Playing) state=State::Paused; }
    void resume() { if (state==State::Paused) state=State::Playing; }
    void leaveMode() { state=State::Lobby; }
    Cell destination(Cell p, Cell d) const {
        Cell next{p.x+d.x,p.y+d.y};
        if (config().wrap) { next.x=(next.x+size)%size; next.y=(next.y+size)%size; }
        return next;
    }
    void nextRound() {
        if (state == State::RoundClear) { ++roundIndex; startRound(); state=State::Playing; }
    }
    // Flood fill treats the current body as blocked; only reachable empty cells qualify.
    std::vector<Cell> foodCandidates() const {
        bool seen[size][size]{};
        std::vector<Cell> queue{body.front()}, result;
        seen[body.front().y][body.front().x] = true;
        for (std::size_t i = 0; i < queue.size(); ++i) {
            for (Cell d : {Cell{1,0}, Cell{-1,0}, Cell{0,1}, Cell{0,-1}}) {
                Cell p=destination(queue[i],d);
                if (!inside(p) || seen[p.y][p.x] || wall(p) || occupied(p)) continue;
                seen[p.y][p.x] = true;
                queue.push_back(p); result.push_back(p);
            }
        }
        return result;
    }
    void spawnFood() {
        auto cells = foodCandidates();
        if (cells.empty()) { food = {-1,-1}; state = State::Complete; return; }
        food = cells[std::uniform_int_distribution<std::size_t>(0, cells.size()-1)(random)];
    }
    void turn(Cell d) {
        if (state != State::Playing || turned || (d.x == -direction.x && d.y == -direction.y)) return;
        if (d != direction) { direction = d; turned = true; }
    }
    void togglePause() {
        if (state == State::Playing) state = State::Paused;
        else if (state == State::Paused) state = State::Playing;
    }
    void tick() {
        if (state != State::Playing) return;
        turned = false;
        Cell head=destination(body.front(),direction);
        bool eating = head == food;
        auto end = body.end();
        if (!eating) --end; // Moving into the departing tail is legal.
        if (!inside(head) || wall(head) || std::find(body.begin(), end, head) != end) {
            state = State::Lost; return;
        }
        body.push_front(head);
        if (!eating) { body.pop_back(); return; }
        score += 10; ++eaten;
        if (round().target && eaten >= round().target) {
            state = roundIndex + 1 == static_cast<int>(config().rounds.size()) ? State::Complete : State::RoundClear;
            food = {-1,-1};
        } else spawnFood();
    }
};
} // namespace snake
