# BitTorrent Client

Welcome to my BitTorrent Client project! This project aims to create a fully functional BitTorrent client capable of downloading publicly available files using the BitTorrent protocol. This client will designed to interoperate seamlessly with commercial and open-source BitTorrent clients, ensuring robust performance and compatibility. The main reference as to which this is built is Transmission.

## Overview
BitTorrent is a peer-to-peer file sharing protocol used for distributing large amounts of data efficiently. This client is built to show the core functionalities of the BitTorrent protocol while offering additional features that could enhance performance and user experience.

## Features--(TODO)
Some core features include

- Tracker Communication: Communicates with the tracker using HTTP, with support for the compact format.
- Peer-to-Peer Downloading:
    - Downloads files from other instances of this client.
    - Downloads files from official BitTorrent clients, ensuring interoperability and smooth performance.

Some more advanced features that I plan to add include:

- UDP-Tracker Protocol: Implementing support for the UDP-tracker protocol for enhanced tracker communication.
- Optimistic Unchoking: Implementing optimistic unchoking to improve the chances of finding new peers and increasing download speeds.
- Rarest-First Strategy: Implementing the rarest-first piece selection strategy to optimize the download process.
- Endgame Mode: Implementing endgame mode to handle the final pieces of the download efficiently.
- BitTyrant Mode: Implementing BitTyrant mode, an alternative and possibly efficient peer selection strategy that could help improving download performance.
- PropShare: To implement the PropShare algorithm to compare performance with "Transmission".

## Design and Implementation
Design Choices

- Modular Architecture: The client is built with a modular architecture in mind, separating functionalities into different part such as tracker communication, peer management, and file handling.
- Most of the libraries here(excluding built in ones) are built from scratch. OpenSSL is the only library used so far for hashing, which I did not build. A lot of testing and is still required, so please, do not use most of this code blindly without proper checking.

## Implementation Details
- bencoding library
- HTTP Tracker Communication: Uses HTTP for communicating with the tracker, handling responses in compact format.
- Peer Management: Efficient peer management to handle multiple peer connections and maintain a stable download performance.
- Piece Selection Strategies: Implements various piece selection strategies to optimize download performance.

## Challenges and Solutions
### Problems Encountered so far

- Handling Null Bytes in SHA1 Hashes: Encountered issues with processing the 'pieces' field due to null bytes in SHA1 hashes. Addressed by properly handling binary data in the implementation.


### Known Issues
- 

## Contributions

Sinmi: Primary developer(me), responsible for core implementation, design decisions, and addressing key challenges. If you would like to assist with this, feel free to let me know please!

## Experiments and Performance Analysis
- 

## Getting Started
### Prerequisites
- Linux OS
- C Compiler: GCC or any compatible C compiler.
- OpenSSL: For SHA1.
- Libcurl: For HTTP communication.

### Building the Client
- 

## References
- BitTorrent Protocol Specification
- BitTyrant: A New BitTorrent Client
- PropShare: A Strategy for Efficient P2P Sharing



### MORE READINGS

#### Bencoding
Bencode (pronounced Bee-encode) is a serialization format used in the BitTorrent protocol. It is used in torrent files and in communication between trackers and peers.

Bencode supports four data types:

    strings
    integers
    arrays
    dictionaries
This serialization logic is implemented in bencode.c
