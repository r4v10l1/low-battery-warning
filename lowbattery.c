#include <stdio.h>
#include <sys/file.h>
#include <errno.h>
#include <libnotify/notify.h>

#define LOW_BATTERY_WARNING_THRESHOLD 20

char* readfile(char* base, char* file) {
    char path[512];
    char* line = (char*)malloc(512 * sizeof(char));
    memset(line, 0, 512 * sizeof(char));

    snprintf(path, 512, "%s/%s", base, file);
    FILE* fp = fopen(path, "r");

    if (fp == NULL)
        return NULL;
    if (fgets(line, 511, fp) == NULL)
        return NULL;

    fclose(fp);

    return line;
}

int main() {
    int cap;
    int charging = 0;

    char file[256];
    strcat(strcpy(file, getenv("HOME")), "/.lowbattery_lock");
    int pid_file = open(file, O_CREAT | O_RDWR, 0666);
    int rc       = flock(pid_file, LOCK_EX | LOCK_NB);
    if (rc) {
        // Another instance is already running
        if (EWOULDBLOCK == errno) {
            return 1;
        }
    }

    for (;; sleep(1)) {
        char* cp = readfile("/sys/class/power_supply/BAT1", "capacity");
        sscanf(cp, "%d", &cap);
        free(cp);

        cp = readfile("/sys/class/power_supply/BAT1", "status");
        if (!strncmp(cp, "Discharging", 11)) {
            if (cap <= LOW_BATTERY_WARNING_THRESHOLD && !charging &&
                notify_init("Low battery notification")) {
                charging = 0;

                NotifyNotification* notification =
                  notify_notification_new("Battery Low", "Connect charger", NULL);
                notify_notification_set_urgency(notification,
                                                NOTIFY_URGENCY_CRITICAL);
                notify_notification_show(notification, NULL);

                g_object_unref(notification);
                notify_uninit();
            }
        } else if (!strncmp(cp, "Charging", 8)) {
            if (!charging) {
                charging = 1;
            }
        }
        free(cp);
    }

    return 0;
}
