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
    int cap0, cap1;
    char *buf0, *buf1;

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
        buf0 = readfile("/sys/class/power_supply/BAT0", "capacity");
        sscanf(buf0, "%d", &cap0);
        free(buf0);

        // We could use the same buffer for cap1 but this way is cleaner
        buf1 = readfile("/sys/class/power_supply/BAT1", "capacity");
        sscanf(buf1, "%d", &cap1);
        free(buf1);

        // If both batteries are charged, stop
        if (cap0 > LOW_BATTERY_WARNING_THRESHOLD &&
            cap1 > LOW_BATTERY_WARNING_THRESHOLD)
            continue;

        buf0 = readfile("/sys/class/power_supply/BAT0", "status");
        buf1 = readfile("/sys/class/power_supply/BAT1", "status");

        // If any of the batteries is charging, stop. We don't care if the second one
        // is empty as long as we have the first one.
        if (strncmp(buf0, "Charging", 8) == 0 || strncmp(buf1, "Charging", 8) == 0)
            continue;

        // Actual notification if no batteries are charging and one is low enough
        if (notify_init("Low battery notification")) {
            NotifyNotification* notification =
              notify_notification_new("Battery Low", "Connect charger", NULL);
            notify_notification_set_urgency(notification, NOTIFY_URGENCY_CRITICAL);
            notify_notification_show(notification, NULL);

            g_object_unref(notification);
            notify_uninit();
        }

        free(buf0);
        free(buf1);
    }

    return 0;
}
