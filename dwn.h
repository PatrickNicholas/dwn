#pragma once

#include <poll.h>


typedef int (*poll_callback_t)(void*, struct pollfd*);

int add_poll_sub(const struct pollfd* pfd, poll_callback_t cb, void* private);

