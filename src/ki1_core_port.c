#include "ki1_core_port.h"

#include "cpu.h"
#include "game.h"
#include "ide.h"
#include "mem.h"
#include "rec.h"
#include "sound.h"
#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define KI1_CPU_HZ 100000000
#define KI1_FRAME_HZ 58.9634
#define KI1_SCANLINES 261
#define KI1_VISIBLE_LINES 240
#define KI1_CYCLES_PER_LINE ((int)(KI1_CPU_HZ / KI1_FRAME_HZ / KI1_SCANLINES))
#define KI1_ROM_SIZE 0x80000u

static struct {
    int sound_acc;
    uint64_t frame;
} machine;

static char game_directory[4096];
static bool core_ready;

static const char *const data_directories[] = {
    "/data/KillerPs4/roms/kinst",
    "/data/KillerPs4/kinst",
    "/data/KillerPs4",
    "/mnt/usb0/KillerPs4/roms/kinst",
    "/mnt/usb0/KillerPs4/kinst",
    "/mnt/usb0/KillerPs4",
    "/mnt/usb1/KillerPs4/roms/kinst",
    "/mnt/usb1/KillerPs4/kinst",
    "/mnt/usb1/KillerPs4",
    NULL
};

static bool make_candidate(const char *directory, const char *name,
                           char *output, size_t capacity)
{
    int written = snprintf(output, capacity, "%s/%s", directory, name);
    if (written < 0 || (size_t)written >= capacity) return false;

    FILE *file = fopen(output, "rb");
    if (!file) return false;
    fclose(file);
    return true;
}

static bool find_game_file(const char *name, char *output, size_t capacity,
                           char *found_directory, size_t directory_capacity)
{
    for (size_t i = 0; data_directories[i]; ++i) {
        if (make_candidate(data_directories[i], name, output, capacity)) {
            if (found_directory && directory_capacity) {
                snprintf(found_directory, directory_capacity, "%s", data_directories[i]);
            }
            return true;
        }
    }
    return false;
}

static bool load_boot_rom(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) return false;

    const size_t count = fread(rom, 1, KI1_ROM_SIZE, file);
    const int extra = fgetc(file);
    fclose(file);
    return count == KI1_ROM_SIZE && extra == EOF;
}

static void run_machine_frame(void)
{
    machine.frame++;

    for (int line = 0; line < KI1_SCANLINES; ++line) {
        if (line == KI1_VISIBLE_LINES) cpu_set_irq(0, true);
        if (line == 0) cpu_set_irq(0, false);

        int remaining = KI1_CYCLES_PER_LINE;
        while (remaining > 0) {
            int slice = remaining < 256 ? remaining : 256;
            int ran = (int)cpu_run((uint64_t)slice);

            if (cpu_idle) {
                cpu_skip((uint64_t)(remaining - ran));
                slice = remaining;
            }

            ide_tick(slice);
            machine.sound_acc += slice;
            sound_run(machine.sound_acc / 10);
            machine.sound_acc %= 10;
            remaining -= slice;
        }
    }
}

uint64_t machine_frame(void)
{
    return machine.frame;
}

size_t machine_state(uint8_t *buffer, bool load)
{
    if (buffer && load) {
        memcpy(&machine, buffer, sizeof machine);
        memcpy(&hw, buffer + sizeof machine, sizeof hw);
    } else if (buffer) {
        memcpy(buffer, &machine, sizeof machine);
        memcpy(buffer + sizeof machine, &hw, sizeof hw);
    }
    return sizeof machine + sizeof hw;
}

static bool file_size_is(const char *directory, const char *name, long expected)
{
    char path[4096];
    const int n = snprintf(path, sizeof path, "%s/%s", directory, name);
    if (n < 0 || (size_t)n >= sizeof path) return false;
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    const bool valid = fseek(file, 0, SEEK_END) == 0 && ftell(file) == expected;
    fclose(file);
    return valid;
}

static bool file_exists(const char *directory, const char *name,
                        char *output, size_t capacity)
{
    const int n = snprintf(output, capacity, "%s/%s", directory, name);
    if (n < 0 || (size_t)n >= capacity) return false;
    FILE *file = fopen(output, "rb");
    if (!file) return false;
    fclose(file);
    return true;
}

bool ki1_core_init_from_directory(const char *directory,
                                  char *error, size_t error_capacity)
{
    if (core_ready) return true;
    if (!directory || !directory[0] || !error || error_capacity == 0) return false;
    error[0] = '\0';

    struct stat directory_stat;
    if (stat(directory, &directory_stat) != 0 || !S_ISDIR(directory_stat.st_mode)) {
        snprintf(error, error_capacity, "Selected location is not a folder.");
        return false;
    }

    game_select(&game_ki1);
    if (strlen(directory) >= sizeof game_directory) {
        snprintf(error, error_capacity, "Selected folder path is too long.");
        return false;
    }
    snprintf(game_directory, sizeof game_directory, "%s", directory);

    char boot_path[4096] = "";
    if (!file_size_is(directory, "ki-l15d.u98", KI1_ROM_SIZE)) {
        if (!file_size_is(directory, "ki-l14.u98", KI1_ROM_SIZE)) {
            snprintf(error, error_capacity,
                     "Missing boot ROM: ki-l15d.u98 or ki-l14.u98 (512 KiB).");
            return false;
        }
        (void)file_exists(directory, "ki-l14.u98", boot_path, sizeof boot_path);
    } else {
        (void)file_exists(directory, "ki-l15d.u98", boot_path, sizeof boot_path);
    }

    for (int i = 0; i < 8; ++i) {
        if (!file_size_is(directory, game.sound_roms[i], KI1_ROM_SIZE)) {
            snprintf(error, error_capacity,
                     "Missing or invalid sound ROM: %s (expected 512 KiB).",
                     game.sound_roms[i]);
            return false;
        }
    }

    char disk_path[4096] = "";
    if (!file_exists(directory, "kinst.chd", disk_path, sizeof disk_path) &&
        !file_exists(directory, "kinst.raw", disk_path, sizeof disk_path)) {
        snprintf(error, error_capacity, "Missing kinst.chd or kinst.raw in this folder.");
        return false;
    }

    if (!load_boot_rom(boot_path)) {
        snprintf(error, error_capacity, "Could not read the selected boot ROM.");
        return false;
    }

    // Save settings in the app's writable data area, not on the selected USB.
    (void)mkdir("/data/KillerPs4", 0777);
    const char *settings_path = "/data/KillerPs4/ki1.settings";
    struct stat settings_stat;
    if (stat("/data/KillerPs4", &settings_stat) != 0 ||
        !S_ISDIR(settings_stat.st_mode)) {
        settings_path = NULL;
    }

    if (!ide_open(disk_path, settings_path, game.cyls, game.heads,
                  game.secs, game.serial_offset)) {
        snprintf(error, error_capacity, "Could not open the selected CHD/raw disk image.");
        return false;
    }

    if (!sound_init(game_directory)) {
        snprintf(error, error_capacity, "Could not load all eight DCS sound ROMs.");
        return false;
    }

    hw.dsw = 0xffffffffu;
    cpu_reset();
    rec_reset();
    rec_init();
    state_init();

    core_ready = true;
    return true;
}

bool ki1_core_init(char *error, size_t error_capacity)
{
    if (core_ready) return true;
    if (!error || error_capacity == 0) return false;
    error[0] = '\0';

    for (size_t i = 0; data_directories[i]; ++i) {
        char boot_path[4096];
        if (file_exists(data_directories[i], "ki-l15d.u98", boot_path, sizeof boot_path) ||
            file_exists(data_directories[i], "ki-l14.u98", boot_path, sizeof boot_path)) {
            return ki1_core_init_from_directory(data_directories[i], error, error_capacity);
        }
    }
    snprintf(error, error_capacity, "Select the folder containing your Killer Instinct files.");
    return false;
}

void ki1_core_run_frame(uint16_t player1_buttons, uint16_t player2_buttons)
{
    if (!core_ready) return;

    // The arcade input ports are active-low. Bit 0x001 is the first action
    // button; direction, start and coin bits use the original board mapping.
    hw.p1 = 0xffffffffu & ~(uint32_t)(player1_buttons & 0x0fffu);
    hw.p2 = 0xffffffffu & ~(uint32_t)(player2_buttons & 0x0fffu);

    run_machine_frame();
    state_checkpoint();
}

void ki1_core_copy_rgb(uint32_t *destination)
{
    if (!destination || !core_ready) return;

    const uint16_t *source = video_base();
    for (int y = 0; y < 240; ++y) {
        for (int x = 0; x < 320; ++x) {
            const uint16_t pixel = source[y * 320 + x];
            const uint32_t red = pixel & 31u;
            const uint32_t green = (pixel >> 5) & 31u;
            const uint32_t blue = (pixel >> 10) & 31u;
            destination[y * 320 + x] =
                (red << 19 | (red >> 2) << 16) |
                (green << 11 | (green >> 2) << 8) |
                (blue << 3 | blue >> 2);
        }
    }
}

int ki1_core_fetch_audio(int16_t *samples, int max_samples)
{
    if (!core_ready || !samples || max_samples <= 0) return 0;
    return sound_fetch(samples, max_samples);
}

uint64_t ki1_core_frame_count(void)
{
    return machine.frame;
}
