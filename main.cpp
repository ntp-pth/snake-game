#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include "game.h"
#include "records.h"

namespace {
using namespace snake;
constexpr UINT classicId = 101, mazeId = 102, speedId = 103, helpId = 104;
constexpr int boardX = 24, boardY = 76, cellSize = 25;
constexpr int clientWidth = 820, clientHeight = 610;
Game game;
Records records{};
std::filesystem::path savePath;
bool helpOpen = false, modal = false, saveFailed = false;
HFONT normalFont, titleFont, smallFont;
HMENU menu;

void loadRecords() {
    PWSTR local = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local))) {
        savePath = std::filesystem::path(local) / L"SnakeGame" / L"records.txt";
        CoTaskMemFree(local);
        std::ifstream file(savePath);
        records = readRecords(file);
    }
}
void updateRecord() {
    int& best = records[static_cast<int>(game.mode)];
    if (game.score <= best && !saveFailed) return;
    best = std::max(best, game.score);
    if (savePath.empty()) { saveFailed = true; return; }
    // Write beside the destination then replace it, preserving the old file on failure.
    std::error_code error;
    std::filesystem::create_directories(savePath.parent_path(), error);
    auto temporary = savePath;
    temporary += L".tmp";
    std::ofstream file(temporary, std::ios::trunc);
    writeRecords(file, records);
    file.close();
    saveFailed = error || !file || !MoveFileExW(temporary.c_str(), savePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}
void resetTimer(HWND window) {
    KillTimer(window, 1);
    SetTimer(window, 1, game.round().interval, nullptr);
}
void refreshMenu(HWND window) {
    for (int i = 0; i < 3; ++i) {
        std::wstring label = (i == static_cast<int>(game.mode) ? L"● " : L"") + configs()[i].name;
        ModifyMenuW(menu, classicId+i, MF_BYCOMMAND | MF_STRING, classicId+i, label.c_str());
    }
    CheckMenuRadioItem(menu, classicId, speedId, classicId+static_cast<int>(game.mode), MF_BYCOMMAND);
    DrawMenuBar(window);
}
void selectMode(HWND window, Mode mode) {
    if (modal) return;
    bool active = game.state == State::Playing || game.state == State::Paused || game.state == State::RoundClear;
    if (active) {
        modal = true;
        int answer = MessageBoxW(window, L"Đổi chế độ sẽ kết thúc ván hiện tại. Bắt đầu ván mới?", L"Chọn chế độ", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        modal = false;
        resetTimer(window);
        if (answer != IDYES) return;
    }
    helpOpen = false;
    game.restart(mode);
    refreshMenu(window);
    resetTimer(window);
    InvalidateRect(window, nullptr, FALSE);
}
void toggleHelp(HWND window) {
    if (modal) return;
    if (!helpOpen && game.state == State::Playing) game.state = State::Paused;
    helpOpen = !helpOpen;
    resetTimer(window);
    InvalidateRect(window, nullptr, FALSE);
}
void text(HDC dc, int x, int y, const std::wstring& value, COLORREF color = RGB(225,233,244), HFONT font = nullptr) {
    SelectObject(dc, font ? font : normalFont);
    SetTextColor(dc, color);
    TextOutW(dc, x, y, value.c_str(), static_cast<int>(value.size()));
}
void box(HDC dc, RECT r, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &r, brush); DeleteObject(brush);
}
void tile(HDC dc, Cell p, COLORREF color, int inset) {
    box(dc, {boardX+p.x*cellSize+inset, boardY+p.y*cellSize+inset,
             boardX+(p.x+1)*cellSize-inset, boardY+(p.y+1)*cellSize-inset}, color);
}
void paint(HDC dc) {
    SetBkMode(dc, TRANSPARENT);
    box(dc, {0,0,clientWidth,clientHeight}, RGB(15,23,38));
    text(dc, 24, 18, L"SNAKE", RGB(100,232,176), titleFont);
    text(dc, 192, 29, game.config().name + L"  •  20 × 20");
    box(dc, {boardX-2,boardY-2,boardX+502,boardY+502}, RGB(73,93,117));
    for (int y=0; y<size; ++y) for (int x=0; x<size; ++x)
        tile(dc, {x,y}, (x+y)%2 ? RGB(25,39,55) : RGB(29,44,60), 0);
    for (Cell wall : game.round().walls) {
        tile(dc, wall, RGB(151,132,196), 2);
        int x=boardX+wall.x*cellSize, y=boardY+wall.y*cellSize;
        box(dc, {x+6,y+6,x+19,y+9}, RGB(207,190,240));
    }
    if (inside(game.food)) {
        HBRUSH brush=CreateSolidBrush(RGB(255,106,105));
        HGDIOBJ old=SelectObject(dc,brush), pen=SelectObject(dc,GetStockObject(NULL_PEN));
        Ellipse(dc, boardX+game.food.x*cellSize+5,boardY+game.food.y*cellSize+5,
                boardX+game.food.x*cellSize+21,boardY+game.food.y*cellSize+21);
        SelectObject(dc,old); SelectObject(dc,pen); DeleteObject(brush);
    }
    for (std::size_t i=0; i<game.body.size(); ++i)
        tile(dc, game.body[i], i ? RGB(53,183,137) : RGB(132,255,191), 2);
    text(dc, 550, 80, L"ĐIỂM", RGB(154,174,198));
    text(dc, 550, 108, std::to_wstring(game.score), RGB(255,255,255), titleFont);
    text(dc, 550, 167, L"Kỷ lục chế độ", RGB(154,174,198));
    text(dc, 550, 196, std::to_wstring(records[static_cast<int>(game.mode)]), RGB(255,213,111), titleFont);
    text(dc, 550, 264, L"BẢN ĐỒ", RGB(154,174,198));
    text(dc, 550, 294, game.round().name);
    if (game.round().target) {
        text(dc, 550, 334, L"Vòng " + std::to_wstring(game.roundIndex+1) + L" / " + std::to_wstring(game.config().rounds.size()));
        text(dc, 550, 368, L"Thức ăn: " + std::to_wstring(game.eaten) + L" / " + std::to_wstring(game.round().target));
        text(dc, 550, 400, L"Còn cần: " + std::to_wstring(std::max(0,game.round().target-game.eaten)));
    } else {
        text(dc, 550, 334, L"Chơi liên tục");
        text(dc, 550, 368, L"Đã ăn: " + std::to_wstring(game.eaten));
    }
    text(dc, 550, 451, L"Nhịp: " + std::to_wstring(game.round().interval) + L" ms");
    text(dc, 550, 502, L"F1: Hướng dẫn", RGB(154,174,198));
    if (saveFailed) {
        text(dc, 550, 537, L"Chưa lưu được kỷ lục.", RGB(255,155,125), smallFont);
        text(dc, 550, 559, L"Sẽ thử lại; kiểm tra quyền ghi.", RGB(255,155,125), smallFont);
    }
    if (game.state != State::Playing) {
        box(dc, {49,232,499,430}, RGB(9,17,29));
        std::wstring title, detail, action;
        switch (game.state) {
        case State::Paused: title=L"Tạm dừng"; detail=L"Ván chơi được giữ nguyên"; action=L"Space / P để tiếp tục"; break;
        case State::RoundClear: title=L"Qua vòng!"; detail=L"Tổng điểm: " + std::to_wstring(game.score); action=L"Enter để bắt đầu vòng tiếp theo"; break;
        case State::Lost: title=L"Kết thúc ván"; detail=L"Tổng điểm: " + std::to_wstring(game.score); action=L"Enter để chơi lại chế độ này"; break;
        case State::Complete: title=L"Hoàn thành"; detail=L"Tổng điểm: " + std::to_wstring(game.score); action=L"Enter để chơi lại chế độ này"; break;
        default: break;
        }
        text(dc, 70, 250, title, RGB(255,213,111), titleFont);
        text(dc, 70, 305, detail);
        if (game.round().target) text(dc, 70, 337, L"Vòng đã đến: " + std::to_wstring(game.roundIndex+1));
        text(dc, 70, 383, action);
    }
    if (helpOpen) {
        box(dc, {16,12,804,593}, RGB(20,34,52));
        text(dc, 40, 30, L"Hướng dẫn", RGB(100,232,176), titleFont);
        const std::vector<std::wstring> lines{
            L"Di chuyển: WASD hoặc các phím mũi tên. Không quay ngược đầu.",
            L"Mỗi nhịp chỉ nhận một lần đổi hướng để tránh tự đâm vào thân.",
            L"Space / P: tạm dừng hoặc tiếp tục. R: chơi lại (có xác nhận giữa ván).",
            L"Enter: vòng tiếp theo; hoặc chơi lại sau Kết thúc ván / Hoàn thành.",
            L"Esc / X: thoát game (có xác nhận giữa ván).",
            L"F1 / Hướng dẫn: mở, đóng hướng dẫn. Esc cũng đóng hướng dẫn.",
            L"Sau khi đóng hướng dẫn, nhấn Space / P để tiếp tục ván đã tạm dừng.",
            L"Mỗi thức ăn được 10 điểm. Qua vòng giữ tổng điểm, đặt lại rắn dài 3 ô.",
            L"Đâm tường, thân rắn hoặc chướng ngại tím: kết thúc ván.",
            L"Cổ điển: bàn trống 20 × 20, nhịp 100 ms, chơi liên tục.",
            L"Mê cung: 3 bản đồ, mỗi vòng cần 5 / 6 / 7 thức ăn; nhịp 120 ms.",
            L"Tốc độ: 3 bản đồ thưa hơn; cần 5 / 6 / 7; nhịp 100 / 80 / 60 ms.",
            L"Đủ thức ăn: dừng chờ Enter. Xong vòng cuối: Hoàn thành.",
            L"Kỷ lục lưu riêng từng chế độ, cập nhật ngay khi vượt điểm cũ.",
            L"Alt+1 / Alt+2 / Alt+3: chọn chế độ. Đổi chế độ sẽ hỏi xác nhận."
        };
        for (std::size_t i=0; i<lines.size(); ++i) text(dc, 40, 90+static_cast<int>(i)*30, lines[i], RGB(225,233,244), smallFont);
        text(dc, 40, 554, L"F1 / Esc để đóng hướng dẫn", RGB(255,213,111));
    }
}
bool confirmRestart(HWND window) {
    bool active=game.state==State::Playing || game.state==State::Paused || game.state==State::RoundClear;
    if (!active) return true;
    modal=true;
    int answer=MessageBoxW(window,L"Kết thúc ván hiện tại và chơi lại?",L"Chơi lại",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);
    modal=false; resetTimer(window);
    return answer==IDYES;
}
LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        menu=CreateMenu();
        AppendMenuW(menu,MF_STRING,classicId,L"Cổ điển");
        AppendMenuW(menu,MF_STRING,mazeId,L"Mê cung");
        AppendMenuW(menu,MF_STRING,speedId,L"Tốc độ");
        AppendMenuW(menu,MF_STRING,helpId,L"Hướng dẫn");
        SetMenu(window,menu); refreshMenu(window); resetTimer(window); return 0;
    case WM_COMMAND:
        if (LOWORD(wParam)>=classicId && LOWORD(wParam)<=speedId) selectMode(window,static_cast<Mode>(LOWORD(wParam)-classicId));
        else if (LOWORD(wParam)==helpId) toggleHelp(window);
        return 0;
    case WM_ENTERMENULOOP: modal=true; return 0;
    case WM_EXITMENULOOP: modal=false; resetTimer(window); return 0;
    case WM_ACTIVATEAPP:
        if (!wParam && game.state==State::Playing) { game.state=State::Paused; InvalidateRect(window,nullptr,FALSE); }
        resetTimer(window); return 0;
    case WM_TIMER:
        if (!modal && !helpOpen) { game.tick(); updateRecord(); InvalidateRect(window,nullptr,FALSE); }
        return 0;
    case WM_SYSKEYDOWN:
        if (wParam>='1' && wParam<='3') { selectMode(window,static_cast<Mode>(wParam-'1')); return 0; }
        break;
    case WM_KEYDOWN:
        if (modal || (lParam & (1LL<<30))) return 0;
        if (wParam==VK_F1 || (helpOpen && wParam==VK_ESCAPE)) { toggleHelp(window); return 0; }
        if (helpOpen) return 0;
        switch (wParam) {
        case 'W': case VK_UP: game.turn({0,-1}); break;
        case 'S': case VK_DOWN: game.turn({0,1}); break;
        case 'A': case VK_LEFT: game.turn({-1,0}); break;
        case 'D': case VK_RIGHT: game.turn({1,0}); break;
        case VK_SPACE: case 'P': game.togglePause(); resetTimer(window); break;
        case VK_RETURN:
            if (game.state==State::RoundClear) game.nextRound();
            else if (game.state==State::Lost || game.state==State::Complete) game.restart(game.mode);
            resetTimer(window); break;
        case 'R': if (confirmRestart(window)) game.restart(game.mode); resetTimer(window); break;
        case VK_ESCAPE: case 'X': SendMessageW(window,WM_CLOSE,0,0); return 0;
        }
        InvalidateRect(window,nullptr,FALSE); return 0;
    case WM_CLOSE: {
        if (modal) return 0;
        bool active=game.state==State::Playing || game.state==State::Paused || game.state==State::RoundClear;
        modal=true;
        int answer=active ? MessageBoxW(window,L"Kết thúc ván và thoát game?",L"Thoát",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2) : IDYES;
        modal=false; resetTimer(window);
        if (answer==IDYES) DestroyWindow(window);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC target=BeginPaint(window,&ps);
        HDC buffer=CreateCompatibleDC(target);
        HBITMAP bitmap=CreateCompatibleBitmap(target,clientWidth,clientHeight);
        HGDIOBJ old=SelectObject(buffer,bitmap);
        paint(buffer);
        BitBlt(target,0,0,clientWidth,clientHeight,buffer,0,0,SRCCOPY);
        SelectObject(buffer,old); DeleteObject(bitmap); DeleteDC(buffer);
        EndPaint(window,&ps); return 0;
    }
    case WM_DESTROY: KillTimer(window,1); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window,message,wParam,lParam);
}
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDPIAware();
    loadRecords();
    normalFont=CreateFontW(-20,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    titleFont=CreateFontW(-32,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    smallFont=CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    WNDCLASSW cls{}; cls.lpfnWndProc=windowProc; cls.hInstance=instance; cls.lpszClassName=L"SnakeGameWindow";
    cls.hCursor=LoadCursor(nullptr,IDC_ARROW); cls.hIcon=LoadIcon(nullptr,IDI_APPLICATION);
    if (!RegisterClassW(&cls)) return 1;
    DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    RECT bounds{0,0,clientWidth,clientHeight}; AdjustWindowRect(&bounds,style,TRUE);
    HWND window=CreateWindowW(cls.lpszClassName,L"Snake — Cổ điển | Mê cung | Tốc độ",style,CW_USEDEFAULT,CW_USEDEFAULT,
                              bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nullptr,instance,nullptr);
    if (!window) return 1;
    ShowWindow(window,show); UpdateWindow(window);
    MSG message;
    while (GetMessageW(&message,nullptr,0,0)>0) { TranslateMessage(&message); DispatchMessageW(&message); }
    DeleteObject(normalFont); DeleteObject(titleFont); DeleteObject(smallFont);
    return static_cast<int>(message.wParam);
}
