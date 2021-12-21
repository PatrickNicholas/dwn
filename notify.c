#include <libnotify/notify.h>

#include "module.h"

static const char *summary = "DWM";
static const char *icon = "dialog-information";

void show_notice(const char *buf) {
    NotifyNotification *msg = notify_notification_new(summary, buf, icon);
    notify_notification_show(msg, NULL);
    g_object_unref(G_OBJECT(msg));
}

static void init() { notify_init("dwn"); }

static void destroy() { notify_uninit(); }

DEFINE_KERNEL_MODULE(notify, init, destroy);
