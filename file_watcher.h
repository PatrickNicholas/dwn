#pragma once

typedef int (*file_watcher_callback_t)(void*);

int add_file_watcher(const char* filename, file_watcher_callback_t cb, void* private);
