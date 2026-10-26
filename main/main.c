#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "config_file_ICS43434.h"
#include "ics_43434.h"

#define READ_SAMPLES  256
static const char *TAG = "MAIN";

static void format_binary32(uint32_t value, char output[33])
{
    for (int bit = 31; bit >= 0; bit--) {
        output[31 - bit] = ((value >> bit) & 1U) ? '1' : '0';
    }
    output[32] = '\0';
}

void app_main(void)
{
    // // Low-Power, 16 kHz, mikrofon z LR = GND
    // ics43434_config_t cfg = ics43434_get_default_config(ICS43434_MODE_LOW_POWER);
    // ESP_ERROR_CHECK(ics43434_init(&cfg));

    // High-Performance
    ics43434_config_t cfg = ics43434_get_default_config(ICS43434_MODE_HIGH_PERFORMANCE);
    ESP_ERROR_CHECK(ics43434_init(&cfg));

    ESP_LOGI("MAIN", "Hello world! Got config for ICS43434.");

    // Odczyt
    static int32_t buf[READ_SAMPLES];
    const uint32_t reads_per_log = cfg.sample_rate / READ_SAMPLES / 2;  
    uint32_t read_count = 0;
    int32_t  peak = 0;
    double   sum_sq = 0.0;
    uint32_t sum_n = 0;

    
    while(1) {

        size_t n = 0;
        esp_err_t err = ics43434_read(buf, READ_SAMPLES, &n,
                                      ICS43434_DEFAULT_READ_TIMEOUT_MS);
        if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (n == 0) {
            ESP_LOGW(TAG, "no data (timeout)");
            continue;
        }

        for (size_t i = 0; i < n; i++) {
            int32_t s24 = ics43434_to_s24(buf[i]);
            int32_t a = abs(s24);
            if (a > peak) peak = a;
            sum_sq += (double)s24 * (double)s24;
        }
        sum_n += n;

        if (++read_count >= reads_per_log) {
            double rms = sqrt(sum_sq / sum_n);
            double dbfs = (rms > 0.0) ? 20.0 * log10(rms / 8388608.0) : -999.0;
            const int32_t raw_signed = buf[0];
            const uint32_t raw_bits = (uint32_t)raw_signed;
            const int32_t sample24 = ics43434_to_s24(raw_signed);
            const uint32_t sample24_bits = (uint32_t)sample24 & 0x00FFFFFFU;
            char raw_binary[33];

            format_binary32(raw_bits, raw_binary);

            ESP_LOGI(TAG, "peak=%" PRId32 "  rms=%.0f  (%.1f dBFS)",
                     peak, rms, dbfs);
            ESP_LOGI(TAG,
                     "first raw: signed=%" PRId32 " unsigned=%" PRIu32
                     " hex=0x%08" PRIX32 " binary=%s",
                     raw_signed, raw_bits, raw_bits, raw_binary);
            ESP_LOGI(TAG, "first sample: signed=%" PRId32 " 24-bit=0x%06" PRIX32,
                     sample24, sample24_bits);


            read_count = 0;
            peak = 0;
            sum_sq = 0.0;
            sum_n = 0;
        }
    }
}