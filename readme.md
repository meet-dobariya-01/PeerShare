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
   │ LOOKUP Topic
   ▼
Tracker
   │
   │ Host List
   ▼
Peer
   │
   │ GET_TOPIC
   ▼
Server / Peer
   │
   │ Topic Files
   ▼
Peer
   │
   │ REGISTER
   ▼
Tracker
```

---

## Project Structure

```text
Topic-Based-P2P-File-Distribution/

├── tracker/
│   └── tracker.cpp
│
├── server/
│   ├── server.cpp
│   └── raw-img/
│
├── peer/
│   ├── peer.cpp
│   └── downloads/
│
├── README.md
└── FINAL.md
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