$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force build | Out-Null
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -municode -mwindows -static main.cpp -o build/snake.exe -lgdi32 -luser32 -lshell32 -lole32 -luuid
if ($LASTEXITCODE -ne 0) { throw "Build thất bại" }
Write-Host "Build thành công: build/snake.exe"
