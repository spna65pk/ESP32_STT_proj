#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config_file_ICS43434.h"
#include "ics_43434.h"
#include "audio_stats_serial.h"

#define READ_SAMPLES 256

static const char *TAG = "MAIN";

static void format_binary32(uint32_t value, char output[33])
{
    for (int bit = 31; bit >= 0; bit--)
    {
        output[31 - bit] = ((value >> bit) & 1U) ? '1' : '0';
    }

    output[32] = '\0';
}

void app_main(void)
{
    // Inicjalizacja mikrofonu ICS-43434
    ics43434_config_t cfg =
        ics43434_get_default_config(ICS43434_MODE_HIGH_PERFORMANCE);

    ESP_ERROR_CHECK(ics43434_init(&cfg));

    ESP_LOGI(TAG, "Hello world! Got config for ICS43434.");

    // Bufory na próbki audio
    static int32_t buf[READ_SAMPLES];
    static float samples_float[READ_SAMPLES];

    // Inicjalizacja modułu statystyk audio
    dsp_audio_stats_t audio_stats = {0};
    dsp_audio_stats_reset(&audio_stats);

    // Okres raportowania statystyk
    const uint32_t reads_per_log = cfg.sample_rate / READ_SAMPLES / 2;
    uint32_t read_count = 0;

    while (1)
    {
        size_t n = 0;

        // Odczyt próbek z mikrofonu
        esp_err_t err = ics43434_read(
            buf,
            READ_SAMPLES,
            &n,
            ICS43434_DEFAULT_READ_TIMEOUT_MS
        );

        if (err != ESP_OK && err != ESP_ERR_TIMEOUT)
        {
            ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (n == 0)
        {
            ESP_LOGW(TAG, "no data (timeout)");
            continue;
        }

        // Konwersja próbek PCM 24-bit na znormalizowany float [-1.0, 1.0]
        for (size_t i = 0; i < n; i++)
        {
            const int32_t s24 = ics43434_to_s24(buf[i]);

            samples_float[i] = (float)s24 / 8388608.0f;
        }

        // Aktualizacja statystyk sygnału audio
        dsp_audio_stats_update(
            &audio_stats,
            samples_float,
            n
        );

        // Okresowe raportowanie wyników
        if (++read_count >= reads_per_log)
        {
            if (dsp_audio_stats_get_result(&audio_stats))
            {
                ESP_LOGI(TAG,
                         "Peak=%.6f (%.2f dBFS) | RMS=%.6f (%.2f dBFS) | Samples=%" PRIu64,
                         audio_stats.peak,
                         audio_stats.peak_dbfs,
                         audio_stats.rms,
                         audio_stats.rms_dbfs,
                         audio_stats.sample_count);
            }

            // Diagnostyka pierwszej próbki z ostatniego odczytu
            const int32_t raw_signed = buf[0];
            const uint32_t raw_bits = (uint32_t)raw_signed;
            const int32_t sample24 = ics43434_to_s24(raw_signed);
            const uint32_t sample24_bits =
                (uint32_t)sample24 & 0x00FFFFFFU;

            char raw_binary[33];

            format_binary32(raw_bits, raw_binary);

            ESP_LOGI(TAG,
                     "first raw: signed=%" PRId32 " unsigned=%" PRIu32
                     " hex=0x%08" PRIX32 " binary=%s",
                     raw_signed,
                     raw_bits,
                     raw_bits,
                     raw_binary);

            ESP_LOGI(TAG,
                     "first sample: signed=%" PRId32 " 24-bit=0x%06" PRIX32,
                     sample24,
                     sample24_bits);

            // Rozpoczęcie nowego okresu pomiarowego
            dsp_audio_stats_reset(&audio_stats);
            read_count = 0;
        }
    }
}
