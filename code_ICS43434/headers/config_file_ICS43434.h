#ifndef config_file_ICS43434_h
#define config_file_ICS43434_h

#include <stdint.h>

/*
 * UWAGA:
 * Numery GPIO sa przykladowe.
 * Przed podlaczeniem sprawdz pinout M5Stamp-P4 S013.
 */
#define ICS43434_DEFAULT_BCLK_GPIO        19
#define ICS43434_DEFAULT_WS_GPIO          23
#define ICS43434_DEFAULT_DIN_GPIO         21

/* Zakresy z datasheetu (Table 5) */
#define ICS43434_LP_SAMPLE_RATE_MIN       6250
#define ICS43434_LP_SAMPLE_RATE_MAX       18750
#define ICS43434_HP_SAMPLE_RATE_MIN       23000
#define ICS43434_HP_SAMPLE_RATE_MAX       51600

/* Domyslne probkowanie dla kazdego trybu */
#define ICS43434_DEFAULT_LP_SAMPLE_RATE   16000
#define ICS43434_DEFAULT_HP_SAMPLE_RATE   48000

#define ICS43434_DEFAULT_READ_TIMEOUT_MS  1000

/* Ile ms danych odrzucic po starcie (datasheet: 32768 cykli SCK) */
#define ICS43434_STARTUP_DISCARD_MS       50

#endif