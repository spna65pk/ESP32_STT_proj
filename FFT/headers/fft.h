#ifndef FFT_H
#define FFT_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"


// Default configuration
#define FFT_DEFAULT_SAMPLE_RATE 48000.0f
#define FFT_DEFAULT_SIZE        1024
#define FFT_MAX_SIZE            2048
#define FFT_MAX_NUM_BINS        (FFT_MAX_SIZE / 2 + 1)

// Configuration structure
typedef struct {
    size_t fft_size;
    float sample_rate;
} fft_config_t;

// FFT result structure
typedef struct {
    float *magnitudes;
    size_t num_bins;
    float frequency_step;
} fft_result_t;

// Initialize FFT
// NULL -> default configuration
esp_err_t fft_init(const fft_config_t *config);

// Process FFT
// samples must contain signed 24-bit values represented as int32_t
esp_err_t fft_process(
    const int32_t *samples,
    size_t sample_count,
    fft_result_t *result
);

// Get frequency corresponding to FFT bin
float fft_get_bin_frequency(size_t bin);

// Reset FFT buffers
esp_err_t fft_reset(void);

// Deinitialize FFT
void fft_deinit(void);


#endif