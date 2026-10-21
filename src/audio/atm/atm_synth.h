
#include <stddef.h>
#include <stdint.h>

#include "osc.h"
#include "atm_cmd_constants.h"

/* Adjust the following to reduce memory usage */

#define ATM_PATTERN_STACK_DEPTH (3)

/* Compiled FX stream backend (ATMlib2FX-aoe prototype).
 *
 * When enabled, the device plays a single sequential, pre-expanded stream from
 * a small refill buffer instead of traversing PROGMEM patterns. The runtime
 * pattern/return stacks and the PROGMEM score/SFX entry points are removed for
 * that build; see src/atm_fx_stream.c and docs/fx-footprint-results.md.
 * Default off; normal library builds are unaffected. */
#ifndef ATM_HAS_FX_STREAM
#define ATM_HAS_FX_STREAM (0)
#endif

/* Compact directly-consumed FX music backend (ATMlib2FX-hlh.5 experimental
 * candidate). Same ABI change as ATM_HAS_FX_STREAM (no runtime pattern/return
 * stacks, no PROGMEM score/SFX paths). See src/atm_fx_compact.c.
 * Default off; normal library builds are unaffected. */
#ifndef ATM_HAS_FX_COMPACT
#define ATM_HAS_FX_COMPACT (0)
#endif

/* Either compiled-stream backend removes the PROGMEM traversal ABI. */
#define ATM_HAS_FX_STREAM_BACKEND (ATM_HAS_FX_STREAM || ATM_HAS_FX_COMPACT)

/* Disable all FX to save space */
#define ATM_HAS_FX_NONE (0)

#if ATM_HAS_FX_NONE
#define ATM_HAS_FX_NOISE_RETRIG (0)
#define ATM_HAS_FX_NOTE_RETRIG (0)
#define ATM_HAS_FX_SLIDE (0)
#define ATM_HAS_FX_LFO (0)
#define ATM_HAS_FX_GLISSANDO (0)
#define ATM_HAS_FX_LOOP (0)
#else
#define ATM_HAS_FX_NOISE_RETRIG (1)
#define ATM_HAS_FX_NOTE_RETRIG (1)
#define ATM_HAS_FX_SLIDE (1)
#define ATM_HAS_FX_LFO (1)
#define ATM_HAS_FX_GLISSANDO (1)
#define ATM_HAS_FX_LOOP (1)
#endif

/* Public API - we will try not to break this, no promises! */

#define ATM_SCORE_FMT_MINIMAL_MONO (0x0)
#define ATM_SCORE_FMT_FULL_MONO (0x2)
#define ATM_SCORE_FMT_FULL (0x3)

struct atm_synth_state;
struct atm_sfx_state;
struct atm_channel_state;
struct atm_synth_ext;
struct atm_cmd_data;

typedef void (*atm_synth_ext_callback)(const uint8_t channel_count, struct atm_synth_state *synth_state, struct atm_channel_state *ch, struct atm_synth_ext *synth_ext);

struct atm_synth_ext {
	atm_synth_ext_callback cb;
	void *priv;
};

extern struct atm_synth_state atmlib_state;

void atm_synth_setup(void);

void atm_synth_play_score(const uint8_t *score);
void atm_synth_stop_score(void);
uint8_t atm_synth_is_score_stopped(void);

void atm_synth_set_score_paused(const uint8_t paused);
uint8_t atm_synth_get_score_paused(void);

/* Change the score tick rate (Hz) at runtime without a score command.
 *
 * Stored in atmlib_state.tick_rate and applied to the music tick callback
 * immediately, so it takes effect while the score is paused or stopped (during
 * playback the score tick handler re-applies it every tick). The sound-effect
 * player has its own independent rate and is unaffected.
 *
 * 0 is clamped to 1 to avoid a divide-by-zero in osc_set_tick_rate(); the upper
 * bound is ATMLIB_TICKRATE_MAX (255). Request >= 8: the osc timer divider is
 * 2000/floor(2000/rate) base events per tick, so rates below the timer's
 * supported minimum (osc.h) wrap the ISR preset and are not honoured. See
 * src/atm_synth.c for the quantized-rate table. */
void atm_synth_set_tempo(uint8_t tick_rate);

/* Cue channel (ATM_CMD_M_CUE). Sketches poll these to sync game events to the
 * music. Both operate on the single shared cue byte: score, SFX and FX backends
 * latch into the same byte, so the most recent ATM_CMD_M_CUE wins. */

/* Return the current cue value and clear it (0 = none). */
uint8_t atm_synth_get_cue(void);

/* Return 1 when the current cue equals id, without clearing it. */
uint8_t atm_synth_check_cue(uint8_t id);

void atm_synth_grab_channel(const uint8_t channel_index, struct osc_params *save);
void atm_synth_release_channel(const uint8_t channel_index);

void atm_synth_play_ext(const struct atm_synth_ext *synth_ext);
void ext_synth_command(const uint8_t ch_index, const struct atm_cmd_data *cmd, struct atm_synth_state *score_state, struct atm_channel_state *ch);

/* Play score as a sound effect on channel_index

Music playback (if any) is muted on channel_index for the duration of sound
effect playback and resumes when the sound effect is stopped or ran its course.

It is possible to start playback of a new sound effect while one is already
being played back, the active sound effect will stop and the new one will replace it.

Sound effect scores must be in ATM_SCORE_FMT_MINIMAL_MONO or ATM_SCORE_FMT_FULL_MONO format.
*/
void atm_synth_play_sfx_track(const uint8_t channel_index, const uint8_t *score,  struct atm_sfx_state *sfx_state);

/* Stop a previously started sound effect score */
void atm_synth_stop_sfx_track(struct atm_sfx_state *sfx_state);

/* Check if a sound effect score is active */
uint8_t atm_synth_is_sfx_stopped(struct atm_sfx_state *sfx_state);

/* Private structures - anything beyond this point is considered internal to the library

structs are only made available in this file so clients can allocate them statically.
All fields shoudl be considered private.

If you think you have a valid use case for exposing some feature please
get in touch with the maintainers.

*/

struct atm_synth_state {
	const uint8_t *score_start;
	uint8_t tick_rate;
	uint8_t channel_active_mute; //0b11110000;
	//                               ||||||||
	//                               |||||||└->  0  channel 0 is muted (0 = false / 1 = true)
	//                               ||||||└-->  1  channel 1 is muted (0 = false / 1 = true)
	//                               |||||└--->  2  channel 2 is muted (0 = false / 1 = true)
	//                               ||||└---->  3  channel 3 is muted (0 = false / 1 = true)
	//                               |||└----->  4  channel 0 is Active (0 = false / 1 = true)
	//                               ||└------>  5  channel 1 is Active (0 = false / 1 = true)
	//                               |└------->  6  channel 2 is Active (0 = false / 1 = true)
	//                               └-------->  7  channel 3 is Active (0 = false / 1 = true)

	/* Last cue byte written by ATM_CMD_M_CUE, or 0 for none. Public since
	 * bece748: score, SFX and FX backends all latch into this single byte
	 * (upstream semantics: score and SFX share the last-cue byte). Read and
	 * clear with atm_synth_get_cue(); compare with atm_synth_check_cue(). */
	uint8_t cue;
};

#if ATM_HAS_FX_SLIDE

struct atm_slide_params {
	int8_t slide_amount;
	uint8_t slide_config;
	uint8_t slide_count;
};

#endif

struct atm_pattern_state {
	const uint8_t *next_cmd_ptr;
	uint8_t pattern_index;
	uint8_t repetitions_counter;
};

struct atm_channel_state {
	uint8_t note;
	uint8_t vol;
	uint8_t mod;
	uint16_t delay;
	// Transposition FX
	int8_t trans_config;

#if !ATM_HAS_FX_STREAM_BACKEND
	// Nesting
	struct atm_pattern_state pstack[ATM_PATTERN_STACK_DEPTH];
	uint8_t pstack_index;
#if ATM_HAS_FX_LOOP
	uint8_t loop_pattern_index;
#endif
#endif

	struct osc_params *dst_osc_params;

#if ATM_HAS_FX_SLIDE
	// Volume & Frequency slide FX
	struct atm_slide_params vf_slide;
#endif

#if ATM_HAS_FX_NOTE_RETRIG
	// Arpeggio or Note Cut FX
	uint8_t arpNotes;       // notes: base, base+[7:4], base+[7:4]+[3:0], if FF => note cut ON
	uint8_t arpTiming;      // [7] = reserved, [6] = not third note ,[5] = retrigger, [4:0] = tick count
	uint8_t arpCount;
#endif

#if ATM_HAS_FX_NOISE_RETRIG
	// Retrig FX
	uint8_t reConfig;       // [7:2] = , [1:0] = speed // used for the noise channel
	uint8_t reCount;
#endif

#if ATM_HAS_FX_LFO
	// Tremolo or Vibrato FX
	uint8_t treviDepth;
	uint8_t treviConfig;
	uint8_t treviCount;
#endif

#if ATM_HAS_FX_GLISSANDO
	// Glissando FX
	int8_t glisConfig;
	uint8_t glisCount;
#endif
};

struct atm_sfx_state {
	uint8_t ch_index;
	struct atm_synth_state track_info;
	struct atm_channel_state channel_state;
	struct osc_params osc_params;
};

struct atm_cmd_data {
	uint8_t id;
	uint8_t params[3];
};
