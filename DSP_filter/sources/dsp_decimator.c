
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "dsp_decimator.h"


#define DSP_DECIMATOR_SYMMETRY_TOLERANCE 1e-5f


esp_err_t dsp_decimator_init(
    dsp_decimator_t *decimator,
    const float *coeffs,
    float *state_buffer,
    size_t num_taps,            // Liczba wspolczynnikow filtra (N) -> state_buffer_len kinda
    size_t decimation_factor)
{
    if (decimator == NULL ||
        coeffs == NULL ||
        state_buffer == NULL ||
        num_taps < 3 ||
        (num_taps % 2) == 0 ||
        decimation_factor == 0 ||
        num_taps > SIZE_MAX / (2u * sizeof(float)))
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Folded FIR wymaga symetrycznych wspolczynnikow.
     */
    for (size_t k = 0; k < num_taps / 2; k++)
    {
        if (fabsf(coeffs[k] - coeffs[num_taps - 1 - k]) >
            DSP_DECIMATOR_SYMMETRY_TOLERANCE)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }

    memset(state_buffer, 0, 2 * num_taps * sizeof(float));
    memset(decimator, 0, sizeof(*decimator));

    decimator->coeffs = coeffs;
    decimator->state = state_buffer;

    decimator->num_taps = num_taps;
    decimator->decimation_factor = decimation_factor;

    decimator->phase = 0;
    decimator->write_index = 0;

    decimator->initialized = true;

    return ESP_OK;
}


/*
 * Oblicza liczbe probek wyjsciowych dla danego bloku.
 */
size_t dsp_decimator_output_count(
    const dsp_decimator_t *decimator,
    size_t input_count)
{
    size_t decimation_factor = decimator->decimation_factor;
    if (decimator == NULL ||
        !decimator->initialized ||
        decimation_factor == 0)
    {
        return 0;
    }

    if (input_count == 0)
    {
        return 0;
    }

    /*
     * Odleglosc od poczatku bloku do nastepnej
     * probki, dla ktorej zostanie obliczone wyjscie.
     */
    const size_t first_output =
        (decimation_factor - decimator->phase)
        % decimation_factor;

    if (first_output >= input_count)
    {
        return 0;
    }

    return 1 + (input_count - 1 - first_output)
                 / decimation_factor;
}


/*
 * Glowna funkcja przetwarzajaca.
 *
 * Folded FIR + decymacja z zachowaniem fazy.
 */
esp_err_t dsp_decimator_process(
    dsp_decimator_t *decimator,
    const float *input,
    size_t input_count,
    float *output,
    size_t output_capacity,
    size_t *output_count)
{
    if (decimator == NULL ||
        !decimator->initialized ||
        decimator->coeffs == NULL ||
        decimator->state == NULL ||
        output_count == NULL ||
        (input_count > 0 && input == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    *output_count = 0;

    const size_t required =
        dsp_decimator_output_count(decimator, input_count);

    /*
     * Sprawdzamy pojemnosc przed rozpoczeciem
     * przetwarzania, aby uniknac czesciowego wykonania.
     */
    if (required > output_capacity ||
        (required > 0 && output == NULL))
    {
        return ESP_ERR_INVALID_SIZE;
    }

    const size_t N = decimator->num_taps;
    const size_t M = (N - 1) / 2;

    

    const float *coeffs = decimator->coeffs;
    float *state = decimator->state;

    size_t write = decimator->write_index;
    size_t phase = decimator->phase;

    size_t out_index = 0;
    //TU SKONCZYLEM REL
    for (size_t i = 0; i < input_count; i++)
    {
        const float x = input[i];

        /*
         * Zapis probki w obu polowach bufora.
         */
        state[write] = x;
        state[write + N] = x;

        /*
         * Wynik obliczamy tylko dla odpowiedniej fazy.
         */
        if (phase == 0)
        {
            /*
             * Wskaznik na aktualna probke.
             *
             * current[-k] oznacza x[n-k].
             */
            const float *current = &state[write + N];

            float acc = 0.0f;

            /*
             * Folded FIR:
             *
             * h[k] == h[N-1-k]
             *
             * y[n] = sum h[k] *
             *        (x[n-k] + x[n-(N-1-k)])
             *
             *        + h[M] * x[n-M]
             */

            for (size_t k = 0; k < M; k++)
            {
                const float sample_a = current[-(ptrdiff_t)k];

                const float sample_b =
                    current[-(ptrdiff_t)(N - 1 - k)];

                acc += coeffs[k] * (sample_a + sample_b);
            }

            /*
             * Srodkowy wspolczynnik.
             * Nie ma pary symetrycznej.
             */
            acc += coeffs[M] * current[-(ptrdiff_t)M];

            output[out_index++] = acc;
        }

        /*
         * Aktualizacja fazy decymacji.
         */
        phase++;

        if (phase == decimator->decimation_factor)
        {
            phase = 0;
        }

        /*
         * Aktualizacja wskaznika bufora cyklicznego.
         */
        write++;

        if (write == N)
        {
            write = 0;
        }
    }

    decimator->write_index = write;
    decimator->phase = phase;

    *output_count = out_index;

    return ESP_OK;
}


/*
 * Reset historii i fazy.
 */
esp_err_t dsp_decimator_reset(dsp_decimator_t *decimator)
{
    if (decimator == NULL ||
        !decimator->initialized ||
        decimator->state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(
        decimator->state,
        0,
        2 * decimator->num_taps * sizeof(float)
    );

    decimator->write_index = 0;
    decimator->phase = 0;

    return ESP_OK;
}


/*
 * Deinit - bez zwalniania pamieci.
 */
esp_err_t dsp_decimator_deinit(dsp_decimator_t *decimator)
{
    if (decimator == NULL || !decimator->initialized)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(decimator, 0, sizeof(*decimator));

    return ESP_OK;
}
