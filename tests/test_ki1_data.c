#include "ki1_data.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const char check[] = "123456789";
    Ki1DataReport report;

    assert(ki1_crc32_bytes(check, sizeof(check) - 1) == 0xcbf43926u);
    assert(ki1_crc32_bytes(NULL, 0) == 0u);
    assert(!ki1_scan_data("/path/that/should/not/exist/KillerPs4", &report));
    assert(report.ready == false);
    assert(report.selected_boot_rom[0] == '\0');
    assert(report.selected_disk_image[0] == '\0');
    assert(report.missing_count == 10u);
    assert(report.invalid_count == 0u);
    assert(report.boot_roms[0].state == KI1_DATA_MISSING);
    assert(report.sound_roms[0].state == KI1_DATA_MISSING);
    assert(report.disk_image.state == KI1_DATA_MISSING);
    assert(strcmp(ki1_data_state_name(KI1_DATA_OK), "OK") == 0);
    assert(ki1_scan_data(NULL, NULL) == false);

    puts("KI1 data-manifest tests passed");
    return 0;
}
