
#ifndef AUDIO_STATS_SERIAL_H
#define AUDIO_STATS_SERIAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    double sum_sq;
    float peak;
    uint64_t sample_count;

    float rms;
    float peak_dbfs;
    float rms_dbfs;
} dsp_audio_stats_t;

/* Zeruje wszystkie statystyki. */
void dsp_audio_stats_reset(dsp_audio_stats_t *stats);

/* Dodaje znormalizowane próbki float i aktualizuje akumulatory. */
void dsp_audio_stats_update(
    dsp_audio_stats_t *stats,
    const float *samples,
    size_t count
);

/* Oblicza wyniki bezpośrednio w strukturze stats. */
bool dsp_audio_stats_get_result(dsp_audio_stats_t *stats);

#endif
