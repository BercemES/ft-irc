*This project has been created as part of the 42 curriculum by gbodur, bekinci-, mdivan.*

# ft_irc

## Description

**ft_irc** is an Internet Relay Chat (IRC) server built from scratch in full compliance with C++98 standards by the team of three(`gbodur`, `bekinci-`, `mdivan`) as part of the 42 School curriculum. This project aims to thoroughly study and implement core network programming concepts, socket management, and application-layer protocols while strictly adhering to the project subject.

Intentionally excluding concurrency mechanisms provided by modern systems—such as multithreading or `fork()`—a strictly non-blocking I/O architecture was designed to manage hundreds of clients within a single main event loop. Developed based on the RFC 1459 and RFC 2812 standards, the system delivers high-performance and uninterrupted data streaming through `poll()`-based I/O multiplexing.

Within the scope of the project, robust security mechanisms were engineered against network flood attacks and buffer overflows that suspended clients (`^Z`) might cause. In particular, a custom memory release architecture (`_pendingRemovals`) was developed to prevent Iterator Invalidation on `std::map` during network broadcasting. This approach completely eliminated the risk of server crashes (Segmentation Faults) and ensured that the project was completed with zero memory leaks.


## Instructions

### Installation and Execution
1. **Clone the Repository:**
   - Clone the project repository to your local machine using `git`:
   - git clone `<repository-url>`
   - cd ft_irc
2. **Compile the Project:**
	`make`
3. **Run the Server:**
	`./ircserv <port> <password>`
	Example: `./ircserv 6667 mypassword`

### How to Use
* You can connect to the server using any standard IRC client (such as **Irssi**) or by using **Netcat (`nc`)** for raw socket testing and debugging.

#### Connecting with Netcat (`nc`):
* `nc 127.0.0.1 <port>`
* ```
	PASS <your_password>
	NICK <your_nickname>
	USER <username> 0 * :<Real Name>
	```
* Upon successful registration, the server will respond with the 001 RPL_WELCOME along with the 002-005 Welcome numerics. You can then begin interacting with the server using standard IRC application-layer commands:
	- JOIN #channel [key]: Join or create a channel.
	- PRIVMSG <target> :<message>: Send a message to a user or channel.
	- TOPIC #channel [topic]: View or change the channel topic.
	- INVITE <nickname> #channel: Invite a user to a channel.
	- KICK #channel <nickname> [reason]: Remove a user from a channel (Operator only).
	- MODE #channel <+|-flags> [params]: Manage channel modes (i, t, k, l, o).
	- PART #channel [reason]: Leave a channel.
	- QUIT [reason]: Disconnect from the server.

### Submission Details

* **Language Standard & Compiler Flags:** Developed strictly in C++98 and compiled using `c++` with `-Wall -Wextra -Werror -std=c++98` flags.
* **Socket Configuration:** All active sockets (both listening and client file descriptors) are explicitly set to non-blocking mode (`O_NONBLOCK`) using `fcntl()`.
* **I/O Multiplexing Architecture:** Engineered around a single, centralized `poll()` event loop without using multithreading or process forking.
* **Memory & Disconnect Safety:** Features a deferred removal system (`_pendingRemovals`) to process client disconnects and broadcast cleanups safely outside active iteration loops, eliminating *Iterator Invalidation* risks on internal data structures (`std::map`).
* **Resource Management:** Rigorously tested under Valgrind to ensure zero memory leaks and zero file descriptor leaks throughout the server lifecycle and upon shutdown.

## Resources

### Networking Concepts Studied
Throughout the development of this project, we conducted in-depth research and practical implementation of the following core network and system programming concepts:

* **TCP/IP Socket Lifecycle:** Mastery of the foundational socket system calls including `socket()`, `bind()`, `listen()`, `accept()`, `recv()`, and `send()` for reliable stream-based communication.
* **Non-Blocking I/O:** Configuring all file descriptors with `fcntl(fd, F_SETFL, O_NONBLOCK)` to ensure the server never hangs while waiting for data.
* **I/O Multiplexing (`poll`):** Efficiently managing hundreds of simultaneous connections on a single thread by monitoring `POLLIN` and `POLLOUT` events without the use of threading or forking.
* **Event-Driven State Machine:** Managing partial packet deliveries and TCP stream fragmentation through dynamic buffers, entirely decoupled from `errno` polling (`EAGAIN`/`EWOULDBLOCK`) to meet strict evaluation guidelines.
* **Safe Memory Management (Iterator Invalidation):** Designing a deferred-deletion mechanism (`_pendingRemovals`) to safely destroy disconnected clients outside of `std::map` iteration cycles, successfully preventing Segmentation Faults during mass broadcasts.
* **Signal Handling:** Preventing unexpected server crashes caused by broken pipes (e.g., when a client abruptly closes the terminal) by explicitly ignoring the `SIGPIPE` signal (`signal(SIGPIPE, SIG_IGN)`).


### AI Usage
In this project, Artificial Intelligence (AI) tools were utilized as a guide and auditor rather than a direct code generator:

* **Conceptual Research:** During the initial phases of the project, AI was heavily used to research and comprehend complex technical concepts, including network programming, socket mechanics, and IRC protocol standards (RFCs).
* **Comprehensive Testing & Final Audit:** In addition to our internal testing and the rigorous peer tests conducted by our fellow 42 School students, AI served as a "final auditor" for the project.

### References
* 42 ft_irc Subject PDF
* [Modern IRC Client Protocol](https://modern.ircdocs.horse/)
* [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
* *RFC 1459 & RFC 2812 Specifications:** Official Internet Relay Chat protocol specifications published by the IETF.
* [Internet Relay Chat (IRC)](https://www.geeksforgeeks.org/computer-networks/internet-relay-chat-irc/)
* [Network Layer in OSI Model](https://www.geeksforgeeks.org/computer-networks/network-layer-in-osi-model/)
* [Difference Between IPv4 and IPv6](https://www.geeksforgeeks.org/computer-networks/differences-between-ipv4-and-ipv6/)
* [Socket Programming](https://medium.com/@gaurav290802/socket-programming-101-cdfd343f3028)
* [what is Socket Programming](https://medium.com/@veysel.sebu.23/socket-programlama-nedir-3a9af665f3e7)

## Acknowledgements
We would like to thank all 42 Istanbul students who shared their knowledge with us during our ft_irc learning process.
