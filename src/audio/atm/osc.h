
#include <stdbool.h>
#include <stdint.h>

#define OSC_SAMPLERATE (16000)
#define OSC_MOD_MIN (0)
#define OSC_MOD_MAX (255)
/* TIMER3_COMPA fires at OSC_SAMPLERATE Hz and the tick handler is only run
   once every OSC_ISR_PRESCALER_DIV interrupts (see the ISR asm in osc.c), so
   the fastest configurable tick rate is OSC_SAMPLERATE/OSC_ISR_PRESCALER_DIV. */
#define OSC_ISR_PRESCALER_DIV (8)
#define OSC_TICKRATE_MAX (OSC_SAMPLERATE/OSC_ISR_PRESCALER_DIV)
/*
OSC period is 2**16 so Nyquist corresponds to 2**16/2. Note that's way too high
for square waves to sound OK.
*/
#define OSC_PHASE_INC_MAX (0x3FFF)
#define OSC_TICK_CALLBACK_COUNT (2)

/*
  Compile-time mixer gating (spike ATMlib2FX-hlh.4).

  Each oscillator channel's mixing block in the osc.c TIMER3 ISR can be compiled
  out independently. A channel compiled out no longer advances its phase
  accumulator or adds to the mixer sum, so its audio output is silenced, while
  its atm_synth/cmd_parse command processing is unchanged (this is the WAVE_OFF
  equivalent used by T-arg/ATMlib 2.x).

  - ATM_HAS_OSC_CHx: assemble channel x's mixer block (default 1).
  - ATM_HAS_NOISE:   master switch for the channel-3 LFSR noise path. 0 removes
                     the LFSR generator and the noise-channel output even if
                     ATM_HAS_OSC_CH3 stays 1 (default 1).

  Every default is all-on, so a build that sets none of these flags must stay
  byte-identical to the pre-flag library.
*/
#ifndef ATM_HAS_OSC_CH0
#define ATM_HAS_OSC_CH0 (1)
#endif
#ifndef ATM_HAS_OSC_CH1
#define ATM_HAS_OSC_CH1 (1)
#endif
#ifndef ATM_HAS_OSC_CH2
#define ATM_HAS_OSC_CH2 (1)
#endif
#ifndef ATM_HAS_OSC_CH3
#define ATM_HAS_OSC_CH3 (1)
#endif
#ifndef ATM_HAS_NOISE
#define ATM_HAS_NOISE (1)
#endif

enum osc_channels_e {
	OSC_CH_ZERO = 0,
	OSC_CH_ONE,
	OSC_CH_TWO,
	OSC_CH_THREE,
	OSC_CH_COUNT,
};

struct osc_params {
	uint8_t  mod;
	uint8_t  vol;
	uint16_t phase_increment;
};

extern struct osc_params osc_params_array[OSC_CH_COUNT];

typedef void (*osc_tick_callback)(const uint8_t cb_index, void *priv);

void osc_setup(void);

/* tick_hz < 8 are currently not supported */
void osc_set_tick_rate(const uint8_t callback_idx, const uint16_t rate_hz);
void osc_set_tick_callback(const uint8_t callback_idx, const osc_tick_callback cb, const void *priv);
void osc_get_tick_callback(const uint8_t callback_idx, osc_tick_callback *cb, void **priv);
