
#include "config_file_ICS43434.h"
#include "ics_43434.h"


#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "ics43434";
static i2s_chan_handle_t s_rx_handle = NULL;

static bool ics43434_rate_valid(ics43434_power_mode_t mode, uint32_t fs)
{
    switch (mode) {
    case ICS43434_MODE_LOW_POWER:
        return fs >= ICS43434_LP_SAMPLE_RATE_MIN &&
               fs <= ICS43434_LP_SAMPLE_RATE_MAX;
    case ICS43434_MODE_HIGH_PERFORMANCE:
        return fs >= ICS43434_HP_SAMPLE_RATE_MIN &&
               fs <= ICS43434_HP_SAMPLE_RATE_MAX;
    default:
        return false;
    }
}

ics43434_config_t ics43434_get_default_config(ics43434_power_mode_t mode)
{
    ics43434_config_t config = {
        .bclk_gpio    = ICS43434_DEFAULT_BCLK_GPIO,
        .ws_gpio      = ICS43434_DEFAULT_WS_GPIO,
        .din_gpio     = ICS43434_DEFAULT_DIN_GPIO,
        .power_mode   = mode,
        .channel_mode = ICS43434_CHANNEL_MONO,                      // Obecnie jedynie mono jest obslugiwane
        .slot         = ICS43434_SLOT_LEFT,                         // Mikrofon z LR = GND
        .sample_rate  = (mode == ICS43434_MODE_HIGH_PERFORMANCE)
                            ? ICS43434_DEFAULT_HP_SAMPLE_RATE
                            : ICS43434_DEFAULT_LP_SAMPLE_RATE
    };

    return config;
}

esp_err_t ics43434_init(const ics43434_config_t *config)
{
    if (config == NULL) 
    {
        return ESP_ERR_INVALID_ARG;
    }

        // Stereo: miejsce na przyszla obsluge
    if (config->channel_mode == ICS43434_CHANNEL_STEREO) 
    {
        ESP_LOGE(TAG, "Stereo mode is not supported");
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (config->channel_mode != ICS43434_CHANNEL_MONO) 
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->slot != ICS43434_SLOT_LEFT &&
        config->slot != ICS43434_SLOT_RIGHT) 
        {
        return ESP_ERR_INVALID_ARG;
    }

    if (!GPIO_IS_VALID_GPIO(config->bclk_gpio) ||
        !GPIO_IS_VALID_GPIO(config->ws_gpio)   ||
        !GPIO_IS_VALID_GPIO(config->din_gpio)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!ics43434_rate_valid(config->power_mode, config->sample_rate)) {
        ESP_LOGE(TAG, "Sample rate %lu Hz invalid for selected power mode",
                 (unsigned long)config->sample_rate);
        return ESP_ERR_INVALID_ARG;
    }

    if (s_rx_handle != NULL) 
    {
        return ESP_ERR_INVALID_STATE;
    }

    // 1. Utworzenie kanalu odbiorczego.
    i2s_chan_config_t chan_config =
        I2S_CHANNEL_DEFAULT_CONFIG(
            I2S_NUM_AUTO,
            I2S_ROLE_MASTER
        );

    esp_err_t err = i2s_new_channel(
        &chan_config,
        NULL,
        &s_rx_handle
    );

    if (err != ESP_OK) {
        return err;
    }

    // 2. Konfiguracja standardowego trybu I2S.
    i2s_std_config_t std_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(
            config->sample_rate
        ),

        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT,
            I2S_SLOT_MODE_STEREO
        ),

        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = config->bclk_gpio,
            .ws = config->ws_gpio,
            .dout = I2S_GPIO_UNUSED,
            .din = config->din_gpio,

            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    // Wybor slotu zgodnie z pinem LR mikrofonu
    std_config.slot_cfg.slot_mask =
        (config->slot == ICS43434_SLOT_LEFT) ? I2S_STD_SLOT_LEFT
                                             : I2S_STD_SLOT_RIGHT;

    err = i2s_channel_init_std_mode(
        s_rx_handle,
        &std_config
    );

    if (err != ESP_OK) {
        i2s_del_channel(s_rx_handle);
        s_rx_handle = NULL;
        return err;
    }

    // 3. Uruchomienie odbioru.
    err = i2s_channel_enable(s_rx_handle);

    if (err != ESP_OK) {
        i2s_del_channel(s_rx_handle);
        s_rx_handle = NULL;
        return err;
    }

//---------------------- OPCJONALNIE DO DODANIA CO CHAT UGOTOWAŁ ------------------------
//---------------------------------------------------------------------------------------

    // // 4. Odrzucenie danych z okresu rozbiegu mikrofonu
    // int32_t tmp[64];
    // size_t remaining =
    //     (size_t)config->sample_rate * ICS43434_STARTUP_DISCARD_MS / 1000;

    // while (remaining > 0) {
    //     size_t chunk = (remaining < 64) ? remaining : 64;
    //     size_t got = 0;

    //     err = i2s_channel_read(s_rx_handle, tmp, chunk * sizeof(tmp[0]),
    //                            &got, ICS43434_DEFAULT_READ_TIMEOUT_MS);
    //     if (err != ESP_OK) {
    //         i2s_channel_disable(s_rx_handle);
    //         i2s_del_channel(s_rx_handle);
    //         s_rx_handle = NULL;
    //         return err;
    //     }

    //     size_t got_samples = got / sizeof(tmp[0]);
    //     remaining = (got_samples >= remaining) ? 0 : remaining - got_samples;
    // }

    return ESP_OK;
}

esp_err_t ics43434_read(
    int32_t *samples,
    size_t sample_count,
    size_t *samples_read,
    uint32_t timeout_ms)
{
    if (s_rx_handle == NULL ||
        samples == NULL ||
        samples_read == NULL ||
        sample_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    *samples_read = 0;

    size_t bytes_read = 0;

    esp_err_t err = i2s_channel_read(
        s_rx_handle,
        samples,
        sample_count * sizeof(samples[0]),
        &bytes_read,
        timeout_ms
    );

    *samples_read = bytes_read / sizeof(samples[0]);

    return err;
}

esp_err_t ics43434_deinit(void)
{
    if (s_rx_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = i2s_channel_disable(s_rx_handle);

    if (err != ESP_OK) {
        return err;
    }

    err = i2s_del_channel(s_rx_handle);

    if (err == ESP_OK) {
        s_rx_handle = NULL;
    }

    return err;
}
