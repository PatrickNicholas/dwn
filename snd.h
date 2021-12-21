#pragma once

struct elem_value_t {
    int count;
    long min, max;
    long values[2];
};

typedef int (*snd_event_callback_t)(const struct elem_value_t*, void*);

void set_playback_volume_change_callback(snd_event_callback_t cb, void* private);

void set_playback_switch_change_callback(snd_event_callback_t cb, void* private);

void set_capture_switch_change_callback(snd_event_callback_t cb, void* private);
