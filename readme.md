# Topic-Based P2P File Distribution with Tracker

## Overview

This project implements a **Tracker-Based Peer-to-Peer (P2P) File Distribution System** in C++.

The system allows peers to request and share complete topic directories. A central Tracker maintains metadata about which hosts currently possess each topic, while the actual file transfer occurs directly between peers or between a server and peers.

Initially, the server owns all topic directories. As peers download topics, they register themselves with the Tracker and become additional sources for those topics, reducing dependency on the central server.

---

## Project Components

### Tracker

The Tracker is responsible for:

- Maintaining the list of available topics.
- Maintaining the mapping between topics and hosts.
- Handling peer registration.
- Returning available hosts for a requested topic.

**The Tracker never transfers files.**

---

### Server

The Server:

- Stores all topic directories.
- Serves topic directories to requesting peers.
- Handles multiple client requests using multithreading.

Initially, every topic exists only on the server.

---

### Peer

A Peer acts as both:

- **Client** (downloads topic directories)
- **Server** (uploads downloaded topic directories)

Workflow:

1. Request a topic from the Tracker.
2. Receive the list of available hosts.
3. Download the topic from one host.
4. Store the topic locally.
5. Register with the Tracker.
6. Serve the downloaded topic to future peers.

---

## Communication Flow

```text
Peer
   │
   │ QUERY Topic
   ▼
Tracker
   │
   │ Host List
   ▼
Peer
   │
   │ DOWNLOAD Topic
   ▼
Server / Peer
   │
   │ Topic Files
   ▼
Peer
   │
   │ REGISTER_TOPIC
   ▼
Tracker
```

---

## Project Structure

```text
Topic-Based-P2P-File-Distribution/

├── Tracker/
│   ├── Tracker.cpp / Tracker.h
│   ├── Topic.cpp / Topic.h
│   ├── Host.cpp / Host.h
│   └── main.cpp
│
├── Server/
│   ├── server.cpp / server.h
│   ├── client.cpp / client.h      (Peer implementation)
│   ├── Filemanager.cpp / FileManager.h
│   ├── FileTransfer.cpp / FileTransfer.h
│   └── raw-img/
│
└── readme.md
```

---

## Features

- Tracker-based topic discovery
- Multi-threaded Tracker
- Multi-threaded Server
- Multi-threaded Peer
- Directory-based file transfer
- Peer registration after download
- Peer-to-peer file sharing
- Linux socket programming in C++
- TCP communication

---

## Technologies

- C++17
- POSIX/BSD Sockets
- std::thread
- std::mutex
- std::filesystem
- Linux (Ubuntu)

---

## Demonstration

1. Start the Tracker.
2. Start the Server.
3. Start Peer 1 and request a topic.
4. Peer 1 downloads the topic and registers with the Tracker.
5. Start Peer 2 and request the same topic.
6. Peer 2 receives a list containing both the Server and Peer 1.
7. Peer 2 can download the topic directly from Peer 1.

---

## Objective

The objective of this project is to demonstrate a tracker-based peer-to-peer file distribution system where peers cooperate in sharing topic directories instead of relying solely on a central server.

---

## How to Run

### Step 1: Clone the Repository

```bash
git clone <repository-url>
cd Topic-Based-P2P-File-Distribution
```

---

### Step 2: Build the Tracker

```bash
cd Tracker
g++ -std=c++17 *.cpp -o tracker -pthread
```

---

### Step 3: Build the Server

```bash
cd ../Server
g++ -std=c++17 server.cpp FileManager.cpp FileTransfer.cpp main.cpp -o server -pthread
```

---

### Step 4: Build the Peer

```bash
g++ -std=c++17 client.cpp FileManager.cpp FileTransfer.cpp -o peer -pthread
```

> **Note:** If your project uses additional source files, include them in the compilation command accordingly.

---

### Step 5: Start the Tracker

Open a terminal:

```bash
cd Tracker
./tracker
```

---

### Step 6: Start the Server

Open a second terminal:

```bash
cd Server
./server
```

The server will begin listening for peer requests.

---

### Step 7: Start Peer 1

Open a third terminal:

```bash
cd Server
./peer
```

Peer 1 can:
- Query the Tracker for available topics.
- Download a topic from the Server.
- Register itself with the Tracker after a successful download.
- Serve downloaded topics to other peers.

---

### Step 8: Start Peer 2

Open a fourth terminal:

```bash
cd Server
./peer
```

Peer 2 can:
- Query the Tracker.
- Receive both the Server and Peer 1 as available hosts.
- Download the topic from either source.

---

### Expected Workflow

```text
Tracker
   ▲
   │
   │ QUERY / REGISTER_TOPIC
   │
Peer 1 --------------------► Server
   │                           │
   │                           │
   └──── Downloads Topic ◄─────┘
   │
   │ REGISTER_TOPIC
   ▼
Tracker

Peer 2
   │
   │ QUERY
   ▼
Tracker
   │
   │ Host List (Server + Peer 1)
   ▼
Peer 2
   │
   │ DOWNLOAD
   ▼
Peer 1
```

---

### Stopping the Application

Press **Ctrl + C** in each terminal to stop the Tracker, Server, and Peer processes.