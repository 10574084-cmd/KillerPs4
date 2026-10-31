#ifndef KI1_DATA_H
#define KI1_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KI1_SOUND_ROM_COUNT 8
#define KI1_BOOT_ROM_COUNT 2

typedef enum {
    KI1_DATA_OK = 0,
    KI1_DATA_MISSING,
    KI1_DATA_BAD_SIZE,
    KI1_DATA_BAD_CRC,
    KI1_DATA_BAD_HEADER,
    KI1_DATA_IO_ERROR
} Ki1DataState;

typedef struct {
    char name[32];
    Ki1DataState state;
    uint64_t size;
    uint32_t expected_crc32;
    uint32_t actual_crc32;
} Ki1DataFile;

typedef struct {
    Ki1DataFile boot_roms[KI1_BOOT_ROM_COUNT];
    Ki1DataFile sound_roms[KI1_SOUND_ROM_COUNT];
    Ki1DataFile disk_image;
    char selected_boot_rom[32];
    char selected_disk_image[32];
    unsigned missing_count;
    unsigned invalid_count;
    bool ready;
} Ki1DataReport;

/* Scan a user-supplied folder. The function never writes to game files. */
bool ki1_scan_data(const char *root, Ki1DataReport *report);

const char *ki1_data_state_name(Ki1DataState state);
uint32_t ki1_crc32_bytes(const void *data, size_t size);

#endif
