# Minecraft Server Manager (Qt6 UI + Native Win32 Process Backend)

Triển khai theo bản đặc tả kỹ thuật (`Minecraft_Server_Manager___Technical_Specification.md`),
phạm vi hiện tại là **Phase 1 — Core MVP**, với một thay đổi so với bản đầu: **lớp giao diện (UI)
dùng Qt6 Widgets** theo yêu cầu, thay vì Win32 control thuần. **Lớp quản lý tiến trình Minecraft
vẫn giữ nguyên Win32 API gốc** (CreateProcessW, Job Object, pipe...) như spec yêu cầu — Qt chỉ
thay thế phần hiển thị.

## Đã sửa/thêm ở bản này

- **UI làm lại đẹp hơn**: theme tối (dark theme) áp dụng toàn cục qua QSS, style
  `Fusion`, nút bấm có màu theo hành động (Start xanh lá, Stop đỏ, Restart vàng cam, Gửi xanh
  dương), danh sách server có hover/selection rõ ràng, có thanh chia `QSplitter` kéo được giữa
  2 panel, nhãn trạng thái đổi màu theo state (●  Running màu xanh, Crashed màu đỏ...).
- **Phase 2 — Process Monitoring (đầy đủ)**: `monitor::ProcessMonitor` lấy mẫu CPU%/RAM của tiến
  trình java.exe mỗi 2 giây (dùng `GetProcessTimes` + `K32GetProcessMemoryInfo`, thuần Win32,
  không thêm thư viện ngoài). **TPS**: tự động gửi lệnh `/tps` mỗi ~10 giây **chỉ với** server
  khai là Paper/Spigot/Purpur/Bukkit/Folia (chọn trong dialog "Thêm Server" → "Loại server"),
  parse dòng phản hồi `"TPS from last 1m, 5m, 15m: ..."` (tự bỏ mã màu §) để hiển thị. Vanilla/
  Forge/Fabric sẽ không bị tự động gửi lệnh lạ vào console. Kể cả gõ tay `/tps` cũng được nhận
  diện và hiển thị lên UI. Tất cả hiện trên thanh trạng thái: `CPU 12.3% · RAM 1024 MB · TPS from
  last 1m, 5m, 15m: 20.0, 20.0, 20.0`.
  - **MSPT vẫn chưa làm** (Paper có lệnh `/mspt` riêng, định dạng phản hồi khác `/tps` và mình
    chưa có mẫu output thật để parse chắc chắn) — để dành phase sau khi có thể test trên server
    thật.

## Đã sửa/thêm ở bản này

- **Nút Browse**: 3 trường "Thư mục server", "Tên file jar", "Đường dẫn java.exe" trong dialog
  "Thêm Server" giờ có nút "..." mở `QFileDialog` thay vì phải gõ tay đường dẫn.
- **Toolbar**: thanh công cụ trên cùng với "🆕 Server mới", "🗑 Xoá server", "📚 Thư viện
  Mod/Plugin", "⟲ Làm mới".
- **Trang khởi động (startup page)**: nếu chưa có server nào, cửa sổ hiện màn hình chào mừng với
  nút "+ Tạo Server Đầu Tiên" thay vì một console trống trơn. Tự chuyển sang giao diện quản lý
  bình thường ngay khi có ít nhất 1 server.
- **Thư viện Modrinth** (`ui::LibraryDialog` + `net::ModrinthClient`, dùng `QNetworkAccessManager`
  + `QJsonDocument` sẵn có trong Qt, không cần thư viện ngoài): tìm kiếm Mod / Plugin / Modpack /
  Resource Pack / Shader qua API công khai của Modrinth (`api.modrinth.com/v2`), chọn kết quả rồi
  bấm "Cài vào server này" — app tự lấy bản build mới nhất, tải file chính (`primary file`) và bỏ
  đúng thư mục (`mods/`, `plugins/`, `resourcepacks/`, `shaderpacks/`) trong thư mục server đang
  chọn. Riêng **modpack chỉ tải file `.mrpack` về**, chưa tự giải nén/cài — nói rõ trong UI sau
  khi tải xong.
  - Mở từ toolbar ("📚 Thư viện Mod/Plugin") hoặc phải chọn/tạo server trước (Modrinth cần biết
    cài vào thư mục nào).
  - Cần máy có kết nối internet khi dùng tính năng này (mọi phần khác của app hoạt động hoàn
    toàn offline).

## Đã sửa so với lần build lỗi trước

Lỗi build bạn gặp (`WndProc` là `private` nhưng bị gọi từ hàm tự do bên ngoài khi đăng ký
window class) nằm ở lớp UI Win32 cũ. Vì đã thay hẳn `MainWindow`/`AddServerDialog` sang Qt6
(`QMainWindow`/`QDialog`), kiểu lỗi này **không còn tồn tại** — Qt tự quản lý event loop và
window procedure nội bộ, code của mình không cần tự viết `WndProc` nữa.

"Nút thêm server không hoạt động" nhiều khả năng là hệ quả trực tiếp của lỗi build ở trên (exe
không build được thì không có gì hoạt động). Ở bản Qt6 này, "Thêm Server" dùng `QDialog::exec()`
— cơ chế modal dialog được Qt kiểm chứng rất kỹ, đáng tin cậy hơn nhiều so với vòng lặp message
tự viết tay của bản Win32 trước.

## Yêu cầu

- **Visual Studio 2022+ (MSVC)**, **CMake ≥ 3.21**, Windows 10/11 x64.
- **Qt6** (khuyến nghị 6.5+), component `Core` + `Widgets` + `Network` (Network dùng cho Thư
  viện Modrinth). Cài qua Qt Online Installer:
  https://www.qt.io/download-qt-installer — chọn bản ứng với trình biên dịch của bạn, ví dụ
  `MSVC 2019 64-bit` hoặc `MSVC 2022 64-bit`.

## Build

```powershell
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64"
cmake --build build --config Release
```

Thay `C:/Qt/6.7.0/msvc2019_64` bằng đường dẫn Qt thật trên máy bạn (thư mục con của Qt chứa
`lib/cmake/Qt6`). Sau khi build, CMake tự chạy `windeployqt` để copy các DLL Qt cần thiết
(`Qt6Core.dll`, `Qt6Widgets.dll`, `platforms/qwindows.dll`...) vào cùng thư mục với file `.exe`,
nên bạn chạy thẳng `build/Release/MinecraftServerManager.exe` mà không cần chỉnh `PATH`.

Nếu CMake báo không tìm thấy Qt6 (`Could NOT find Qt6`), gần như chắc chắn là do
`CMAKE_PREFIX_PATH` chưa trỏ đúng thư mục Qt — kiểm tra lại đường dẫn có chứa
`lib/cmake/Qt6/Qt6Config.cmake` hay không.

Chạy smoke test (không cần Qt/GUI, chỉ test JSON + ConfigManager):

```powershell
cmake -S . -B build -A x64 -DMSM_BUILD_TESTS=ON
cmake --build build --config Release --target MinecraftServerManagerTests
./build/Release/MinecraftServerManagerTests.exe
```

## ⚠️ Quan trọng: chưa build/test được trong môi trường tạo ra code này

Code được viết trong môi trường **Linux, không có MSVC/Windows SDK, không có Qt6**, nên **chưa
compile thử được lần nào**. Mình đã soát kỹ từng file theo tay, nhưng những lỗi kiểu "thiếu
include", "sai chữ ký hàm Qt", hay lỗi từ `moc` (do quên `Q_OBJECT`, quên khai báo `slots` đúng
chỗ...) chỉ MSVC + Qt thật mới bắt được hết. Hãy build theo hướng dẫn trên và gửi lại **nguyên
văn lỗi** (chụp toàn bộ dòng lỗi trong Error List, không chỉ 4 dòng đầu) để mình vá đúng chỗ.

## Sử dụng

1. Chạy `MinecraftServerManager.exe`.
2. Bấm **"+ Thêm Server"** → điền Tên, Thư mục server, tên file jar (vd `server.jar`), đường dẫn
   `java.exe` (hoặc chỉ `java.exe` nếu đã có trong PATH), RAM tối thiểu/tối đa (MB), Port → **Save**.
3. Chọn server trong danh sách bên trái, bấm **▶ Start**.
4. Xem console real-time bên phải; gõ lệnh vào ô dưới cùng rồi bấm **Gửi** (hoặc nhấn Enter).
5. **■ Stop** gửi lệnh `stop` và chờ tối đa 30s trước khi buộc kill; **⟳ Restart** = Stop rồi Start.
6. Nếu server tự thoát ngoài ý muốn, trạng thái chuyển **Crashed** và (nếu bật `autoRestart`
   trong `data/servers.json`) sẽ tự khởi động lại.

Lần chạy đầu với 1 server mới, bạn vẫn cần tạo/chấp nhận `eula.txt` như bình thường (chạy tay
`java -jar server.jar --nogui` một lần trong thư mục server, sửa `eula=false` → `eula=true`),
vì đây là yêu cầu của chính Mojang, Manager không (và không nên) tự động bypass nó.

## Không nằm trong MVP này (theo đúng lộ trình Phase 2–6 của spec)

- **MSPT** — xem giải thích ở trên.
- **Java Manager** (tự phát hiện các bản JDK/JRE đã cài) — Phase 3. Hiện người dùng tự nhập
  đường dẫn `java.exe` (đã có nút Browse để chọn file thay vì gõ tay).
- **server.properties editor, EULA helper, Whitelist/OP UI** — Phase 3.
- **BackupManager** (tạo/restore/nén/retention) — Phase 4.
- **Cài đặt modpack tự động** (giải nén `.mrpack`, tải toàn bộ dependency, áp overrides) — Thư
  viện Modrinth hiện chỉ tải file modpack về, chưa tự cài; xem mục "Thư viện Modrinth" bên dưới.
- **Playit.gg / Cloudflare Tunnel / firewall integration** — Phase 6.

## Giới hạn kỹ thuật đã biết (được ghi nhận có chủ đích, không phải bug bỏ sót)

- `core::EventDispatcher` chưa có `Unsubscribe()`. Trong luồng tắt ứng dụng bình thường, mọi
  server đang chạy được `Stop()` **trước** khi `MainWindow`/`EventDispatcher` bị hủy
  (`Application::ShutdownServers()` chạy ngay sau khi `qtApp.exec()` trả về), nên không có
  thread nền nào còn gọi `Publish()` sau khi `MainWindow` đã bị hủy trong trường hợp thông
  thường. Có một khoảng hẹp giữa lúc cửa sổ đóng và lúc `ShutdownServers()` chạy xong nơi
  `QMetaObject::invokeMethod` tới một `MainWindow` sắp bị hủy về lý thuyết có thể chạy vào một
  con trỏ đã dangling — vô hại trong thực tế vì tiến trình thoát ngay sau đó, nhưng đáng để biết
  nếu bạn mở rộng vòng đời ứng dụng sau này (ví dụ: giữ ứng dụng chạy nền ở khay hệ thống).
- ID server sinh từ `thời gian hệ thống + counter`, không dùng GUID/COM — đủ duy nhất cho file
  cấu hình cục bộ.
- JSON reader/writer tự viết (cho `data/servers.json`) chỉ đủ dùng cho đúng schema
  `ServerConfig`, không phải parser JSON tổng quát 100% theo RFC 8259.
- Backend xử lý tiến trình (`process/`, `console/`) **không** dùng Qt (`QProcess`) mà vẫn dùng
  Win32 API thuần theo đúng mục 8-10 của spec gốc — đây là lựa chọn có chủ đích để giữ toàn quyền
  kiểm soát Job Object/pipe như spec yêu cầu, không phải do quên chuyển đổi.

## Cấu trúc

Bám sát mục 5 của spec, có thêm `src/ui/AddServerDialog.*` (dialog thêm server, không có trong
sơ đồ gốc nhưng cần thiết để hoàn thành "Tạo server profile" trong Definition of Done).

