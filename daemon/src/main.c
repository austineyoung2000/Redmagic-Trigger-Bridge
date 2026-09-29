#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DEVICE_MODEL "NX809J"
#define SAR0_NAME "nubia_tgk_aw_sar0_ch0"
#define SAR1_NAME "nubia_tgk_aw_sar1_ch0"
#define TOUCH_NAME "synaptics_tcm_touch"
#define SAR0_MODE "/sys/class/leds/sar0/mode_operation"
#define SAR1_MODE "/sys/class/leds/sar1/mode_operation"
#define CONFIG_PATH "/data/adb/redmagic_trigger_bridge/config.conf"
#define ACTIVE_PATH "/data/adb/redmagic_trigger_bridge/active"
#define NORMALIZED_MAX 10000
#define SLOT_LEFT 0
#define SLOT_RIGHT 1

struct point {
    int x;
    int y;
};

struct config {
    bool enabled;
    bool grab_devices;
    bool swap_triggers;
    struct point targets[4][2];
};

struct bridge {
    int sar0_fd;
    int sar1_fd;
    int touch_fd;
    int uinput_fd;
    int x_max;
    int y_max;
    bool down[2];
    bool active;
    bool grabbed;
    int original_mode[2];
    bool original_mode_valid[2];
    int tracking_id[2];
    struct config config;
};

static volatile sig_atomic_t stop_requested;
static volatile sig_atomic_t reload_requested;

static void handle_stop_signal(int signal_number) {
    (void)signal_number;
    stop_requested = 1;
}

static void handle_reload_signal(int signal_number) {
    (void)signal_number;
    reload_requested = 1;
}

static void log_message(const char *level, const char *message) {
    time_t now = time(NULL);
    struct tm local_time;
    char timestamp[32] = {0};

    localtime_r(&now, &local_time);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time);
    fprintf(stderr, "%s [%s] %s\n", timestamp, level, message);
}

static bool read_line(const char *path, char *buffer, size_t size) {
    FILE *file = fopen(path, "re");
    if (file == NULL) {
        return false;
    }

    bool ok = fgets(buffer, (int)size, file) != NULL;
    fclose(file);
    if (!ok) {
        return false;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
    return true;
}

static bool is_supported_device(void) {
    char model[PROP_VALUE_MAX] = {0};
    char device[PROP_VALUE_MAX] = {0};

    __system_property_get("ro.product.model", model);
    __system_property_get("ro.product.device", device);
    return strcmp(model, DEVICE_MODEL) == 0 || strcmp(device, DEVICE_MODEL) == 0;
}

static int find_input_device(const char *wanted_name) {
    DIR *directory = opendir("/sys/class/input");
    if (directory == NULL) {
        return -1;
    }

    int result = -1;
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }

        char name_path[PATH_MAX];
        char event_name[128];
        int written = snprintf(name_path, sizeof(name_path),
                               "/sys/class/input/%s/device/name", entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(name_path) ||
            !read_line(name_path, event_name, sizeof(event_name)) ||
            strcmp(event_name, wanted_name) != 0) {
            continue;
        }

        char device_path[PATH_MAX];
        written = snprintf(device_path, sizeof(device_path),
                           "/dev/input/%s", entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(device_path)) {
            continue;
        }

        result = open(device_path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
        if (result >= 0) {
            break;
        }
    }

    closedir(directory);
    return result;
}

static int clamp_normalized(long value) {
    if (value < 0) {
        return 0;
    }
    if (value > NORMALIZED_MAX) {
        return NORMALIZED_MAX;
    }
    return (int)value;
}

static void default_config(struct config *config) {
    memset(config, 0, sizeof(*config));
    config->enabled = true;
    config->grab_devices = true;

    for (int rotation = 0; rotation < 4; ++rotation) {
        config->targets[rotation][SLOT_LEFT] = (struct point){2500, 5000};
        config->targets[rotation][SLOT_RIGHT] = (struct point){7500, 5000};
    }
}

static void set_config_value(struct config *config, const char *key, long value) {
    if (strcmp(key, "enabled") == 0) {
        config->enabled = value != 0;
        return;
    }
    if (strcmp(key, "grab_devices") == 0) {
        config->grab_devices = value != 0;
        return;
    }
    if (strcmp(key, "swap_triggers") == 0) {
        config->swap_triggers = value != 0;
        return;
    }

    for (int rotation = 0; rotation < 4; ++rotation) {
        for (int slot = 0; slot < 2; ++slot) {
            char expected[48];
            const char *side = slot == SLOT_LEFT ? "left" : "right";
            snprintf(expected, sizeof(expected), "rot%d_%s_x", rotation, side);
            if (strcmp(key, expected) == 0) {
                config->targets[rotation][slot].x = clamp_normalized(value);
                return;
            }
            snprintf(expected, sizeof(expected), "rot%d_%s_y", rotation, side);
            if (strcmp(key, expected) == 0) {
                config->targets[rotation][slot].y = clamp_normalized(value);
                return;
            }
        }
    }
}

static void load_config(struct config *config) {
    default_config(config);
    FILE *file = fopen(CONFIG_PATH, "re");
    if (file == NULL) {
        log_message("WARN", "configuration unavailable; using safe defaults");
        return;
    }

    char line[160];
    while (fgets(line, sizeof(line), file) != NULL) {
        char *cursor = line;
        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }
        if (*cursor == '#' || *cursor == '\n' || *cursor == '\0') {
            continue;
        }

        char *equals = strchr(cursor, '=');
        if (equals == NULL) {
            continue;
        }
        *equals = '\0';
        char *value_text = equals + 1;
        cursor[strcspn(cursor, " \t\r\n")] = '\0';

        errno = 0;
        char *end = NULL;
        long value = strtol(value_text, &end, 10);
        if (errno == 0 && end != value_text) {
            set_config_value(config, cursor, value);
        }
    }
    fclose(file);
}

static bool write_text(const char *path, const char *value) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    size_t length = strlen(value);
    ssize_t written = write(fd, value, length);
    int saved_errno = errno;
    close(fd);
    errno = saved_errno;
    return written == (ssize_t)length;
}

static bool read_mode(const char *path, int *mode) {
    char value[128];
    if (!read_line(path, value, sizeof(value))) {
        return false;
    }

    int parsed = -1;
    if (sscanf(value, "mode : %d", &parsed) != 1 ||
        (parsed != 0 && parsed != 1)) {
        return false;
    }
    *mode = parsed;
    return true;
}

static int configure_uinput(struct bridge *bridge) {
    struct input_absinfo x_info;
    struct input_absinfo y_info;
    if (ioctl(bridge->touch_fd, EVIOCGABS(ABS_MT_POSITION_X), &x_info) < 0 ||
        ioctl(bridge->touch_fd, EVIOCGABS(ABS_MT_POSITION_Y), &y_info) < 0) {
        return -1;
    }
    bridge->x_max = x_info.maximum;
    bridge->y_max = y_info.maximum;

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    int event_types[] = {EV_SYN, EV_KEY, EV_ABS};
    for (size_t index = 0; index < sizeof(event_types) / sizeof(event_types[0]); ++index) {
        if (ioctl(fd, UI_SET_EVBIT, event_types[index]) < 0) {
            close(fd);
            return -1;
        }
    }

    if (ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH) < 0 ||
        ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT) < 0) {
        close(fd);
        return -1;
    }

    int axes[] = {ABS_X, ABS_Y, ABS_MT_SLOT, ABS_MT_TRACKING_ID,
                  ABS_MT_POSITION_X, ABS_MT_POSITION_Y,
                  ABS_MT_TOUCH_MAJOR, ABS_MT_PRESSURE};
    for (size_t index = 0; index < sizeof(axes) / sizeof(axes[0]); ++index) {
        if (ioctl(fd, UI_SET_ABSBIT, axes[index]) < 0) {
            close(fd);
            return -1;
        }
    }

    struct uinput_user_dev device;
    memset(&device, 0, sizeof(device));
    snprintf(device.name, UINPUT_MAX_NAME_SIZE, "redmagic_trigger_bridge");
    device.id.bustype = BUS_VIRTUAL;
    device.id.vendor = 0x19d2;
    device.id.product = 0x8091;
    device.id.version = 1;
    device.absmin[ABS_X] = 0;
    device.absmax[ABS_X] = bridge->x_max;
    device.absmin[ABS_Y] = 0;
    device.absmax[ABS_Y] = bridge->y_max;
    device.absmin[ABS_MT_SLOT] = 0;
    device.absmax[ABS_MT_SLOT] = 1;
    device.absmin[ABS_MT_TRACKING_ID] = 0;
    device.absmax[ABS_MT_TRACKING_ID] = 65535;
    device.absmin[ABS_MT_POSITION_X] = 0;
    device.absmax[ABS_MT_POSITION_X] = bridge->x_max;
    device.absmin[ABS_MT_POSITION_Y] = 0;
    device.absmax[ABS_MT_POSITION_Y] = bridge->y_max;
    device.absmin[ABS_MT_TOUCH_MAJOR] = 0;
    device.absmax[ABS_MT_TOUCH_MAJOR] = 255;
    device.absmin[ABS_MT_PRESSURE] = 0;
    device.absmax[ABS_MT_PRESSURE] = 255;

    if (write(fd, &device, sizeof(device)) != (ssize_t)sizeof(device) ||
        ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }

    bridge->uinput_fd = fd;
    usleep(250000);
    return 0;
}

static bool emit_event(int fd, uint16_t type, uint16_t code, int32_t value) {
    struct input_event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    return write(fd, &event, sizeof(event)) == (ssize_t)sizeof(event);
}

static int current_rotation(void) {
    char value[PROP_VALUE_MAX] = {0};
    if (__system_property_get("sys.rm.trig_rot", value) <= 0) {
        return 0;
    }
    int rotation = atoi(value);
    return rotation >= 0 && rotation <= 3 ? rotation : 0;
}

static struct point raw_point(const struct bridge *bridge, int slot) {
    int rotation = current_rotation();
    struct point normalized = bridge->config.targets[rotation][slot];
    struct point result;

    switch (rotation) {
        case 1:
            result.x = normalized.y * bridge->x_max / NORMALIZED_MAX;
            result.y = (NORMALIZED_MAX - normalized.x) * bridge->y_max /
                       NORMALIZED_MAX;
            break;
        case 2:
            result.x = (NORMALIZED_MAX - normalized.x) * bridge->x_max /
                       NORMALIZED_MAX;
            result.y = (NORMALIZED_MAX - normalized.y) * bridge->y_max /
                       NORMALIZED_MAX;
            break;
        case 3:
            result.x = (NORMALIZED_MAX - normalized.y) * bridge->x_max /
                       NORMALIZED_MAX;
            result.y = normalized.x * bridge->y_max / NORMALIZED_MAX;
            break;
        default:
            result.x = normalized.x * bridge->x_max / NORMALIZED_MAX;
            result.y = normalized.y * bridge->y_max / NORMALIZED_MAX;
            break;
    }
    return result;
}

static void send_contact(struct bridge *bridge, int slot, bool pressed) {
    if (bridge->down[slot] == pressed) {
        return;
    }

    emit_event(bridge->uinput_fd, EV_ABS, ABS_MT_SLOT, slot);
    if (pressed) {
        struct point point = raw_point(bridge, slot);
        int tracking_id = ++bridge->tracking_id[slot];
        if (tracking_id <= 0) {
            tracking_id = slot + 1;
            bridge->tracking_id[slot] = tracking_id;
        }
        emit_event(bridge->uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, tracking_id);
        emit_event(bridge->uinput_fd, EV_ABS, ABS_MT_POSITION_X, point.x);
        emit_event(bridge->uinput_fd, EV_ABS, ABS_MT_POSITION_Y, point.y);
        emit_event(bridge->uinput_fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 32);
        emit_event(bridge->uinput_fd, EV_ABS, ABS_MT_PRESSURE, 128);
        emit_event(bridge->uinput_fd, EV_ABS, ABS_X, point.x);
        emit_event(bridge->uinput_fd, EV_ABS, ABS_Y, point.y);
        if (!bridge->down[SLOT_LEFT] && !bridge->down[SLOT_RIGHT]) {
            emit_event(bridge->uinput_fd, EV_KEY, BTN_TOUCH, 1);
        }
    } else {
        emit_event(bridge->uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
        if (!bridge->down[1 - slot]) {
            emit_event(bridge->uinput_fd, EV_KEY, BTN_TOUCH, 0);
        }
    }
    bridge->down[slot] = pressed;
    emit_event(bridge->uinput_fd, EV_SYN, SYN_REPORT, 0);
}

static void release_contacts(struct bridge *bridge) {
    send_contact(bridge, SLOT_LEFT, false);
    send_contact(bridge, SLOT_RIGHT, false);
}

static void deactivate_bridge(struct bridge *bridge) {
    release_contacts(bridge);

    if (bridge->grabbed) {
        ioctl(bridge->sar0_fd, EVIOCGRAB, 0);
        ioctl(bridge->sar1_fd, EVIOCGRAB, 0);
        bridge->grabbed = false;
    }

    const char *mode_paths[2] = {SAR0_MODE, SAR1_MODE};
    for (int index = 0; index < 2; ++index) {
        if (!bridge->original_mode_valid[index]) {
            continue;
        }
        const char *value = bridge->original_mode[index] == 0 ? "0\n" : "1\n";
        if (!write_text(mode_paths[index], value)) {
            log_message("WARN", "unable to restore trigger hardware mode");
        }
        bridge->original_mode_valid[index] = false;
    }

    if (bridge->active) {
        log_message("INFO", "trigger bridge deactivated");
    }
    bridge->active = false;
}

static bool activate_bridge(struct bridge *bridge) {
    if (bridge->active) {
        return true;
    }

    bridge->original_mode_valid[0] = read_mode(SAR0_MODE,
                                                &bridge->original_mode[0]);
    bridge->original_mode_valid[1] = read_mode(SAR1_MODE,
                                                &bridge->original_mode[1]);

    if (!write_text(SAR0_MODE, "1\n") || !write_text(SAR1_MODE, "1\n")) {
        log_message("ERROR", "unable to arm shoulder-trigger hardware");
        deactivate_bridge(bridge);
        return false;
    }

    if (bridge->config.grab_devices) {
        if (ioctl(bridge->sar0_fd, EVIOCGRAB, 1) < 0) {
            log_message("ERROR", "unable to grab SAR0 trigger device");
            deactivate_bridge(bridge);
            return false;
        }
        bridge->grabbed = true;
        if (ioctl(bridge->sar1_fd, EVIOCGRAB, 1) < 0) {
            log_message("ERROR", "unable to grab SAR1 trigger device");
            deactivate_bridge(bridge);
            return false;
        }
    }

    bridge->active = true;
    log_message("INFO", "trigger bridge activated");
    return true;
}

static void process_input(struct bridge *bridge, int fd, int expected_code,
                          int configured_slot) {
    struct input_event events[16];
    ssize_t bytes = read(fd, events, sizeof(events));
    if (bytes <= 0) {
        return;
    }

    size_t count = (size_t)bytes / sizeof(events[0]);
    int slot = bridge->config.swap_triggers ? 1 - configured_slot : configured_slot;
    for (size_t index = 0; index < count; ++index) {
        if (bridge->active && events[index].type == EV_KEY &&
            events[index].code == expected_code) {
            send_contact(bridge, slot, events[index].value != 0);
        }
    }
}

static void close_bridge(struct bridge *bridge) {
    deactivate_bridge(bridge);
    if (bridge->uinput_fd >= 0) {
        ioctl(bridge->uinput_fd, UI_DEV_DESTROY);
        close(bridge->uinput_fd);
    }
    if (bridge->sar0_fd >= 0) close(bridge->sar0_fd);
    if (bridge->sar1_fd >= 0) close(bridge->sar1_fd);
    if (bridge->touch_fd >= 0) close(bridge->touch_fd);
}

int main(void) {
    struct bridge bridge;
    memset(&bridge, 0, sizeof(bridge));
    bridge.sar0_fd = -1;
    bridge.sar1_fd = -1;
    bridge.touch_fd = -1;
    bridge.uinput_fd = -1;

    if (!is_supported_device()) {
        log_message("ERROR", "unsupported device; expected NX809J");
        return EXIT_FAILURE;
    }

    load_config(&bridge.config);
    if (!bridge.config.enabled) {
        log_message("INFO", "bridge disabled by configuration");
        return EXIT_SUCCESS;
    }

    bridge.sar0_fd = find_input_device(SAR0_NAME);
    bridge.sar1_fd = find_input_device(SAR1_NAME);
    bridge.touch_fd = find_input_device(TOUCH_NAME);
    if (bridge.sar0_fd < 0 || bridge.sar1_fd < 0 || bridge.touch_fd < 0) {
        log_message("ERROR", "required input device was not found");
        close_bridge(&bridge);
        return EXIT_FAILURE;
    }

    if (configure_uinput(&bridge) < 0) {
        log_message("ERROR", "unable to create virtual touchscreen");
        close_bridge(&bridge);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_stop_signal);
    signal(SIGTERM, handle_stop_signal);
    signal(SIGHUP, handle_reload_signal);
    signal(SIGUSR1, handle_reload_signal);
    log_message("INFO", "trigger bridge started inactive");

    struct pollfd poll_fds[2] = {
        {.fd = bridge.sar0_fd, .events = POLLIN},
        {.fd = bridge.sar1_fd, .events = POLLIN},
    };
    time_t next_activation_attempt = 0;

    while (!stop_requested) {
        if (reload_requested) {
            deactivate_bridge(&bridge);
            load_config(&bridge.config);
            next_activation_attempt = 0;
            reload_requested = 0;
        }

        bool should_be_active = bridge.config.enabled &&
                                access(ACTIVE_PATH, F_OK) == 0;
        time_t now = time(NULL);
        if (should_be_active && !bridge.active &&
            now >= next_activation_attempt) {
            if (!activate_bridge(&bridge)) {
                next_activation_attempt = now + 2;
            }
        } else if (!should_be_active && bridge.active) {
            deactivate_bridge(&bridge);
            next_activation_attempt = 0;
        }

        int result = poll(poll_fds, 2, 250);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            log_message("ERROR", "input polling failed");
            break;
        }
        if ((poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 ||
            (poll_fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            log_message("ERROR", "trigger input device disconnected");
            break;
        }
        if ((poll_fds[0].revents & POLLIN) != 0) {
            process_input(&bridge, bridge.sar0_fd, KEY_F7, SLOT_LEFT);
        }
        if ((poll_fds[1].revents & POLLIN) != 0) {
            process_input(&bridge, bridge.sar1_fd, KEY_F8, SLOT_RIGHT);
        }
    }

    close_bridge(&bridge);
    log_message("INFO", "trigger bridge stopped cleanly");
    return EXIT_SUCCESS;
}
