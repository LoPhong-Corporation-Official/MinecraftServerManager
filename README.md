# Minecraft Server Manager

A powerful, lightweight, and user-friendly desktop application built with **C++** designed to simplify the creation, management, and monitoring of Minecraft servers.

---

## ✨ Features

- **🖥️ Dedicated Console & Live Process Pipe:**
  - Real-time server log output streaming using non-blocking asynchronous pipes.
  - Interactive command input directly to the Minecraft server console.

- **🔌 Modrinth & Core Provider Integration:**
  - Search and download mods, plugins, and modpacks directly via **Modrinth API**.
  - Built-in provider to automatically download standard server jars (Vanilla, Paper, Spigot, Fabric, etc.).

- **☕ Java Manager:**
  - Automatically detect installed Java Runtimes (JDK/JRE).
  - Assign custom Java binaries and flags (`Xms`, `Xmx`, Garbage Collection options) per server instance.

- **⚙️ Visual Property Editor:**
  - Graphical user interface to edit `server.properties` (Gamemode, Difficulty, Ports, Whitelist, Max Players) without manually modifying text files.

- **💾 World & Backup Management:**
  - Create and restore server backups easily.
  - Player management interface for managing Whitelists, OPs, Banning, and Kicking users.

- **📊 Resource Monitoring & Auto-Restart:**
  - Track server performance (CPU, RAM utilization, and process health).
  - Event-driven architecture for automatic status tracking and crashes recovery.

---

## 🏗️ Project Architecture

The project is structured modularly:

```
MinecraftServerManager/
├── cmake/               # CMake compiler options and configuration
├── src/
│   ├── app/             # Main application lifecycle management
│   ├── config/          # Configuration & server.properties parsing logic
│   ├── console/         # Console buffers and input/output controllers
│   ├── core/            # Event system, JSON parser, Logger, and utilities
│   ├── javamanager/     # Java runtime detection & execution management
│   ├── monitor/         # Process performance monitoring (CPU/RAM)
│   ├── net/             # Modrinth API & Server Jar downloader clients
│   ├── platform/        # OS-specific startup and environment helpers
│   ├── process/         # Low-level process spawner & Pipe IO handling
│   ├── server/          # Server core management & import functionality
│   └── ui/              # User Interface dialogs & main windows
└── tests/               # Unit tests (Config, Process, Network)
```

---

## 🛠️ Building from Source

### Prerequisites

- **C++20** compatible compiler (GCC, Clang, or MSVC)
- **CMake** (v3.16 or higher)
- **Git**

### Build Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com/LoPhong-Corporation-Official/MinecraftServerManager.git
   cd MinecraftServerManager
   ```

2. **Generate Build Files & Build:**
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

3. **Run the Application:**
   - **Windows:** `.\Release\MinecraftServerManager.exe`
   - **Linux/macOS:** `./MinecraftServerManager`

---

## 🧪 Running Unit Tests

To run the test suite, build with testing enabled and run CTest:

```bash
cd build
ctest --output-on-failure
```

---

## 🤝 Contributing

Contributions are welcome! Please follow these steps:

1. Fork the Repository.
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`).
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`).
4. Push to the Branch (`git push origin feature/AmazingFeature`).
5. Open a Pull Request.

---

## 📄 License

This project is open-source. Check the repository for licensing information.
