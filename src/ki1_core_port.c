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

bool ki1_core_init(char *error, size_t error_capacity)
{
    if (core_ready) return true;
    if (!error || error_capacity == 0) return false;
    error[0] = '\0';

    game_select(&game_ki1);

    char boot_path[4096] = "";
    if (!find_game_file("ki-l15d.u98", boot_path, sizeof boot_path,
                        game_directory, sizeof game_directory)) {
        if (!find_game_file("ki-l14.u98", boot_path, sizeof boot_path,
                            game_directory, sizeof game_directory)) {
            snprintf(error, error_capacity,
                     "Missing KI1 boot ROM. Put ki-l15d.u98 or ki-l14.u98 in /data/KillerPs4/roms/kinst/.");
            return false;
        }
    }

    if (!load_boot_rom(boot_path)) {
        snprintf(error, error_capacity, "Could not read the 512 KiB boot ROM: %s", boot_path);
        return false;
    }

    char disk_path[4096] = "";
    if (!find_game_file("kinst.chd", disk_path, sizeof disk_path, NULL, 0) &&
        !find_game_file("kinst.raw", disk_path, sizeof disk_path, NULL, 0)) {
        snprintf(error, error_capacity,
                 "Missing KI1 hard-disk image. Put kinst.chd or kinst.raw beside the ROM files.");
        return false;
    }

    // The app's writable data directory is separate from its read-only package.
    // If it cannot be created, the image still opens with its original settings.
    (void)mkdir("/data/KillerPs4", 0777);
    const char *settings_path = "/data/KillerPs4/ki1.settings";
    struct stat st;
    if (stat("/data/KillerPs4", &st) != 0 || (st.st_mode & S_IFDIR) == 0) {
        settings_path = NULL;
    }

    if (!ide_open(disk_path, settings_path, game.cyls, game.heads,
                  game.secs, game.serial_offset)) {
        snprintf(error, error_capacity, "Could not open KI1 disk image: %s", disk_path);
        return false;
    }

    // Missing sound ROMs are non-fatal to the machine boot; UltraCombo leaves
    // the DCS board initialized but silent when those files are unavailable.
    (void)sound_init(game_directory);

    hw.dsw = 0xffffffffu;
    cpu_reset();
    rec_reset();
    rec_init();
    state_init();

    core_ready = true;
    return true;
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
