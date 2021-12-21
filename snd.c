#include "snd.h"

#include <alsa/asoundlib.h>

#include "dwn.h"
#include "module.h"

#define MAX_CARDS 256

snd_ctl_t* ctls[MAX_CARDS];
size_t ncards = 0;
struct elem_value_t playback_volume, playback_switch, capture_switch;
snd_event_callback_t playback_volume_callback, playback_switch_callback, capture_switch_callback;

void set_playback_volume_change_callback(snd_event_callback_t cb, void* private) {
    playback_volume_callback = cb;
}

void set_playback_switch_change_callback(snd_event_callback_t cb, void* private) {
    playback_switch_callback = cb;
}

void set_capture_switch_change_callback(snd_event_callback_t cb, void* private) {
    capture_switch_callback = cb;
}

void close_all_snds() {
    for (ncards -= 1; ncards >= 0; --ncards) {
        snd_ctl_close(ctls[ncards]);
    }
}

int init_snd_ctls(const char* name) {
    int err = 0;
    int card = -1;
    char cardname[16];

    if (!name) {
        while (snd_card_next(&card) >= 0 && card >= 0) {
            if (ncards >= MAX_CARDS) {
                fprintf(stderr, "alsactl: too many cards\n");
                err = -E2BIG;
                break;
            }

            sprintf(cardname, "hw:%d", card);
            err = snd_ctl_open(&ctls[ncards], cardname, SND_CTL_READONLY);
            if (err < 0) {
                fprintf(stderr, "Cannot open ctl %s\n", cardname);
                break;
            }

            printf("found %s\n", cardname);

            ncards += 1;
            err = snd_ctl_subscribe_events(ctls[ncards - 1], 1);
            if (err < 0) {
                fprintf(stderr, "Cannot open subscribe events to ctl %s\n", cardname);
                break;
            }
        }
    } else {
        err = snd_ctl_open(&ctls[0], name, SND_CTL_READONLY);
        if (err < 0) {
            fprintf(stderr, "Cannot open ctl %s\n", name);
            return err;
        }

        ncards++;
        err = snd_ctl_subscribe_events(ctls[ncards - 1], 1);
        if (err < 0) {
            fprintf(stderr, "Cannot open subscribe events to ctl %s\n", cardname);
        }
    }

    if (err != 0) {
        close_all_snds();
    }

    return err;
}

void dump(snd_ctl_t* ctl) {
    snd_ctl_card_info_t* info;
    snd_ctl_card_info_alloca(&info);
    snd_ctl_card_info(ctl, info);
    printf("card name: %s\n", snd_ctl_card_info_get_name(info));

    snd_ctl_elem_list_t* list;
    snd_ctl_elem_list_malloc(&list);
    snd_ctl_elem_list_alloc_space(list, 100);

    snd_ctl_elem_list(ctl, list);
    int used = snd_ctl_elem_list_get_used(list);
    printf("list used: %d\n", used);

    snd_ctl_elem_info_t* elem_info;
    snd_ctl_elem_info_malloc(&elem_info);

    snd_ctl_elem_value_t* value;
    snd_ctl_elem_value_malloc(&value);
    for (int i = 0; i < used; ++i) {
        int numid = snd_ctl_elem_list_get_numid(list, i);
        printf("\tnumid: %d\n", numid);
        snd_ctl_elem_info_set_numid(elem_info, numid);
        snd_ctl_elem_info(ctl, elem_info);

        int count = snd_ctl_elem_info_get_count(elem_info);
        printf("\tname: %s\n", snd_ctl_elem_info_get_name(elem_info));
        printf("\t\tiface: %d\n", snd_ctl_elem_info_get_interface(elem_info));
        printf("\t\ttype: %d\n", snd_ctl_elem_info_get_type(elem_info));
        printf("\t\tvalue count: %d\n", snd_ctl_elem_info_get_count(elem_info));

        snd_ctl_elem_value_set_numid(value, numid);
        snd_ctl_elem_read(ctl, value);

        unsigned type = snd_ctl_elem_info_get_type(elem_info);
        switch (type) {
            case SND_CTL_ELEM_TYPE_INTEGER:
                printf("\t\tmin: %ld\n", snd_ctl_elem_info_get_min(elem_info));
                printf("\t\tmax: %ld\n", snd_ctl_elem_info_get_max(elem_info));
                for (int i = 0; i < count; ++i) {
                    printf("\t\tvalue %d: %ld\n", i, snd_ctl_elem_value_get_integer(value, i));
                }
                break;
            case SND_CTL_ELEM_TYPE_BOOLEAN:
                for (int i = 0; i < count; ++i) {
                    printf("\t\tvalue %d: %d\n", i, snd_ctl_elem_value_get_boolean(value, i));
                }
        }
    }

    snd_ctl_elem_value_free(value);
    snd_ctl_elem_info_free(elem_info);
    snd_ctl_elem_list_free(list);
}

void read_elem_value(snd_ctl_t* ctl, const char* elem_name, struct elem_value_t* out) {
    /* https://github.com/alsa-project/alsa-lib/blob/master/include/control.h */
    bzero(out, sizeof(struct elem_value_t));

    snd_ctl_elem_info_t* elem_info;
    snd_ctl_elem_info_malloc(&elem_info);
    snd_ctl_elem_info_set_interface(elem_info, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_info_set_name(elem_info, elem_name);
    snd_ctl_elem_info(ctl, elem_info);

    snd_ctl_elem_value_t* value;
    snd_ctl_elem_value_malloc(&value);
    snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_value_set_name(value, elem_name);
    snd_ctl_elem_read(ctl, value);

    out->count = snd_ctl_elem_info_get_count(elem_info);
    switch (snd_ctl_elem_info_get_type(elem_info)) {
        case SND_CTL_ELEM_TYPE_INTEGER:
            out->min = snd_ctl_elem_info_get_min(elem_info);
            out->max = snd_ctl_elem_info_get_max(elem_info);
            for (int i = 0; i < out->count && i < 2; ++i)
                out->values[i] = snd_ctl_elem_value_get_integer(value, i);
            break;
        case SND_CTL_ELEM_TYPE_BOOLEAN:
            for (int i = 0; i < out->count && i < 2; ++i)
                out->values[i] = snd_ctl_elem_value_get_boolean(value, i);
            break;
        default:
            /* ignore */
            break;
    }

    snd_ctl_elem_info_free(elem_info);
    snd_ctl_elem_value_free(value);
}

int is_elem_value_changed(const struct elem_value_t* old, const struct elem_value_t* current) {
    for (int i = 0; i < old->count && i < 2; ++i) {
        if (old->values[i] != current->values[i]) return 1;
    }
    return 0;
}

int maybe_update_elem_value(snd_ctl_t* ctl, const char* name, struct elem_value_t* old) {
    struct elem_value_t value;
    read_elem_value(ctl, name, &value);
    if (is_elem_value_changed(old, &value)) {
        memcpy(old, &value, sizeof(struct elem_value_t));
        return 1;
    }
    return 0;
}

int snd_event_handler(void* private, struct pollfd* pfd) {
    unsigned short revents;
    snd_ctl_event_t* event;
    snd_ctl_t* ctl = private;

    snd_ctl_poll_descriptors_revents(ctl, pfd, 1, &revents);
    if (revents & POLLIN) {
        snd_ctl_event_alloca(&event);
        if (snd_ctl_read(ctl, event) > 0 && snd_ctl_event_get_type(event) == SND_CTL_EVENT_ELEM &&
            snd_ctl_event_elem_get_mask(event) & SND_CTL_EVENT_MASK_VALUE) {
            if (maybe_update_elem_value(ctl, "Master Playback Volume", &playback_volume) &&
                playback_volume_callback)
                playback_volume_callback(&playback_volume, NULL);
            if (maybe_update_elem_value(ctl, "Master Playback Switch", &playback_switch) &&
                playback_switch_callback)
                playback_switch_callback(&playback_switch, NULL);
            if (maybe_update_elem_value(ctl, "Capture Switch", &capture_switch) &&
                capture_switch_callback)
                capture_switch_callback(&capture_switch, NULL);
        }
    }

    return 0;
}

void init_snd() {
    struct pollfd pfd;

    init_snd_ctls("default");
    for (size_t i = 0; i < ncards; ++i) {
        // FOR DEBUG:
        // dump(ctls[i]);

        read_elem_value(ctls[i], "Master Playback Volume", &playback_volume);
        read_elem_value(ctls[i], "Master Playback Switch", &playback_switch);
        read_elem_value(ctls[i], "Capture Switch", &capture_switch);

        snd_ctl_poll_descriptors(ctls[i], &pfd, 1);
        if (add_poll_sub(&pfd, snd_event_handler, ctls[i]) == -1) {
            fprintf(stderr, "fail to add snd event handler\n");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_snd() { close_all_snds(); }

DEFINE_KERNEL_MODULE(snd, init_snd, destroy_snd);
