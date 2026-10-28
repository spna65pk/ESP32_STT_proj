
#ifndef DSP_FILTER_H
#define DSP_FILTER_H

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

/**
 * Kontekst pojedynczej instancji filtra FIR.
 *
 * coeffs:
 *     Wspolczynniki FIR w kolejnosci:
 *     b[0] * x[n] + b[1] * x[n-1] + ...
 *
 * state:
 *     Pamiec przechowujaca historie probek.
 *
 * num_taps:
 *     Liczba wspolczynnikow filtra (rzad + 1).
 *
 * write_index:
 *     Indeks kolejnej probki w buforze cyklicznym.
 */
typedef struct {
    const float *coeffs_reversed;  /* b[N-1], b[N-2], ..., b[0] */
    float *state;                  /* bufor 2*N floatow, nalezy do wywolujacego */
    size_t num_taps;
    size_t write_index;
    bool initialized;
} dsp_filter_t;

/**
 * Inicjalizuje filtr FIR.
 *
 * filter    - wskaznik na kontekst filtra
 * coeffs    - tablica wspolczynnikow FIR
 * num_taps  - liczba wspolczynnikow, musi byc > 0
 *
 * Wspolczynniki pozostaja w pamieci zarzadzanej przez wywolujacego.
 */
esp_err_t dsp_filter_init(
    dsp_filter_t *filter,
    const float *coeffs_reversed,
    float *state_buffer,
    size_t num_taps
);

/**
 * Przetwarza blok probek przez filtr FIR.
 *
 * input  - probki wejsciowe
 * output - bufor na probki wyjsciowe
 * count  - liczba probek do przetworzenia
 *
 * Mozna przekazac ten sam bufor jako input i output.
 * Stan filtra jest zachowywany pomiedzy wywolaniami.
 */
esp_err_t dsp_filter_process(
    dsp_filter_t *filter,
    const float *input,
    float *output,
    size_t count
);

/**
 * Zeruje historie probek bez zmiany wspolczynnikow.
 */
esp_err_t dsp_filter_reset(dsp_filter_t *filter);

/**
 * Czyści strukturę. Nie zwalnia pamieci (bufory naleza do wywolujacego).
 */
esp_err_t dsp_filter_deinit(dsp_filter_t *filter);

#endif /* DSP_FILTER_H */
