#include "common.h"

#define ISVALIDSOCKET(s) ((s) >= 0)
#define SOCKET int
#define INVALID_SOCKET 2
#define GETSOCKETERRNO() (errno)
#define CLOSESOCKET(s) close(s)

extern void process_game_state(const uint8_t *buf);
struct addrinfo *peer_address;
extern App app;
extern gamestate state;
client_info *clientInfo;

