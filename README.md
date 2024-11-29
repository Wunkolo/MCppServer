<div align="center">
    
# MCpp Server

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![Current version)](https://img.shields.io/badge/current_version-1.21.1-green)

Fast and super efficient C++ 1.21.1 Minecraft Server. Compatible with Java Clients.

![image](https://i.postimg.cc/cJsPzxp9/sample-Image-Sized.png)

</div>

MCpp Server is a high-performance Minecraft server developed entirely in C++. Designed for speed, efficiency, and extensive customization, MCpp Server aims to provide a seamless and enjoyable experience for players while maintaining full compatibility with the latest Minecraft features.

## 🚀 Features

### 🏎️ Performance
- **Multi-threaded Architecture:** Leverages multiple threads to handle various server tasks simultaneously.
- **Super fast and efficient Chunk Loading and Generation**: Uses multiple threads to load and generate chunks with minimal Memory usage.
- **Optimized Codebase:** Written in C++ for maximum efficiency and low latency.

### 🔧 Customization & Extensibility
- **Configurable Settings:** Easily adjustable configuration files to tailor server behavior to your needs.
- **<span style="color:gray">*Plugin Support soon*</span>:** Provides a foundation for developing and integrating custom plugins.

### 🌐 Networking
- **Packet Compression:** Reduces bandwidth usage by compressing data packets.
- **Server Status & Ping:** Provides real-time server status information and latency measurements.

### 🧩 Supported and WIP Features
- Login
    - [x] Authentication (online mdoe)
    - [x] Encryption
    - [x] Packet Compression
- Server Configuration
    - [x] Server Links
    - [x] Registries
    - [x] Resource Packs (multiple)
    - [x] Server Brand
    - [ ] Cookies
- Server
    - [ ] Lua Plugin API
    - [x] Query
    - [x] RCON
    - [x] Commands
    - [x] Chat
- World
    - [x] World Joining
    - [x] Chunk Loading
    - [x] Tablist
    - [x] Entity Spawning
    - [x] World Loading
    - [x] Chunk Generation
    - [ ] World Time
    - [ ] Scoreboard
    - [ ] World Saving
    - [ ] World Borders
- Player
    - [x] Player Skins
    - [x] Client brand
    - [x] Movement
    - [x] Inventory
    - [x] Equipment
    - [ ] Combat
- Entities
    - [x] Players
    - [ ] Mobs (Animals, Monsters)
    - [ ] Entity AI
    - [ ] Boss
    - [ ] Minecart

## 🌍 Use Pre-Generated World
Just put the world folder of your Vanilla Minecraft world in the Directory where the server executable is and it will be loaded when the server starts.

## ⚠️ Important Notes
- **Linux Compatibility:** Mojang authentication is currently not functional on Linux, and the Linux version has not been thoroughly tested. Users may encounter issues when running MCpp on Linux systems.
- **Ongoing Development:** MCpp is actively being developed. Contributions and feedback are welcome to help improve the server.

## 🛠️ Installation & Building

### 📋 Prerequisites
- **C++20 Compiler:** Ensure you have a modern C++ compiler installed (e.g., GCC, Clang). On Windows you need MingW.
- **CMake:** Version 3.14 or higher.
- **Git:** To clone the repository.

### 🔧 Build Instructions

1. **Clone the Repository**
    ```bash
    git clone https://github.com/Noeli14/MCppServer.git
    cd MCppServer
    ```

2. **Create a Build Directory**
    ```bash
    mkdir build
    cd build
    ```

3. **Generate Build Files with CMake**
    ```bash
    cmake ..
    ```

4. **Compile the Project**
    ```bash
    cmake --build .
    ```

### 🚀 Running the Server
After a successful build, execute the server binary:
```bash
./MCppServer
   ```
**Important**: Make sure the resources folder is in the same directory as the executable.

## 📦 Data Sources
MCpp utilizes data from the [PrismarineJS](https://github.com/PrismarineJS/minecraft-data) Minecraft Data repository to ensure accurate and up-to-date game mechanics and data.

## 🤝 Contributing
Contributions are welcome! Whether it's reporting bugs, suggesting features, or submitting pull requests, your help is greatly appreciated.
1. **Fork the Repository**
2. **Create a Feature Branch:**
 ```bash
git checkout -b feature/YourFeature
   ```
3. **Commit Your Changes:**
 ```bash
git commit -m "Add your feature"
   ```
4. **Push to the Branch:**
 ```bash
git push origin feature/YourFeature
   ```
5. **Open a Pull Request**

## 📄 License
MCpp is licensed under the MIT License.

## 📫 Contact
For any questions or support, feel free to open an issue on the [GitHub repository](https://github.com/Noeli14/MCppServer)
