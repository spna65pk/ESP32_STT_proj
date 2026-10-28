#include <stdlib.h>
#include <string.h>
#include "dsp_filter.h"

#define NUM_TAPS 150

// 1. Zmienna na strukturę filtra
static dsp_filter_t fir_filter;

// 2. Tablica ze współczynnikami (we Flashu / ROM)
const float fir_filter_coeffs[NUM_TAPS] = {0};

// 3. STATYCZNY BUFOR HISTORII w pamięci RAM
// Opcjonalnie moze byc potrzebne ich kilka jak zrobimy kilka filtrow
// bedzie je mozna dodac do oddzielnych plikow .c ktore beda mialy napisane funkcje getterów
static float fir_filter_state_buffer[2 * NUM_TAPS];

esp_err_t dsp_filter_init(
    dsp_filter_t *filter,
    const float *coeffs_reversed,
    float *state_buffer,
    size_t num_taps)
{
    if (filter == NULL || coeffs_reversed == NULL || state_buffer == NULL || num_taps == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Kontekst powinien byc nowy lub wczesniej zwolniony.
     * Nie wywoluj init ponownie na aktywnym filtrze.
     */
    memset(state_buffer, 0, 2 * num_taps * sizeof(float));
    memset(filter, 0, sizeof(dsp_filter_t));

    filter->coeffs_reversed = coeffs_reversed;
    filter->state = state_buffer;
    filter->num_taps = num_taps;
    filter->write_index = 0;
    filter->initialized = true;

    return ESP_OK;
}

// stop tutaj
esp_err_t dsp_filter_process(
    dsp_filter_t *filter,
    const float *input,
    float *output,
    size_t count)
{
    if (filter == NULL ||
        input == NULL ||
        output == NULL ||
        !filter->initialized ||
        filter->coeffs_reversed == NULL ||
        filter->state == NULL ||
        filter->num_taps == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const size_t N = filter->num_taps;
    float *state = filter->state;
    const float *coeffs = filter->coeffs_reversed;
    size_t write = filter->write_index;

    for (size_t i = 0; i < count; i++)
    {
        const float x = input[i];

        /* Zapis probki w obu polowach bufora. */
        state[write] = x;
        state[write + N] = x;

        /*
         * Okno N probek ulozonych od najstarszej do najnowszej.
         * Dzieki podwojnemu buforowi nie ma zawijania indeksu
         * w wewnetrznej petli FIR.
         */
        const float *samples = &state[write + 1];

        float acc = 0.0f;

        /*
         * FIR:
         * y[n] = b[0]*x[n] + b[1]*x[n-1] + ...
         */
        for (size_t k = 0; k < N; k++) 
        {
            acc += coeffs[k] * samples[k];
        }

        output[i] = acc;

        /* Przesuniecie wskaznika bufora cyklicznego. */
        write++;

        if (write == N) 
        {
            write = 0;
        }
    }
    
    filter->write_index = write;

    return ESP_OK;
}

esp_err_t dsp_filter_reset(dsp_filter_t *filter)
{
    if (filter == NULL ||
        !filter->initialized ||
        filter->state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(filter->state, 0,
           2 * filter->num_taps * sizeof(filter->state[0]));

    filter->write_index = 0;

    return ESP_OK;
}

esp_err_t dsp_filter_deinit(dsp_filter_t *filter)
{
    if (filter == NULL || !filter->initialized)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(filter, 0, sizeof(*filter));

    return ESP_OK;
}
