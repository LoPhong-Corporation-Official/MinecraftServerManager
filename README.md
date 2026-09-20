# Minecraft Server Manager (Qt6 UI + Native Win32 Process Backend)

Triển khai theo bản đặc tả kỹ thuật (`Minecraft_Server_Manager___Technical_Specification.md`).
UI dùng **Qt6 Widgets** theo yêu cầu; **lớp quản lý tiến trình Minecraft vẫn giữ nguyên Win32 API
gốc** (CreateProcessW, Job Object, pipe...) như spec yêu cầu — Qt chỉ thay thế phần hiển thị.

Tiến độ: **Phase 1-5 đầy đủ/phần lớn** (chi tiết bên dưới), **Phase 6 (Network Tunnel)** —
quản lý vòng đời tiến trình Playit.gg/Cloudflare Tunnel, cộng thêm vài tính năng lấy cảm hứng từ
[Fork](https://github.com/ForkGG/Fork): Import Server có sẵn, xem/xoá plugin-mod đã cài, tự
restart định kỳ có cảnh báo trong game. Chưa làm: firewall rule tự động, Discord bot.

## Tính năng theo phase

### Phase 1 — Core MVP
- `CreateProcessW` + pipe redirect + Windows Job Object (`KILL_ON_JOB_CLOSE`).
- Console real-time trên `std::jthread` riêng, không block UI; gửi lệnh qua stdin.
- State machine đầy đủ + phát hiện crash + auto-restart (tối đa 5 lần/10 phút).
- Lưu server profile vào `data/servers.json` (JSON reader/writer tự viết, không phụ thuộc ngoài).
- RAII cho mọi `HANDLE`.

### Phase 2 — Process Monitoring
- `monitor::ProcessMonitor` lấy mẫu CPU%/RAM mỗi 2 giây (`GetProcessTimes` +
  `K32GetProcessMemoryInfo`, thuần Win32).
- **TPS**: tự động gửi `/tps` mỗi ~10s **chỉ với** server khai Paper/Spigot/Purpur/Bukkit/Folia
  (chọn ở "Loại server" khi tạo), parse `"TPS from last 1m, 5m, 15m: ..."` (tự bỏ mã màu §).
  Vanilla/Forge/Fabric không bị gửi lệnh lạ vào console. Gõ tay `/tps` cũng được nhận diện.
  **MSPT chưa làm** (định dạng phản hồi `/mspt` khác, chưa có mẫu thật để parse chắc).

### Phase 3 — Java Manager, cấu hình server, EULA, người chơi
- **Java Manager** (`javamanager::DetectInstallations`): quét `JAVA_HOME`, mọi thư mục trong
  `PATH`, và các thư mục cài đặt phổ biến (Oracle, Eclipse Adoptium/Temurin, Azul Zulu, Amazon
  Corretto, Microsoft Build of OpenJDK, BellSoft Liberica) để liệt kê sẵn trong dialog "Thêm
  Server" — không cần gõ tay đường dẫn `java.exe` nữa (vẫn có nút Browse cho trường hợp khác).
  *Không đọc Registry* (mỗi hãng có schema khác nhau) và *không chạy `java -version`* để lấy số
  phiên bản (tránh phải spawn nhiều tiến trình chỉ để liệt kê) — để dành cho bản sau nếu cần.
- **server.properties editor** (`⚙️ server.properties` trên toolbar): form quen thuộc cho các
  mục hay chỉnh nhất (MOTD, max-players, difficulty, gamemode, pvp, online-mode, whitelist,
  view-distance, spawn-protection, level-name, level-seed). Các dòng khác trong file được giữ
  nguyên khi lưu.
- **EULA helper** (`📜 Chấp nhận EULA`): khi server dừng ngay vì `eula.txt` có `eula=false`
  (hành vi mặc định của chính Minecraft ở lần chạy đầu), app tự nhận diện và **hỏi rõ** bạn có
  đọc & đồng ý với https://aka.ms/MinecraftEULA không — chỉ khi bấm Đồng ý mới ghi `eula=true`.
  Không có chuyện tự động bypass âm thầm.
- **Whitelist/OP UI** (`👥 Người chơi`, cần server đang chạy): gửi thẳng lệnh console
  `op`/`deop`/`whitelist add`/`whitelist remove`/`kick`/`ban`/`pardon` — để chính Minecraft tự lo
  việc tra UUID, thay vì tự sửa tay `ops.json`/`whitelist.json`.

### Phase 4 — Backup Manager
- `backup::CreateBackup`: zip world (`world`, `world_nether`, `world_the_end` — cái nào có) +
  `server.properties` thành `backups/backup-YYYY-MM-DD_HH-MM-SS.zip`. Zip tự viết (định dạng
  STORE, không nén — xem lý do trong `backup/ZipWriter.hpp`), stream từng file 64 KB một lần,
  không load cả file vào RAM nên an toàn với world nhiều GB.
- **Retention**: tự giữ lại `kMaxBackupsToKeep` = 10 bản mới nhất, bản cũ hơn tự xoá.
- Dialog `💾 Backups`: tạo (chạy nền, không đứng UI), xoá, và "📂 Mở thư mục backups".
- **Restore chưa làm** — xem mục Giới hạn bên dưới, lý do là an toàn dữ liệu.

### Phase 5 — Server Providers
- Trong dialog "Thêm Server", mục **"Tải server tự động"**: chọn Vanilla/Paper/Fabric → chọn
  phiên bản → bấm "⬇ Tải" — app tự tải thẳng file server.jar vào Thư mục server đã điền, và tự
  điền vào ô "Tên file jar". Không cần tự đi tìm/tải file jar ở đâu nữa.
  - **Vanilla**: `piston-meta.mojang.com/mc/game/version_manifest_v2.json` (chỉ liệt kê bản
    `release`, không hiện snapshot) → `downloads.server.url` của từng phiên bản.
  - **Paper**: `api.papermc.io/v2/projects/paper` (danh sách version) → `.../builds` (lọc
    `channel == "default"`, lấy build mới nhất) → tải `paper-{version}-{build}.jar`.
  - **Fabric**: `meta.fabricmc.net/v2/versions/game` (chỉ bản `stable`) → lấy loader mới nhất
    tương thích (`/versions/loader/{version}`) + installer mới nhất (`/versions/installer`) → tải
    thẳng file server launcher tự-bootstrap ở endpoint `.../server/jar` (file này tự tải thêm
    vanilla+loader trong lần chạy đầu — vẫn cần internet ở lần Start đầu tiên).
  - **Forge/NeoForge không có** trong danh sách: 2 loader này phân phối dưới dạng file cài đặt
    tương tác (installer chạy wizard/CLI để vá vào jar vanilla), không phải 1 URL tải thẳng như
    Vanilla/Paper/Fabric — không khớp với mô hình "1 URL, 1 file" ở đây. Muốn dùng Forge, bạn vẫn
    tự chạy trình cài đặt Forge trước như bình thường, rồi trỏ "Tên file jar" vào file nó tạo ra.
  - Đã tra cứu tài liệu chính thức của cả 3 API trước khi viết code (không đoán field/endpoint),
    nhưng **chưa gọi thử request thật** — xem mục Giới hạn.

### Phase 6 — Network Tunnel (Playit.gg / Cloudflare Tunnel)
App **không** tự implement lại giao thức tunnel riêng của Playit.gg hay Cloudflare (không khả
thi/không phù hợp — đây là dịch vụ độc quyền). Thay vào đó, `⚙ Cài đặt nâng cao` cho mỗi server
cho phép:
- Trỏ tới file thực thi bạn đã tải sẵn (`playit.exe`, `cloudflared.exe`, hoặc bất kỳ exe nào) +
  tham số dòng lệnh.
- App quản lý vòng đời tiến trình đó y hệt cách quản lý `java.exe` (dùng lại `process::Process` —
  tức cũng được bọc trong Windows Job Object, không thể sống sót nếu Manager crash).
- Output của tunnel (bao gồm địa chỉ public nó in ra) được gộp thẳng vào console của server, đánh
  dấu tiền tố `[tunnel]` — không cần mở thêm cửa sổ terminal riêng.
- Tuỳ chọn tự khởi động tunnel cùng lúc Start server; tunnel cũng tự dừng khi server dừng.
- Nút Start/Stop tunnel thủ công ngay trong dialog, độc lập với server (test tunnel mà không cần
  chạy Minecraft).

### Từ repo Fork (ForkGG/Fork) — đã thêm
- **📥 Import Server**: trỏ vào thư mục server có sẵn (đã có file .jar từ trước, có thể do tự
  tải hoặc chuyển từ tool khác) — `server::ScanServerDirectory` tự tìm file .jar, đoán loại
  server từ tên file, và đọc `server-port` từ `server.properties` nếu đã có, để không phải tự
  gõ tay từng thứ.
- **🧩 Đã cài**: liệt kê file `.jar` trong `plugins/` và `mods/` của server, xoá được — bổ sung
  cho Thư viện Modrinth (vốn chỉ cài, không xem/xoá được cái đã có).
- **Tự động Restart định kỳ**: đặt chu kỳ (giờ) trong `⚙ Cài đặt nâng cao`; app gửi cảnh báo
  `say [Manager] Server sẽ tự khởi động lại sau 60 giây.` vào chat trước khi restart, không restart
  đột ngột giữa lúc người chơi đang chơi.
- Discord bot điều khiển server (có trong Fork) — **chưa làm**, xem mục Giới hạn.

### UI chung
- Theme tối (QSS + style Fusion), nút bấm phân màu theo hành động.
- Toolbar: Server mới / Xoá server / server.properties / Người chơi / Backups / Chấp nhận EULA /
  Thư viện Mod-Plugin / Làm mới.
- Trang khởi động (empty state) khi chưa có server nào.
- Nút Browse cho Thư mục server / file jar / java.exe trong dialog "Thêm Server".
- **Thư viện Modrinth** (`net::ModrinthClient` + `ui::LibraryDialog`, dùng
  `QNetworkAccessManager`/`QJsonDocument` sẵn có trong Qt): tìm & cài Mod/Plugin/Resource
  Pack/Shader vào đúng thư mục (`mods/`, `plugins/`, `resourcepacks/`, `shaderpacks/`) của server
  đang chọn. Modpack chỉ tải `.mrpack` về, chưa tự cài. Cần internet khi dùng; mọi phần khác của
  app chạy offline hoàn toàn.

## Yêu cầu

- **Visual Studio 2022+ (MSVC)**, **CMake ≥ 3.21**, Windows 10/11 x64.
- **Qt6** (khuyến nghị 6.5+), component `Core` + `Widgets` + `Network`. Cài qua Qt Online
  Installer: https://www.qt.io/download-qt-installer — chọn bản ứng với trình biên dịch của bạn,
  ví dụ `MSVC 2019 64-bit` hoặc `MSVC 2022 64-bit`.

## Build

```powershell
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64"
cmake --build build --config Release
```

Thay đường dẫn Qt cho đúng máy bạn (thư mục chứa `lib/cmake/Qt6`). Sau khi build, CMake tự chạy
`windeployqt` để copy DLL Qt cần thiết vào cạnh file `.exe`.

Chạy smoke test (không cần Qt/GUI, chỉ test JSON + ConfigManager):

```powershell
cmake -S . -B build -A x64 -DMSM_BUILD_TESTS=ON
cmake --build build --config Release --target MinecraftServerManagerTests
./build/Release/MinecraftServerManagerTests.exe
```

## ⚠️ Quan trọng: chưa build/test được trong môi trường tạo ra code này

Code được viết trong môi trường **Linux, không có MSVC/Windows SDK, không có Qt6**, nên **chưa
compile thử được lần nào**. Mình đã soát kỹ từng file theo tay (include, chữ ký hàm Qt, thứ tự
hủy đối tượng, an toàn đa luồng...), nhưng vẫn có khả năng sót lỗi nhỏ mà chỉ MSVC + Qt thật mới
bắt được. Build theo hướng dẫn trên rồi gửi lại **nguyên văn lỗi** (toàn bộ Error List, không chỉ
vài dòng đầu) để mình vá đúng chỗ.

## Sử dụng

1. Chạy `MinecraftServerManager.exe`.
2. Toolbar → **"🆕 Server mới"** (hoặc nút "+ Tạo Server Đầu Tiên" ở màn hình chào) → điền Tên,
   Thư mục, file jar, chọn Java (combo tự phát hiện hoặc Browse), Loại server, RAM, Port → Save.
3. Chọn server, bấm **▶ Start**.
4. Xem console real-time, gõ lệnh + Gửi (hoặc Enter). CPU/RAM/TPS hiện trên thanh trạng thái.
5. Cần chỉnh MOTD/difficulty/... → **⚙️ server.properties**. Quản lý người chơi khi server đang
   chạy → **👥 Người chơi**. Backup thủ công/tự động dọn cũ → **💾 Backups**. Cài mod/plugin →
   **📚 Thư viện Mod/Plugin**; xem/xoá cái đã cài → **🧩 Đã cài**.
6. Nếu server dừng ngay vì chưa chấp nhận EULA, app tự hỏi — đồng ý thì bấm Yes, hoặc dùng
   **📜 Chấp nhận EULA** trên toolbar bất cứ lúc nào.
7. Đã có sẵn 1 server ở đâu đó (không cần tạo mới) → **📥 Import Server**, trỏ vào thư mục đó.
8. Muốn public server qua Playit.gg/Cloudflare Tunnel, hoặc tự restart định kỳ → **🌐 Tunnel &
   Nâng cao**.

## Không nằm trong bản này

- **MSPT thật** — xem Phase 2.
- **Java Manager đọc Registry / probe version** — xem Phase 3.
- **Restore backup tự động** — xem Phase 4; hiện dùng "📂 Mở thư mục backups" rồi tự giải nén
  bằng Windows Explorer (dừng server trước, xoá/đổi tên thư mục world cũ, giải nén bản backup
  vào đúng chỗ).
- **Cài modpack tự động** (giải nén `.mrpack`, tải dependency, áp overrides) — Thư viện Modrinth
  hiện chỉ tải file `.mrpack` về.
- **Forge/NeoForge auto-download** — xem Phase 5, lý do kỹ thuật (installer tương tác, không
  phải 1 URL tải thẳng).
- **Discord bot** (điều khiển server từ Discord, có trong repo Fork) — cần thêm thư viện
  WebSocket/Gateway client cho Discord + OAuth, đi ngược triết lý "tối thiểu dependency" của
  project này; để dành làm riêng nếu bạn thực sự cần, thay vì làm dở dang.
- **In-app code editor có syntax highlight** (Fork bản mới có) — hiện chưa có; muốn sửa file cấu
  hình khác ngoài server.properties (bukkit.yml, spigot.yml, config plugin...) thì tạm mở bằng
  Notepad/VS Code như bình thường.
- **Playit.gg/Cloudflare Tunnel**: chỉ quản lý vòng đời tiến trình bạn đã cài sẵn, không tự tải
  giúp `playit.exe`/`cloudflared.exe`, không tự tạo Cloudflare Tunnel/route DNS giúp bạn (cần tài
  khoản Cloudflare, tự làm trên Dashboard/CLI của họ trước).
- **Playit.gg / Cloudflare Tunnel / firewall integration** — Phase 6 hoàn tất phần tunnel;
  **firewall rule tự động (mở port Windows Firewall) chưa làm**.
- Modrinth "Plugin" filter dùng facet `project_type:plugin` — mình đã tra docs.modrinth.com
  nhưng chưa test request thật; nếu trả về rỗng, thử đổi qua "Mod" (nhiều plugin Bukkit/Paper
  vẫn được gắn category "mod" trên Modrinth).

## Giới hạn kỹ thuật đã biết (có chủ đích, không phải bug bỏ sót)

- **Vì sao restore chưa làm**: extract-tại-chỗ mà lỡ giữa chừng (mất điện, hết dung lượng đĩa...)
  có thể phá luôn world hiện tại mà không còn đường lùi. Làm đúng cần: dừng server, di chuyển
  world hiện tại sang thư mục an toàn trước khi ghi đè, xác minh archive trước khi động vào bất
  cứ file nào — nhiều bước hơn để làm đúng và an toàn so với create/list/delete, nên để dành.
- `ZipWriter` chỉ hỗ trợ STORE (không nén) và không hỗ trợ ZIP64 (không backup được 1 file nào đó
  > 4 GB — bản thân từng file trong world Minecraft hiếm khi to vậy, region file thường vài MB).
- `core::EventDispatcher` chưa có `Unsubscribe()`. Mọi server đang chạy được `Stop()` trước khi
  `MainWindow` bị hủy khi thoát app bình thường nên không có thread nền nào gọi `Publish()` sau
  khi `MainWindow` đã bị hủy trong trường hợp thông thường; có một khoảng hẹp giữa lúc đóng cửa
  sổ và lúc dừng xong các server nơi 1 sự kiện trễ về lý thuyết có thể nhắm vào con trỏ đã
  dangling — vô hại vì tiến trình thoát ngay sau đó.
- ID server sinh từ thời gian hệ thống + counter, không dùng GUID/COM.
- JSON reader/writer tự viết cho `data/servers.json` chỉ đủ dùng cho đúng schema `ServerConfig`.
- Java Manager không đọc Windows Registry, không probe `java -version` (xem Phase 3 ở trên).

## Cấu trúc

```
src/
  app/            Application - khởi tạo QApplication, wiring, shutdown
  core/           Types, Event, EventDispatcher, Logger, Json, StringUtil (dùng chung)
  process/        Process, ProcessPipes - Win32 CreateProcessW + Job Object
  monitor/        ProcessMonitor - CPU%/RAM sampling (Phase 2)
  console/        ConsoleBuffer, ConsoleController - đọc pipe trên thread riêng
  javamanager/    JavaManager - dò Java cài sẵn (Phase 3)
  backup/         ZipWriter, BackupManager (Phase 4)
  server/         MinecraftServer (state machine + EULA + tunnel + scheduled restart),
                  ServerManager, ServerImporter (Fork-inspired import)
  config/         ConfigManager (data/servers.json), ServerProperties (server.properties)
  net/            ModrinthClient (mod/plugin/modpack), ServerProviderClient (Phase 5: tải
                  server.jar Vanilla/Paper/Fabric)
  ui/             MainWindow, AddServerDialog, ImportServerDialog, PropertiesDialog,
                  PlayersDialog, BackupsDialog, InstalledAddonsDialog, AdvancedSettingsDialog
                  (tunnel + scheduled restart), LibraryDialog
```
