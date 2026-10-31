#ifndef KI1_CORE_PORT_H
#define KI1_CORE_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * PS4-facing adapter for the UltraCombo KI1 machine core.
 * Game data is supplied by the user and is never bundled in the PKG.
 */
bool ki1_core_init(char *error, size_t error_capacity);
void ki1_core_run_frame(uint16_t player1_buttons, uint16_t player2_buttons);
void ki1_core_copy_rgb(uint32_t *destination_320x240);
int ki1_core_fetch_audio(int16_t *samples, int max_samples);
uint64_t ki1_core_frame_count(void);

#endif
