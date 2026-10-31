
#include "audio_stats_serial.h"

#include <math.h>

#define DSP_AUDIO_DBFS_FLOOR (-120.0f)

void dsp_audio_stats_reset(dsp_audio_stats_t *stats)
{
    if (stats == NULL)
    {
        return;
    }

    stats->sum_sq = 0.0;
    stats->peak = 0.0f;
    stats->sample_count = 0;

    stats->rms = 0.0f;
    stats->peak_dbfs = DSP_AUDIO_DBFS_FLOOR;
    stats->rms_dbfs = DSP_AUDIO_DBFS_FLOOR;
}

void dsp_audio_stats_update(
    dsp_audio_stats_t *stats,
    const float *samples,
    size_t count)
{
    if (stats == NULL || samples == NULL)
    {
        return;
    }

    for (size_t i = 0; i < count; i++)
    {
        const float x = samples[i];
        const float abs_x = fabsf(x);

        if (abs_x > stats->peak)
        {
            stats->peak = abs_x;
        }

        stats->sum_sq += (double)x * (double)x;
    }

    stats->sample_count += count;
}

bool dsp_audio_stats_get_result(dsp_audio_stats_t *stats)
{
    if (stats == NULL || stats->sample_count == 0)
    {
        return false;
    }

    const double mean_sq =
        stats->sum_sq / (double)stats->sample_count;

    stats->rms = (float)sqrt(mean_sq);

    stats->peak_dbfs =
        (stats->peak > 0.0f)
            ? 20.0f * log10f(stats->peak)
            : DSP_AUDIO_DBFS_FLOOR;

    stats->rms_dbfs =
        (stats->rms > 0.0f)
            ? 20.0f * log10f(stats->rms)
            : DSP_AUDIO_DBFS_FLOOR;

    return true;
}
