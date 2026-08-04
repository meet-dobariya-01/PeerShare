# PeerShare - Tracker-Based P2P File Distribution System

A multi-threaded **Peer-to-Peer (P2P)** direct file distribution system built in C++17. 

Peers can discover and download complete topic directories directly from other peers or a central seed server without intermediate file chunking.

---

## 💡 How It Works (Simple Concept)

1. **Tracker**: A lightweight directory server. It tracks which peers (IP & Port) own which topic folders. **It does not transfer any files.**
2. **Server**: The initial seed host that owns all raw topic folders.
3. **Peer**: Can act as a **Client** (to download topic folders) and as a **Server** (to serve downloaded topic folders to other peers).

```
                     ┌──────────────────┐
                     │     Tracker      │
                     │  (Directory/IPs) │
                     └────────▲─────────┘
                              │
               1. Query Topic │ 3. Register as Host
                              │
  ┌───────────────────────────┴───────────────────────────┐
  │                                                       │
┌─┴────────────┐          2. Direct File Download       ┌─┴────────────┐
│    Peer 1    ├───────────────────────────────────────►│ Seed Server /│
│   (Client)   │       (Full Direct TCP Stream)         │    Peer 2    │
└──────────────┘                                        └──────────────┘
```

---

## 📁 Project Structure

```text
PeerShare/
├── Tracker/
│   ├── Host.cpp / Host.h        # Host IP & Port data structure
│   ├── Topic.cpp / Topic.h      # Topic & Host listing logic
│   ├── Tracker.cpp / Tracker.h  # Tracker database & lookup logic
│   └── main.cpp                 # Tracker server entry point (Port 9000)
│
├── Server/
│   ├── server.cpp / server.h    # Seed server implementation
│   ├── client.cpp / client.h    # Peer client & P2P server implementation
│   ├── FileManager.h / Filemanager.cpp # Directory & disk scanner
│   ├── FileTransfer.h / FileTransfer.cpp # TCP Direct file sender/receiver
│   ├── main.cpp                 # Seed server entry point
│   └── raw-img/                 # Raw dataset folders (Cane, Gatto, etc.)
│
└── readme.md
```

---

## 🚀 Quick Start Guide

### Step 1: Build the Project

Open your terminal in the `PeerShare` folder:

#### 1️⃣ Build the Tracker
```bash
cd Tracker
g++ -std=c++17 Host.cpp Topic.cpp Tracker.cpp main.cpp -o tracker -pthread
cd ..
```

#### 2️⃣ Build the Seed Server
```bash
cd Server
g++ -std=c++17 server.cpp Filemanager.cpp FileTransfer.cpp main.cpp -o server -pthread
```

#### 3️⃣ Build the Peer Client
```bash
g++ -std=c++17 client.cpp Filemanager.cpp FileTransfer.cpp -o peer -pthread
cd ..
```

---

## 🏃 Running the System (Step-by-Step)

### Terminal 1: Run Tracker
```bash
cd Tracker
./tracker
```
*The Tracker starts listening on port `9000`.*

---

### Terminal 2: Run Seed Server
```bash
cd Server
./server 8080 ./raw-img
```
*The Server starts listening on port `8080` serving raw topic folders from `./raw-img`.*

---

### Terminal 3: Run Peer 1
```bash
cd Server
./peer 127.0.0.1 9000
```
#### Useful Peer Commands:
- `fetch <topic>` — Downloads an entire topic directory (e.g. `fetch Cane`).
- `register <topic> <port> <dir>` — Registers your peer with the Tracker so others can download from you.
- `serve <port> <dir>` — Starts listening for incoming peer download requests.
- `quit` — Disconnects from the network.

---

## 🛠️ Features & Design

- **Direct File Streaming**: Whole files are transferred directly over raw TCP sockets without chunking overhead.
- **Multi-Threaded Architecture**: Handles concurrent connections seamlessly using standard `std::thread` and `std::mutex`.
- **Decentralized Sharing**: Once a peer downloads a topic, it can register with the Tracker to share it with new peers, reducing server load.