# Snake — C++ / Win32

Game cửa sổ Win32, bàn 20 × 20, giao diện tiếng Việt. Thanh menu trên cùng theo thứ tự **Điều khiển ▾ | Cổ điển | Mê cung | Tốc độ | Xuyên biên | Xuyên góc | Hướng dẫn**; dấu **●** chỉ chế độ đang chọn. Bảng bên phải hiển thị tổng điểm, **Kỷ lục chế độ**, bản đồ và tiến độ vòng.

> Mã `main` tại thời điểm nâng cấp chỉ có bản console (`conio.h`, `Sleep(100)`), chưa chứa bản cửa sổ đã được mô tả. Bản này xây dựng giao diện Win32 từ đó, giữ kích thước 20 × 20, 100 ms của Cổ điển và 10 điểm/thức ăn. Không có cấu hình build GUI cũ trong repo để tái sử dụng.

## Build và chạy trên Windows (PowerShell)

Cần GCC/MinGW-w64 có C++17 và `g++` trong PATH (khuyến nghị GCC 9 trở lên). Không cần thư viện đồ họa ngoài Windows.

```powershell
git clone https://github.com/ntp-pth/snake-game.git
Set-Location snake-game
git switch feature/manual-controls-wrap
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

## Điều khiển bằng chuột và bắt đầu chủ động

**Mở game hoặc chọn chế độ: rắn đứng yên ở màn hình Sẵn sàng**, không tự chạy. Nhấn **Điều khiển ▾ → Bắt đầu** hoặc **Enter** để chơi.

Menu **Điều khiển ▾** có:

| Mục | Tác dụng |
| --- | --- |
| Bắt đầu | Bắt đầu từ Sẵn sàng; bắt đầu vòng kế tiếp sau Qua vòng; hoặc ván mới sau kết thúc |
| Dừng | Tạm dừng ván, giữ rắn, điểm, thức ăn |
| Tiếp tục | Tiếp tục ván đã tạm dừng, không đặt lại |
| Thoát chế độ | Hỏi xác nhận nếu lượt chưa kết thúc; quay về màn hình chọn chế độ, cửa sổ vẫn mở |
| Thoát game | Đóng cửa sổ, hỏi xác nhận nếu lượt chưa kết thúc |

Mục chưa dùng được ở trạng thái hiện tại sẽ được làm mờ. Muốn chơi lại sau kết thúc, chọn Bắt đầu để vào ván mới ngay; hoặc nhấn Enter/R để về Sẵn sàng rồi Bắt đầu. Sau khi thoát chế độ phải chọn một chế độ để tạo lượt mới; Tiếp tục không khôi phục lượt đã thoát.

## Luật chơi

- Rắn khởi đầu dài 3 ô tại giữa bàn, hướng sang phải. WASD và mũi tên đổi hướng; không được quay ngược đầu, mỗi nhịp nhận tối đa một lần đổi hướng.
- Mỗi thức ăn cộng **10 điểm**, rắn dài thêm một ô. Đâm thân hoặc chướng ngại vật tím sẽ **Kết thúc ván**. Đâm biên cũng kết thúc ở Cổ điển/Mê cung/Tốc độ; Xuyên biên/Xuyên góc có cổng đi qua cạnh (xem dưới). Có thể đi vào ô đuôi vừa rời đi.
- Thức ăn đỏ chỉ sinh ở ô trống mà đầu rắn có thể tiếp cận tại thời điểm sinh, tránh chướng ngại và toàn bộ thân rắn. Nếu không còn ô trống tiếp cận được, lượt chơi **Hoàn thành**.

| Chế độ | Bản đồ / vòng | Nhịp di chuyển | Điều kiện |
| --- | --- | --- | --- |
| Cổ điển | Đồng cỏ, không chướng ngại | 100 ms | Chơi liên tục |
| Mê cung | Hai hàng rào → Lối rẽ → Đường zích zắc | 120 ms mỗi vòng | 5 → 6 → 7 thức ăn |
| Tốc độ | Khởi động → Tăng tốc → Bứt phá | 100 → 80 → 60 ms | 5 → 6 → 7 thức ăn |
| Xuyên biên | Bốn góc chắn, mỗi góc 2 × 2 ô | 100 ms | Chơi liên tục, xuyên qua các cạnh còn trống |
| Xuyên góc | Các ô sát cạnh bị chắn, trừ vùng 2 × 2 ở mỗi góc | 100 ms | Chơi liên tục, chỉ đi xuyên qua phần góc còn trống |

Ở hai chế độ mới, đi qua trái xuất hiện bên phải **cùng hàng**, đi qua trên xuất hiện bên dưới **cùng cột** (và ngược lại). Khung xanh biểu thị bản đồ có xuyên biên; ô chướng ngại tím vẫn cấm đi. Ví dụ ở Xuyên biên, từ `(19, 10)` đi phải đến `(0, 10)`; ở Xuyên góc, từ `(19, 1)` đi phải đến `(0, 1)`. Nếu ô đến có thân rắn thì vẫn va chạm; không được đi xuyên qua chướng ngại. Thức ăn dùng cùng luật kết nối qua cạnh khi tìm vùng tiếp cận được.

Đủ thức ăn thì dừng ở thông báo **Qua vòng**, nhấn **Enter** để bắt đầu vòng mới. Tổng điểm được giữ; rắn trở về vị trí, chiều dài, hướng xuất phát an toàn. Hoàn thành vòng 3 được **180 điểm**, hiện **Hoàn thành**. Màn hình kết thúc hiển thị tổng điểm, vòng đã đến (nếu có). Enter ở màn hình kết thúc/hoàn thành đặt lại vòng đầu của chế độ hiện tại và chờ Bắt đầu.

Đổi chế độ hoặc chọn lại chế độ hiện tại khi đang trong lượt chơi sẽ hỏi xác nhận; chọn **No** giữ ván và trạng thái tạm dừng trước đó. Mở Hướng dẫn giữa ván tự tạm dừng. Đóng hướng dẫn vẫn giữ tạm dừng để người chơi chủ động tiếp tục. Khi chuyển sang ứng dụng khác, game cũng tự tạm dừng.

## Phím điều khiển

| Phím | Tác dụng |
| --- | --- |
| WASD / mũi tên | Di chuyển |
| Space / P | Tạm dừng, tiếp tục |
| Enter | Bắt đầu từ Sẵn sàng; qua vòng; đặt lại về Sẵn sàng sau kết thúc/hoàn thành |
| R | Đặt lại chế độ hiện tại về Sẵn sàng; hỏi xác nhận nếu lượt chưa kết thúc |
| F1 / mục Hướng dẫn | Mở, đóng hướng dẫn |
| Esc khi xem hướng dẫn | Đóng hướng dẫn |
| Esc / X khi chơi | Thoát, có xác nhận nếu lượt chưa kết thúc |
| Alt+1 / Alt+2 / Alt+3 / Alt+4 / Alt+5 | Chọn Cổ điển / Mê cung / Tốc độ / Xuyên biên / Xuyên góc |

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
wrap_edges 70
wrap_corners 50
```

Tệp từ bản cũ chỉ có ba dòng vẫn giữ nguyên kỷ lục ba chế độ đó; hai mục mới mặc định 0.

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

Kiểm tra 9 bản đồ: ô chướng ngại hợp lệ, vị trí xuất phát và 5 ô phía trước an toàn, mọi ô trống kết nối với đầu rắn. Kiểm tra 9.000 lần sinh thức ăn; một tình huống thân rắn ngăn vùng sinh; 60 lượt chơi bằng BFS tới thức ăn qua đủ ba vòng của Mê cung/Tốc độ; giữ điểm, đặt lại rắn, tổng 180, chơi lại đúng chế độ; va chạm biên/thân/chướng ngại; tạm dừng; khóa đổi hướng; đọc/ghi/đọc lại kỷ lục và dữ liệu hỏng. Thêm kiểm tra không tự chạy khi mở/chọn chế độ, Dừng/Tiếp tục/Thoát chế độ, xuyên bốn hướng ở hai bản đồ, giữ hàng/cột, ăn và va chạm ở điểm ra, flood fill qua biên, chuyển đổi tệp kỷ lục cũ sang năm mục.

## Kiểm tra trực tiếp cần thực hiện trên Windows

Trong Cloud đã chạy thành công bộ kiểm tra logic Linux và biên dịch chéo **PE32 GUI x86** bằng MinGW-w64 C++17, liên kết tĩnh. Cloud không có Windows/Wine nên chưa chạy cửa sổ `.exe`, hộp thoại, menu hay xác thực ghi vào LocalAppData thật trên Windows. Kết quả build Windows trong Actions xem riêng ở PR.

1. Build/chạy như trên (hoặc tải artifact `snake-win32`). Chờ vài giây: rắn không di chuyển. Chọn chế độ khác cũng phải đứng chờ. Bấm Điều khiển → Bắt đầu: rắn mới chạy.
2. Bấm Dừng, chờ, rồi Tiếp tục: giữ nguyên vị trí/điểm. Thử Thoát chế độ và hủy: ván giữ nguyên; đồng ý: về chọn chế độ, không đóng game. Thoát game và hủy/đồng ý phải hoạt động riêng. Kiểm tra các mục vô hiệu đúng trạng thái.
3. Chọn lần lượt năm chế độ bằng chuột và Alt+1 đến Alt+5. Kiểm tra dấu ●, kỷ lục riêng, nội dung menu và chữ Việt không bị cắt ở DPI của bạn.
4. Giữa ván mở Hướng dẫn: rắn dừng. Đóng bằng F1/menu/Esc: vẫn tạm dừng; Điều khiển → Tiếp tục hoặc Space/P mới chạy. Hướng dẫn lúc Sẵn sàng không tự bắt đầu khi đóng.
5. Xuyên biên: thử qua trái/phải ở hàng giữa, trên/dưới ở cột giữa; phải ra đúng hàng/cột đối diện. Thử đâm bốn góc. Xuyên góc: cạnh giữa phải bị chặn, chỉ các ô góc trống đi xuyên; thử hai hướng ở mỗi góc và va chạm thân tại điểm ra.
6. Mê cung/Tốc độ: ăn đủ 5, 6, 7, chờ Enter mỗi vòng; điểm 50/110/180, reset rắn an toàn. Vòng cuối Hoàn thành; Enter về Sẵn sàng vòng 1. Cổ điển vẫn không xuyên biên.
7. Tạo kỷ lục khác nhau ở năm chế độ, kiểm tra tệp ngay khi vượt điểm cũ; đóng/mở lại để đối chiếu. Tệp ba mục của bản trước phải giữ điểm cũ; hai mục mới dùng 0 đến khi có điểm.
8. Đóng game, sao lưu tệp rồi thử thiếu/rỗng, `classic abc`, `maze -10`, `wrap_edges nope`, `wrap_corners -4`. Game vẫn mở, mục lỗi bằng 0, mục hợp lệ giữ nguyên. Khôi phục bản sao sau khi thử.

## Cấu trúc

- `game.h`: logic thuần C++, cấu hình chế độ/bản đồ/vòng trong `configs()`; thêm vòng bằng dữ liệu, không sửa luồng tick.
- `records.h`: định dạng và kiểm tra dữ liệu kỷ lục.
- `main.cpp`: Win32/GDI, menu, phím, hộp thoại và ghi tệp LocalAppData.
- `tests/game_tests.cpp`: kiểm tra logic không phụ thuộc Win32.
