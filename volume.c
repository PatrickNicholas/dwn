#include <stdio.h>

#include "module.h"
#include "snd.h"
#include "notify.h"

static int on_playback_volume_change(const struct elem_value_t *ev, void *private) {
    long volume = ev->values[0];
    long percent = volume * 100.0 / ev->max;

    if (ev->count == 2) {
        volume += ev->values[1];
        volume >>= 1;
    }

    char buf[256];
    sprintf(buf, "change volume to %ld%%\n", percent);
    show_notice(buf);

    return 0;
}

static int on_playback_switch_change(const struct elem_value_t *ev, void *private) {
    char buf[256];
    sprintf(buf, "set playback switch to %s\n", ev->values[0] ? "on" : "off");
    show_notice(buf);
    return 0;
}

void init() {
    set_playback_volume_change_callback(on_playback_volume_change, NULL);
    set_playback_switch_change_callback(on_playback_switch_change, NULL);
}

void destroy() {}

DEFINE_CONFIG_MODULE(volume, init, destroy);
