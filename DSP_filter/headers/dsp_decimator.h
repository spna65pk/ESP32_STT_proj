#ifndef DSP_DECIMATOR_H
#define DSP_DECIMATOR_H

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

/**
 * Kontekst decymatora FIR.
 *
 * coeffs:
 *     Pelna tablica wspolczynnikow LPF w normalnej kolejnosci.
 *     Wymagana symetria: h[k] == h[N-1-k].
 *
 * state:
 *     Podwojny bufor historii probek.
 *     Wymagany rozmiar: 2 * num_taps floatow.
 *
 * num_taps:
 *     Liczba wspolczynnikow filtra.
 *     Musi byc nieparzysta.
 *
 * decimation_factor:
 *     Wspolczynnik decymacji, np. 3 dla 48 kHz -> 16 kHz.
 *
 * phase:
 *     Aktualna faza decymacji.
 *     Zachowywana pomiedzy wywolaniami process().
 *
 * write_index:
 *     Indeks zapisu w buforze cyklicznym.
 */
typedef struct
{
    const float *coeffs;
    float *state;

    size_t num_taps;
    size_t decimation_factor;
    size_t phase;
    size_t write_index;

    bool initialized;

} dsp_decimator_t;


/**
 * Inicjalizuje decymator FIR.
 *
 * coeffs             - wspolczynniki LPF (normalna kolejnosc)
 * state_buffer       - bufor historii 2 * num_taps
 * num_taps           - nieparzysta liczba wspolczynnikow
 * decimation_factor  - wspolczynnik decymacji
 */
esp_err_t dsp_decimator_init(
    dsp_decimator_t *decimator,
    const float *coeffs,
    float *state_buffer,
    size_t num_taps,
    size_t decimation_factor
);


/**
 * Zwraca liczbe probek wyjsciowych, ktore powstana
 * po przetworzeniu input_count probek.
 *
 * Uwzglednia aktualna faze decymacji.
 */
size_t dsp_decimator_output_count(
    const dsp_decimator_t *decimator,
    size_t input_count
);


/**
 * Przetwarza dowolna liczbe probek wejsciowych.
 *
 * input          - probki wejsciowe
 * input_count    - liczba probek wejsciowych
 * output         - bufor wyjsciowy
 * output_capacity- pojemnosc bufora wyjsciowego
 * output_count   - faktyczna liczba wygenerowanych probek
 *
 * Stan filtra oraz faza decymacji sa zachowywane
 * pomiedzy wywolaniami.
 *
 * Mozna uzyc tego samego bufora dla input i output.
 */
esp_err_t dsp_decimator_process(
    dsp_decimator_t *decimator,
    const float *input,
    size_t input_count,
    float *output,
    size_t output_capacity,
    size_t *output_count
);


/**
 * Zeruje historie probek i faze decymacji.
 */
esp_err_t dsp_decimator_reset(dsp_decimator_t *decimator);


/**
 * Czysci strukture. Nie zwalnia pamieci.
 */
esp_err_t dsp_decimator_deinit(dsp_decimator_t *decimator);

#endif /* DSP_DECIMATOR_H */