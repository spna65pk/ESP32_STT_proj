
#ifndef ICS_43434_H
#define ICS_43434_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    ICS43434_MODE_LOW_POWER,        // fs: 6.25 - 18.75 kHz, ~230 uA
    ICS43434_MODE_HIGH_PERFORMANCE  // fs: 23 - 51.6 kHz,   ~490 uA
} ics43434_power_mode_t;

typedef enum {
    ICS43434_CHANNEL_MONO,
    ICS43434_CHANNEL_STEREO         // obecnie nieobslugiwane
} ics43434_channel_mode_t;

typedef enum {
    ICS43434_SLOT_LEFT,             // pin LR mikrofonu = GND
    ICS43434_SLOT_RIGHT             // pin LR mikrofonu = VDD
} ics43434_slot_t;


typedef struct {
    int bclk_gpio;
    int ws_gpio;
    int din_gpio;

    ics43434_power_mode_t   power_mode;
    ics43434_channel_mode_t channel_mode;
    ics43434_slot_t         slot;

    uint32_t sample_rate;
} ics43434_config_t;

/* 
 *   Domyslna konfiguracja dla wybranego trybu (mono, lewy slot). 
 */
ics43434_config_t ics43434_get_default_config(ics43434_power_mode_t mode);

/*
 * Inicjalizuje odbior I2S.
 */
esp_err_t ics43434_init(const ics43434_config_t *config);

/*
 * Odbiera surowe sloty 32-bitowe.
 *
 * samples       - bufor uzytkownika
 * sample_count  - maksymalna liczba slotow
 * samples_read  - faktyczna liczba odebranych slotow
 * timeout_ms    - maksymalny czas oczekiwania
 */
esp_err_t ics43434_read(
    int32_t *samples,
    size_t sample_count,
    size_t *samples_read,
    uint32_t timeout_ms
);

/* Konwersja surowego slotu na 24-bitowa probke ze znakiem. */
static inline int32_t ics43434_to_s24(int32_t raw)
{
    return raw >> 8;
}

/*
 * Zwalnia kanal I2S.
 */
esp_err_t ics43434_deinit(void);

#endif
