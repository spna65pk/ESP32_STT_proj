#include "fft.h"

#include <math.h>
#include <string.h>
#include <stdbool.h>

#define FFT_PI 3.14159265358979323846

static fft_config_t s_config;

static float s_window[FFT_MAX_SIZE];
static float s_real[FFT_MAX_SIZE];
static float s_imag[FFT_MAX_SIZE];

static bool s_initialized = false;



static void fft_bit_reverse(float *real, float *imag, size_t N)
{
    size_t j = 0;

    for (size_t i = 1; i < N; i++) {

        size_t bit = N >> 1;

        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }

        j ^= bit;

        if (i < j) {

            float temp;

            // Swap real parts
            temp = real[i];
            real[i] = real[j];
            real[j] = temp;

            // Swap imaginary parts
            temp = imag[i];
            imag[i] = imag[j];
            imag[j] = temp;
        }
    }
}



static void fft_radix2(float *real, float *imag, size_t N)
{
    fft_bit_reverse(real, imag, N);

    for (size_t len = 2; len <= N; len <<= 1) {

        float angle = (float)(-2.0 * FFT_PI / (double)len);

        float wlen_real = cosf(angle);
        float wlen_imag = sinf(angle);

        for (size_t i = 0; i < N; i += len) {

            float w_real = 1.0f;
            float w_imag = 0.0f;

            for (size_t j = 0; j < len / 2; j++) {

                size_t u = i + j;
                size_t v = i + j + len / 2;

                float v_real =
                    real[v] * w_real - imag[v] * w_imag;

                float v_imag =
                    real[v] * w_imag + imag[v] * w_real;

                // Butterfly
                real[v] = real[u] - v_real;
                imag[v] = imag[u] - v_imag;

                real[u] += v_real;
                imag[u] += v_imag;

                // Update twiddle factor
                float next_w_real =
                    w_real * wlen_real - w_imag * wlen_imag;

                float next_w_imag =
                    w_real * wlen_imag + w_imag * wlen_real;

                w_real = next_w_real;
                w_imag = next_w_imag;
            }
        }

        // Prevent overflow in len <<= 1
        if (len == N) {
            break;
        }
    }
}


esp_err_t fft_init(const fft_config_t *config)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Default configuration
    if (config == NULL) {

        s_config.fft_size = FFT_DEFAULT_SIZE;
        s_config.sample_rate = FFT_DEFAULT_SAMPLE_RATE;

    } else {

        s_config = *config;
    }

    // Validate configuration
    if (s_config.fft_size < 2 ||
        s_config.fft_size > FFT_MAX_SIZE ||
        (s_config.fft_size & (s_config.fft_size - 1)) != 0 ||
        !isfinite(s_config.sample_rate) ||
        s_config.sample_rate <= 0.0f) {

        memset(&s_config, 0, sizeof(s_config));

        return ESP_ERR_INVALID_ARG;
    }

    // Initialize buffers
    memset(s_window, 0, sizeof(s_window));
    memset(s_real, 0, sizeof(s_real));
    memset(s_imag, 0, sizeof(s_imag));

    // Generate Hann window
    for (size_t i = 0; i < s_config.fft_size; i++) {

        s_window[i] = 0.5f * (
            1.0f - cosf(
                (float)(
                    2.0 * FFT_PI * (double)i /
                    (double)(s_config.fft_size - 1)
                )
            )
        );
    }

    s_initialized = true;

    return ESP_OK;
}



esp_err_t fft_process(
    const int32_t *samples,
    size_t sample_count,
    fft_result_t *result
)
{
    // Validate arguments
    if (!s_initialized ||
        samples == NULL ||
        result == NULL ||
        result->magnitudes == NULL ||
        sample_count != s_config.fft_size ||
        result->num_bins < s_config.fft_size / 2 + 1) {

        return ESP_ERR_INVALID_ARG;
    }

    size_t N = s_config.fft_size;
    size_t bins = N / 2 + 1;

    double mean = 0.0;
    double window_sum = 0.0;

    // Calculate DC component (mean value)
    for (size_t i = 0; i < N; i++) {
        mean += (double)samples[i];
    }

    mean /= (double)N;

    // Normalize samples and apply Hann window
    for (size_t i = 0; i < N; i++) {

        float sample = (float)(
            ((double)samples[i] - mean) / 8388608.0
        );

        s_real[i] = sample * s_window[i];
        s_imag[i] = 0.0f;

        window_sum += s_window[i];
    }

    // Calculate FFT
    fft_radix2(s_real, s_imag, N);

    // Calculate magnitude spectrum
    for (size_t k = 0; k < bins; k++) {

        float re = s_real[k];
        float im = s_imag[k];

        float magnitude = sqrtf(re * re + im * im);

        // Normalize by window gain
        if (window_sum > 0.0) {
            magnitude /= (float)window_sum;
        }

        // Single-sided spectrum correction
        if (k != 0 && k != N / 2) {
            magnitude *= 2.0f;
        }

        result->magnitudes[k] = magnitude;
    }

    // Frequency resolution
    result->frequency_step =
        s_config.sample_rate / (float)N;

    return ESP_OK;
}


float fft_get_bin_frequency(size_t bin)
{
    if (!s_initialized ||
        bin > s_config.fft_size / 2) {

        return 0.0f;
    }

    return (float)bin * s_config.sample_rate /
           (float)s_config.fft_size;
}

esp_err_t fft_reset(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    memset(s_real, 0, sizeof(s_real));
    memset(s_imag, 0, sizeof(s_imag));

    return ESP_OK;
}



void fft_deinit(void)
{
    memset(s_window, 0, sizeof(s_window));
    memset(s_real, 0, sizeof(s_real));
    memset(s_imag, 0, sizeof(s_imag));
    memset(&s_config, 0, sizeof(s_config));

    s_initialized = false;
}