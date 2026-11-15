# Snake — C++ / Win32

Game cửa sổ Win32, bàn 20 × 20, giao diện tiếng Việt. Thanh menu trên cùng theo thứ tự **Cổ điển | Mê cung | Tốc độ | Hướng dẫn**; dấu **●** chỉ chế độ đang chọn. Bảng bên phải hiển thị tổng điểm, **Kỷ lục chế độ**, bản đồ và tiến độ vòng.

> Mã `main` tại thời điểm nâng cấp chỉ có bản console (`conio.h`, `Sleep(100)`), chưa chứa bản cửa sổ đã được mô tả. Bản này xây dựng giao diện Win32 từ đó, giữ kích thước 20 × 20, 100 ms của Cổ điển và 10 điểm/thức ăn. Không có cấu hình build GUI cũ trong repo để tái sử dụng.

## Build và chạy trên Windows (PowerShell)

Cần GCC/MinGW-w64 có C++17 và `g++` trong PATH (khuyến nghị GCC 9 trở lên). Không cần thư viện đồ họa ngoài Windows.

```powershell
git clone https://github.com/ntp-pth/snake-game.git
Set-Location snake-game
git switch feature/win32-modes-records
.\build.ps1
.\build\snake.exe
```

Nếu PowerShell chặn script, dùng trực tiếp:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -municode -mwindows -static main.cpp -o build/snake.exe -lgdi32 -luser32 -lshell32 -lole32 -luuid
if ($LASTEXITCODE -eq 0) { .\build\snake.exe }
```

Hoặc trong **Developer PowerShell for VS 2022** có workload Desktop development with C++ (chọn môi trường x86 để tạo Win32):

```powershell
New-Item -ItemType Directory -Force build | Out-Null
cl /nologo /std:c++17 /EHsc /utf-8 /W4 /O2 main.cpp /Fo:build\main.obj /Fe:build\snake.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib shell32.lib ole32.lib uuid.lib
if ($LASTEXITCODE -eq 0) { .\build\snake.exe }
```

Workflow `Windows build and logic tests` build x86 trên Windows bằng MSVC, chạy kiểm tra logic và xuất artifact `snake-win32` chứa `snake.exe`. Workflow không kiểm tra giao diện bằng mắt.

## Luật chơi

- Rắn khởi đầu dài 3 ô tại giữa bàn, hướng sang phải. WASD và mũi tên đổi hướng; không được quay ngược đầu, mỗi nhịp nhận tối đa một lần đổi hướng.
- Mỗi thức ăn cộng **10 điểm**, rắn dài thêm một ô. Đâm biên, thân hoặc chướng ngại vật tím sẽ **Kết thúc ván**. Có thể đi vào ô đuôi vừa rời đi.
- Thức ăn đỏ chỉ sinh ở ô trống mà đầu rắn có thể tiếp cận tại thời điểm sinh, tránh chướng ngại và toàn bộ thân rắn. Nếu không còn ô trống tiếp cận được, lượt chơi **Hoàn thành**.

| Chế độ | Bản đồ / vòng | Nhịp di chuyển | Điều kiện |
| --- | --- | --- | --- |
| Cổ điển | Đồng cỏ, không chướng ngại | 100 ms | Chơi liên tục |
| Mê cung | Hai hàng rào → Lối rẽ → Đường zích zắc | 120 ms mỗi vòng | 5 → 6 → 7 thức ăn |
| Tốc độ | Khởi động → Tăng tốc → Bứt phá | 100 → 80 → 60 ms | 5 → 6 → 7 thức ăn |

Đủ thức ăn thì dừng ở thông báo **Qua vòng**, nhấn **Enter** để bắt đầu vòng mới. Tổng điểm được giữ; rắn trở về vị trí, chiều dài, hướng xuất phát an toàn. Hoàn thành vòng 3 được **180 điểm**, hiện **Hoàn thành**. Màn hình kết thúc hiển thị tổng điểm, vòng đã đến (nếu có). Enter ở màn hình kết thúc/hoàn thành chơi lại từ vòng đầu của chế độ hiện tại.

Đổi chế độ hoặc chọn lại chế độ hiện tại khi đang trong lượt chơi sẽ hỏi xác nhận; chọn **No** giữ ván và trạng thái tạm dừng trước đó. Mở Hướng dẫn giữa ván tự tạm dừng. Đóng hướng dẫn vẫn giữ tạm dừng để người chơi chủ động tiếp tục. Khi chuyển sang ứng dụng khác, game cũng tự tạm dừng.

## Phím điều khiển

| Phím | Tác dụng |
| --- | --- |
| WASD / mũi tên | Di chuyển |
| Space / P | Tạm dừng, tiếp tục |
| Enter | Qua vòng; chơi lại sau kết thúc/hoàn thành |
| R | Chơi lại chế độ hiện tại; hỏi xác nhận nếu lượt chưa kết thúc |
| F1 / mục Hướng dẫn | Mở, đóng hướng dẫn |
| Esc khi xem hướng dẫn | Đóng hướng dẫn |
| Esc / X khi chơi | Thoát, có xác nhận nếu lượt chưa kết thúc |
| Alt+1 / Alt+2 / Alt+3 | Chọn Cổ điển / Mê cung / Tốc độ |

Các mục menu dùng được bằng chuột và điều hướng menu chuẩn của Windows (Alt, mũi tên, Enter).

## Kỷ lục cục bộ

Lưu riêng theo chế độ tại **`%LOCALAPPDATA%\SnakeGame\records.txt`**, không phụ thuộc thư mục đang chạy. Trong PowerShell:

```powershell
Get-Content "$env:LOCALAPPDATA\SnakeGame\records.txt"
```

Định dạng văn bản:

```text
classic 120
maze 180
speed 90
```

Điểm phải là số nguyên không âm trong phạm vi `int`. Thiếu tệp, thiếu mục hoặc mục lỗi dùng `0`; mục hợp lệ khác vẫn đọc bình thường. Dòng không nhận biết được bỏ qua; nếu lặp khóa thì dùng dòng cuối cùng. Vượt kỷ lục sẽ ghi ngay, không đợi kết thúc lượt. Game ghi tệp tạm rồi thay thế tệp chính. Nếu không thể ghi, vẫn chơi và giữ kỷ lục trong bộ nhớ, hiển thị cảnh báo ở bảng điểm, thử ghi lại trong ván. Không có dữ liệu nào gửi qua mạng.

## Kiểm tra tự động

Trên Windows PowerShell với MinGW:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic tests/game_tests.cpp -o build/game_tests.exe
if ($LASTEXITCODE -eq 0) { .\build\game_tests.exe }
```

Trên Linux:

```bash
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic tests/game_tests.cpp -o build/game_tests
./build/game_tests
```

Kiểm tra 7 bản đồ: ô chướng ngại hợp lệ, vị trí xuất phát và 5 ô phía trước an toàn, mọi ô trống kết nối với đầu rắn. Kiểm tra 7.000 lần sinh thức ăn; một tình huống thân rắn ngăn vùng sinh; 60 lượt chơi bằng BFS tới thức ăn qua đủ ba vòng của Mê cung/Tốc độ; giữ điểm, đặt lại rắn, tổng 180, chơi lại đúng chế độ; va chạm biên/thân/chướng ngại; tạm dừng; khóa đổi hướng; đọc/ghi/đọc lại kỷ lục và dữ liệu hỏng.

## Kiểm tra trực tiếp cần thực hiện trên Windows

Trong Cloud đã chạy thành công bộ kiểm tra logic Linux và biên dịch chéo **PE32 GUI x86** bằng MinGW-w64 C++17, liên kết tĩnh. Cloud không có Windows/Wine nên chưa chạy cửa sổ `.exe`, hộp thoại, menu hay xác thực ghi vào LocalAppData thật trên Windows. Kết quả build Windows trong Actions xem riêng ở PR.

1. Build/chạy như trên (hoặc tải artifact `snake-win32` của Actions). Kiểm tra chữ Việt, menu không bị cắt, dấu ●, màu tím của chướng ngại, tổng điểm và bảng tiến độ ở DPI màn hình của bạn.
2. Chọn lần lượt cả 3 mục chế độ bằng chuột, Alt+1/2/3 và menu bằng bàn phím. Chọn No khi được hỏi: ván phải tiếp tục nguyên trạng; chọn Yes: về vòng 1, điểm 0, đúng kỷ lục chế độ.
3. Giữa ván mở Hướng dẫn: rắn dừng. Đóng bằng F1/menu/Esc: vẫn tạm dừng, chỉ Space/P mới tiếp tục. Kiểm tra WASD, mũi tên, R, Enter, Esc/X và hủy xác nhận.
4. Đâm vào chướng ngại ở mỗi chế độ có vòng: Kết thúc ván, đúng tổng điểm/vòng; Enter chơi lại cùng chế độ. Quan sát thức ăn luôn khác tường/thân và có đường tới tại thời điểm sinh.
5. Ăn đủ 5, 6, 7 thức ăn ở Mê cung và Tốc độ: mỗi mốc chờ Enter, rắn reset an toàn, điểm lần lượt 50/110/180. Vòng cuối Hoàn thành; Enter về vòng 1. Quan sát Tốc độ nhanh dần.
6. Tạo kỷ lục khác nhau ở ba chế độ, kiểm tra tệp ngay khi vượt điểm cũ; đóng/mở lại và đổi chế độ để đối chiếu. Không cần kết thúc ván mới thấy dữ liệu được ghi.
7. Đóng game, sao lưu tệp kỷ lục rồi thử thiếu tệp, tệp rỗng, `classic abc`, `maze -10`, `speed 99999999999999999999`. Mở game không được lỗi, các mục tương ứng bằng 0; thử một mục hợp lệ cùng các mục lỗi. Khôi phục bản sao sau khi thử.

## Cấu trúc

- `game.h`: logic thuần C++, cấu hình chế độ/bản đồ/vòng trong `configs()`; thêm vòng bằng dữ liệu, không sửa luồng tick.
- `records.h`: định dạng và kiểm tra dữ liệu kỷ lục.
- `main.cpp`: Win32/GDI, menu, phím, hộp thoại và ghi tệp LocalAppData.
- `tests/game_tests.cpp`: kiểm tra logic không phụ thuộc Win32.
