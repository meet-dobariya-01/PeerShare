#include <bits/stdc++.h>
#include "Tracker.h"
#include "Host.h"

using namespace std;

int main()
{
    // Create Tracker
    Tracker tracker;

    // Create Server
    Host server(
        "127.0.0.1",
        8000,
        "Server/raw-img/"
    );

    // Create Peers
    Host peer1(
        "127.0.0.1",
        8001,
        "Peer1/downloads/"
    );

    Host peer2(
        "127.0.0.1",
        8002,
        "Peer2/downloads/"
    );

    Host peer3(
        "127.0.0.1",
        8003,
        "Peer3/downloads/"
    );

    Host peer4(
        "127.0.0.1",
        8004,
        "Peer4/downloads/"
    );

    Host peer5(
        "127.0.0.1",
        8005,
        "Peer5/downloads/"
    );

    // ===============================
    // Server registers all topics
    // ===============================

    tracker.registerTopic("Cane", server);
    tracker.registerTopic("Cavallo", server);
    tracker.registerTopic("Elefante", server);
    tracker.registerTopic("Farfalla", server);
    tracker.registerTopic("Gallina", server);
    tracker.registerTopic("Gatto", server);
    tracker.registerTopic("Mucca", server);
    tracker.registerTopic("Pecora", server);
    tracker.registerTopic("Ragno", server);
    tracker.registerTopic("Scoiattolo", server);

    // ===============================
    // Peer1 downloads
    // ===============================

    tracker.registerTopic("Cane", peer1);
    tracker.registerTopic("Cavallo", peer1);
    tracker.registerTopic("Elefante", peer1);

    // ===============================
    // Peer2 downloads
    // ===============================

    tracker.registerTopic("Gatto", peer2);
    tracker.registerTopic("Mucca", peer2);
    tracker.registerTopic("Pecora", peer2);

    // ===============================
    // Peer3 downloads
    // ===============================

    tracker.registerTopic("Ragno", peer3);
    tracker.registerTopic("Scoiattolo", peer3);
    tracker.registerTopic("Cane", peer3);

    // ===============================
    // Peer4 downloads
    // ===============================

    tracker.registerTopic("Gallina", peer4);
    tracker.registerTopic("Farfalla", peer4);
    tracker.registerTopic("Gatto", peer4);

    // ===============================
    // Peer5 downloads
    // ===============================

    tracker.registerTopic("Mucca", peer5);
    tracker.registerTopic("Elefante", peer5);
    tracker.registerTopic("Scoiattolo", peer5);

    // ===============================
    // Display Tracker Database
    // ===============================

    tracker.displayDatabase();

    // ===============================
    // Lookup Examples
    // ===============================

    cout << "\nSearching for Cane...\n";
    tracker.lookupTopic("Cane");

    cout << "\nSearching for Gatto...\n";
    tracker.lookupTopic("Gatto");

    cout << "\nSearching for Mucca...\n";
    tracker.lookupTopic("Mucca");

    cout << "\nSearching for Scoiattolo...\n";
    tracker.lookupTopic("Scoiattolo");

    cout << "\nSearching for Tiger...\n";
    tracker.lookupTopic("Tiger");

    return 0;
}