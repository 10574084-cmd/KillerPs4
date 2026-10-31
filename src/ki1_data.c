#include "ki1_data.h"

#include <stdio.h>
#include <string.h>

#define KI1_ROM_BYTES 0x80000u
#define KI1_RAW_DISK_BYTES ((uint64_t)419u * 13u * 47u * 512u)

typedef struct {
    const char *name;
    uint32_t crc32;
} RomSpec;

static const RomSpec boot_specs[KI1_BOOT_ROM_COUNT] = {
    { "ki-l15d.u98", 0x7b65ca3du },
    { "ki-l14.u98",  0xafedb75fu }
};

static const RomSpec sound_specs[KI1_SOUND_ROM_COUNT] = {
    { "u10-l1", 0xb6cc155fu },
    { "u11-l1", 0x0b5e05dfu },
    { "u12-l1", 0xd05ce6adu },
    { "u13-l1", 0x7d0954eau },
    { "u33-l1", 0x8bbe4f0cu },
    { "u34-l1", 0xb2e73603u },
    { "u35-l1", 0x0aaef4fcu },
    { "u36-l1", 0x0577bb60u }
};

uint32_t ki1_crc32_bytes(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xffffffffu;

    if (bytes == NULL && size != 0) {
        return 0;
    }

    for (size_t i = 0; i < size; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            const uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (0xedb88320u & mask);
        }
    }

    return ~crc;
}

const char *ki1_data_state_name(Ki1DataState state)
{
    switch (state) {
        case KI1_DATA_OK: return "OK";
        case KI1_DATA_MISSING: return "missing";
        case KI1_DATA_BAD_SIZE: return "wrong size";
        case KI1_DATA_BAD_CRC: return "checksum mismatch";
        case KI1_DATA_BAD_HEADER: return "invalid disk header";
        case KI1_DATA_IO_ERROR: return "read error";
        default: return "unknown";
    }
}

static bool make_path(const char *root, const char *name,
                      char *path, size_t capacity)
{
    int written;

    written = snprintf(path, capacity, "%s/%s", root, name);
    if (written >= 0 && (size_t)written < capacity) {
        FILE *file = fopen(path, "rb");
        if (file != NULL) {
            fclose(file);
            return true;
        }
    }

    /* MAME-style layout: roms/kinst/kinst.chd. */
    written = snprintf(path, capacity, "%s/kinst/%s", root, name);
    if (written >= 0 && (size_t)written < capacity) {
        FILE *file = fopen(path, "rb");
        if (file != NULL) {
            fclose(file);
            return true;
        }
    }

    if (capacity != 0) {
        path[0] = '\0';
    }
    return false;
}

static uint64_t file_size(FILE *file, bool *ok)
{
    long end;

    *ok = false;
    if (fseek(file, 0, SEEK_END) != 0) {
        return 0;
    }

    end = ftell(file);
    if (end < 0 || fseek(file, 0, SEEK_SET) != 0) {
        return 0;
    }

    *ok = true;
    return (uint64_t)end;
}

static uint32_t crc32_file(FILE *file, bool *ok)
{
    uint8_t buffer[16384];
    uint32_t crc = 0xffffffffu;
    size_t count;

    *ok = false;
    if (fseek(file, 0, SEEK_SET) != 0) {
        return 0;
    }

    while ((count = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        for (size_t i = 0; i < count; ++i) {
            crc ^= buffer[i];
            for (unsigned bit = 0; bit < 8; ++bit) {
                const uint32_t mask = 0u - (crc & 1u);
                crc = (crc >> 1) ^ (0xedb88320u & mask);
            }
        }
    }

    if (ferror(file)) {
        return 0;
    }

    *ok = true;
    return ~crc;
}

static Ki1DataFile check_rom(const char *root, const RomSpec *spec)
{
    Ki1DataFile result;
    char path[512];
    FILE *file;
    bool size_ok, crc_ok;

    memset(&result, 0, sizeof(result));
    snprintf(result.name, sizeof(result.name), "%s", spec->name);
    result.expected_crc32 = spec->crc32;

    if (!make_path(root, spec->name, path, sizeof(path))) {
        result.state = KI1_DATA_MISSING;
        return result;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        result.state = KI1_DATA_IO_ERROR;
        return result;
    }

    result.size = file_size(file, &size_ok);
    if (!size_ok) {
        result.state = KI1_DATA_IO_ERROR;
        fclose(file);
        return result;
    }

    if (result.size != KI1_ROM_BYTES) {
        result.state = KI1_DATA_BAD_SIZE;
        fclose(file);
        return result;
    }

    result.actual_crc32 = crc32_file(file, &crc_ok);
    fclose(file);

    if (!crc_ok) {
        result.state = KI1_DATA_IO_ERROR;
    } else if (result.actual_crc32 != result.expected_crc32) {
        result.state = KI1_DATA_BAD_CRC;
    } else {
        result.state = KI1_DATA_OK;
    }

    return result;
}

static Ki1DataFile check_raw_disk(const char *root)
{
    Ki1DataFile result;
    char path[512];
    FILE *file;
    bool size_ok;

    memset(&result, 0, sizeof(result));
    snprintf(result.name, sizeof(result.name), "%s", "kinst.raw");

    if (!make_path(root, result.name, path, sizeof(path))) {
        result.state = KI1_DATA_MISSING;
        return result;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        result.state = KI1_DATA_IO_ERROR;
        return result;
    }

    result.size = file_size(file, &size_ok);
    fclose(file);

    if (!size_ok) {
        result.state = KI1_DATA_IO_ERROR;
    } else if (result.size != KI1_RAW_DISK_BYTES) {
        result.state = KI1_DATA_BAD_SIZE;
    } else {
        result.state = KI1_DATA_OK;
    }

    return result;
}

static Ki1DataFile check_chd_disk(const char *root)
{
    Ki1DataFile result;
    char path[512];
    char header[8];
    FILE *file;
    bool size_ok;

    memset(&result, 0, sizeof(result));
    snprintf(result.name, sizeof(result.name), "%s", "kinst.chd");

    if (!make_path(root, result.name, path, sizeof(path))) {
        result.state = KI1_DATA_MISSING;
        return result;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        result.state = KI1_DATA_IO_ERROR;
        return result;
    }

    result.size = file_size(file, &size_ok);
    if (!size_ok || result.size < sizeof(header)) {
        result.state = KI1_DATA_BAD_SIZE;
        fclose(file);
        return result;
    }

    if (fseek(file, 0, SEEK_SET) != 0 ||
        fread(header, 1, sizeof(header), file) != sizeof(header)) {
        result.state = KI1_DATA_IO_ERROR;
    } else if (memcmp(header, "MComprHD", sizeof(header)) != 0) {
        result.state = KI1_DATA_BAD_HEADER;
    } else {
        result.state = KI1_DATA_OK;
    }

    fclose(file);
    return result;
}

bool ki1_scan_data(const char *root, Ki1DataReport *report)
{
    const char *folder = (root != NULL && root[0] != '\0') ? root : ".";
    bool boot_found = false;

    if (report == NULL) {
        return false;
    }

    memset(report, 0, sizeof(*report));

    for (unsigned i = 0; i < KI1_BOOT_ROM_COUNT; ++i) {
        report->boot_roms[i] = check_rom(folder, &boot_specs[i]);
        if (!boot_found && report->boot_roms[i].state == KI1_DATA_OK) {
            snprintf(report->selected_boot_rom, sizeof(report->selected_boot_rom),
                     "%s", report->boot_roms[i].name);
            boot_found = true;
        }
    }

    if (!boot_found) {
        if (report->boot_roms[0].state == KI1_DATA_MISSING &&
            report->boot_roms[1].state == KI1_DATA_MISSING) {
            ++report->missing_count;
        } else {
            ++report->invalid_count;
        }
    }

    for (unsigned i = 0; i < KI1_SOUND_ROM_COUNT; ++i) {
        report->sound_roms[i] = check_rom(folder, &sound_specs[i]);
        if (report->sound_roms[i].state == KI1_DATA_MISSING) {
            ++report->missing_count;
        } else if (report->sound_roms[i].state != KI1_DATA_OK) {
            ++report->invalid_count;
        }
    }

    report->disk_image = check_raw_disk(folder);
    if (report->disk_image.state == KI1_DATA_OK) {
        snprintf(report->selected_disk_image, sizeof(report->selected_disk_image),
                 "%s", report->disk_image.name);
    } else {
        Ki1DataFile chd = check_chd_disk(folder);
        if (chd.state == KI1_DATA_OK) {
            report->disk_image = chd;
        } else if (report->disk_image.state == KI1_DATA_MISSING &&
                   chd.state == KI1_DATA_MISSING) {
            report->disk_image = chd;
        } else if (report->disk_image.state == KI1_DATA_MISSING) {
            report->disk_image = chd;
        } else if (chd.state != KI1_DATA_MISSING) {
            report->disk_image = chd;
        }
        if (report->disk_image.state == KI1_DATA_OK) {
            snprintf(report->selected_disk_image, sizeof(report->selected_disk_image),
                     "%s", report->disk_image.name);
        }
    }

    if (report->disk_image.state == KI1_DATA_MISSING) {
        ++report->missing_count;
    } else if (report->disk_image.state != KI1_DATA_OK) {
        ++report->invalid_count;
    }

    report->ready = (report->missing_count == 0 && report->invalid_count == 0);
    return report->ready;
}
