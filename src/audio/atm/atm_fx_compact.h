/*
 * atm_fx_compact.h - compact, directly consumed FX music stream
 * (ATMlib2FX-hlh.5), with pause/replacement semantics and an independent-tempo
 * SFX overlay (ATMlib2FX-hlh.9).
 *
 * Experimental spike candidate, not a production ABI. A single sequential,
 * complete-record byte stream is read from FX by foreground code into a small
 * SRAM ring. Each record is a "due pass": the commands that run at the current
 * music tick for a bitmap of channels, plus one shared gap field giving the
 * number of effect-only ticks until the next pass. Per-channel delay bytecode
 * and the runtime pattern/return stacks are removed (see tools/fx_compact_spike/
 * fx_compile_compact.c); live effect/oscillator state is unchanged.
 *
 * The timer consumer preflights the whole record (length + structure) before
 * mutating any effect or channel state, applies it directly from the ring, and
 * only then advances the consumer head. It never touches SPI.
 *
 * The platform supplies the byte source:
 *   void atm_fx_compact_fetch(uint32_t offset, uint8_t *dst, uint8_t n);
 *
 * ---------------------------------------------------------------------------
 * Ownership and API surface (ATMlib2FX-hlh.9)
 *
 * The library owns: the music ring + decoder state (`atm_fx_compact_state`),
 * the four music channel states (`channels[]`) and their oscillator params
 * (`osc_params_array[]`), and (when `ATM_FX_COMPACT_SFX`) one borrowed voice for
 * the SFX overlay. The caller owns the FX data page (`FX::begin`), the compact
 * byte streams (their lifetime must extend from `play`/`play_sfx` to completion
 * or the matching `stop`) and the foreground main-loop service call.
 *
 * `play` (replacement) is atomic with respect to the timer: it disables the
 * timer callback, resets the ring/descriptor, prefills the first record, then
 * arms. Unread bytes of a previous track are discarded; no partially mixed
 * track state is observable. `stop` mutes the owned voices and unregisters.
 * `pause` preserves ring position and all live state while muting, and
 * `resume` restores the saved voice volumes and re-arms.
 *
 * SFX (`ATM_FX_COMPACT_SFX=1`) is a second, independent compact stream stored in
 * PROGMEM and read by the timer directly (no FX SPI, no second ring, no
 * mandatory second queue). It runs on its own tempo (callback 1), borrows one
 * music channel (save + zero + redirect the music channel's effects into the
 * saved shadow), and on natural stop, explicit stop, or replacement restores
 * the current music voice from that shadow.
 */
#ifndef ATM_FX_COMPACT_H
#define ATM_FX_COMPACT_H

#include <stdint.h>

/* Music ring size (power of two). 32-byte target; judged by the spike. */
#ifndef ATM_FX_COMPACT_RING
#define ATM_FX_COMPACT_RING (32)
#endif
/* Largest foreground FX fetch per transaction (direct into the ring). */
#define ATM_FX_COMPACT_CHUNK (16)

/* Diagnostics (counters + trace). Off in production-size builds. */
#ifndef ATM_FX_COMPACT_DIAG
#define ATM_FX_COMPACT_DIAG (0)
#endif

/* Independent-tempo SFX overlay (ATMlib2FX-hlh.9). Off pays nothing; only
 * enabled builds carry the SFX state, reader code and PROGMEM asset reader. */
#ifndef ATM_FX_COMPACT_SFX
#define ATM_FX_COMPACT_SFX (0)
#endif

/* Global-loop streams (ATMlib2FX-hlh.8), magic 0xC1. Off by default so the
 * finite build and its code size are unchanged; when enabled the decoder also
 * accepts loop streams. A region that fits the ring is pinned and replayed in
 * place (no further fetch); a larger region streams through the ring
 * (ATMlib2FX-hlh.16), the foreground producer re-fetching the region from FX at
 * the wrap so the timer never sees END. Requires ATM_FX_COMPACT. */
#ifndef ATM_FX_COMPACT_LOOP
#define ATM_FX_COMPACT_LOOP (0)
#endif

/* Largest loop region accepted by the decoder (ATMlib2FX-hlh.16). Bounded by
 * the uint16 loop-region counter: the loop_start_rel u24 field could address
 * more, but a streamed region is tracked in 16 bits. A region above this is a
 * clean format rejection (never a garbled replay). */
#ifndef ATM_FX_COMPACT_LOOP_MAX_REGION
#define ATM_FX_COMPACT_LOOP_MAX_REGION (0xFFFF)
#endif

/* Streaming-loop sub-feature of ATM_FX_COMPACT_LOOP (ATMlib2FX-hlh.16): when
 * enabled the decoder also accepts a loop region larger than the ring and
 * streams it (the foreground producer re-fetches the region at the wrap, so the
 * timer never sees END). On by default for loop builds; disable to recover
 * exactly the pinned-only hlh.8 decoder, which is how the footprint delta of
 * the streaming code/SRAM is measured. Requires ATM_FX_COMPACT_LOOP. */
#ifndef ATM_FX_COMPACT_LOOP_STREAMING
#define ATM_FX_COMPACT_LOOP_STREAMING (ATM_FX_COMPACT_LOOP)
#endif
#if ATM_FX_COMPACT_LOOP_STREAMING && !ATM_FX_COMPACT_LOOP
#error "ATM_FX_COMPACT_LOOP_STREAMING requires ATM_FX_COMPACT_LOOP"
#endif

#if (ATM_FX_COMPACT_RING & (ATM_FX_COMPACT_RING - 1)) != 0
#error "ATM_FX_COMPACT_RING must be a power of two"
#endif
#if ATM_FX_COMPACT_RING > 256
#error "ATM_FX_COMPACT_RING must fit a uint8_t index"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Requires `struct atm_synth_state`, `struct atm_channel_state` and
 * `struct osc_params` (include <atm_synth.h> first).
 */
struct atm_fx_compact_state {
	uint8_t buf[ATM_FX_COMPACT_RING];
	uint8_t head;          /* consumer read index (timer)       */
	uint8_t tail;          /* producer write index (foreground) */
	uint32_t next_offset;  /* next FX stream byte to fetch      */
	uint32_t stream_end;   /* one past the last FX stream byte   */
	uint8_t active;        /* music player armed                 */
	uint16_t gap;          /* effect-only ticks until the next pass */
	uint8_t error;         /* sticky 1=format 2=oversize (0 = none) */
	uint8_t paused;        /* music paused, position/live state kept */
	uint8_t paused_vol[4]; /* saved music osc vols while paused  */
#if ATM_FX_COMPACT_LOOP
	/* --- global-loop stream state (ATMlib2FX-hlh.8) --- */
	uint32_t stream_base;   /* absolute offset of the stream byte 0 */
	uint32_t head_offset;   /* absolute stream offset of buf[head]  */
	uint32_t loop_start;    /* absolute offset of the loop region   */
	uint16_t loop_bytes;    /* loop region length in bytes          */
	uint16_t loop_remaining;/* bytes left in the current loop pass  */
	uint8_t loop_base;      /* ring index of the loop region start  */
	uint8_t loopable;       /* stream is a 0xC1 loop stream         */
	uint8_t loop_pinned;    /* loop region resident, no more fetch  */
#if ATM_FX_COMPACT_LOOP_STREAMING
	uint8_t loop_streaming; /* region > ring: the foreground producer
	                         * re-fetches the region at the wrap, so the
	                         * timer never sees END (ATMlib2FX-hlh.16) */
#endif
#endif
#if ATM_FX_COMPACT_SFX
	/* --- independent-tempo SFX overlay (PROGMEM compact stream) --- */
	const uint8_t *sfx_ptr;   /* PROGMEM base of the SFX stream  */
	uint32_t sfx_end;         /* descriptor length in bytes      */
	uint32_t sfx_pos;         /* next SFX byte (from sfx_ptr)    */
	uint16_t sfx_gap;         /* effect-only ticks until next SFX pass */
	uint8_t sfx_active;
	uint8_t sfx_ch;           /* borrowed music channel          */
	struct osc_params sfx_save;            /* borrowed music voice */
	struct atm_synth_state sfx_track;      /* SFX player state     */
	struct atm_channel_state sfx_channel;  /* SFX channel state    */
#endif
#if ATM_FX_COMPACT_DIAG
	uint16_t ticks;
	uint16_t underruns;
	uint16_t service_gap;  /* ticks since last service()        */
	uint16_t max_service_gap;
	uint16_t serviced;
	uint16_t stop_tick;
	uint8_t max_record;    /* largest record consumed            */
	uint8_t window;        /* bytes consumed since last service  */
	uint8_t max_window;    /* largest such window                */
#if ATM_FX_COMPACT_SFX
	uint16_t sfx_ticks;       /* SFX callback count              */
	uint16_t sfx_start_tick;  /* music tick at SFX start         */
	uint16_t sfx_stop_tick;   /* music tick at SFX stop          */
	uint16_t sfx_last_inc;    /* SFX voice phase_increment (last SFX tick) */
	uint16_t sfx_restore_inc; /* borrowed voice phase_increment after restore */
	uint8_t sfx_borrow;       /* borrowed channel index          */
	uint8_t sfx_restore_ok;   /* 1 after a restore completed     */
#endif
#endif
};

extern struct atm_fx_compact_state atm_fx_compact_state;

/* Platform byte source (implemented by the sketch / host harness). */
void atm_fx_compact_fetch(uint32_t offset, uint8_t *dst, uint8_t n);

/* Replace the current track atomically with the stream at `offset`
 * (FX-section relative), `length` bytes long. Stops/mutes any previous
 * playback, resets the channels, prefills the first record, then arms. The
 * first due tick therefore cannot underrun. */
void atm_fx_compact_play(uint32_t offset, uint32_t length);

/* Foreground refill. Call when no FX/OLED transaction is in progress. */
void atm_fx_compact_service(void);

/* Stop the music player, mute the owned voices and unregister (also stops any
 * active SFX overlay, restoring its borrowed voice). */
void atm_fx_compact_stop(void);

/* True while the music player is armed (a paused player is still active). */
uint8_t atm_fx_compact_is_active(void);

/* Pause: keep ring position and live state, mute the owned voices, unregister
 * the timer; no tick advances until resume. */
void atm_fx_compact_pause(void);
/* Resume: restore the saved voice volumes and re-arm at the preserved position. */
void atm_fx_compact_resume(void);
uint8_t atm_fx_compact_is_paused(void);

#if ATM_FX_COMPACT_SFX
/* Play a compact SFX stream stored in PROGMEM (`length` bytes, magic + records
 * + END), borrowing `channel_index` from the music player at independent tempo.
 * Stops/replaces any current SFX first (restoring its borrowed voice) and
 * saves, zeroes and redirects the named music channel. Returns nothing; check
 * is_sfx_active(). */
void atm_fx_compact_play_sfx(uint8_t channel_index, const uint8_t *stream, uint32_t length);

/* Stop the SFX overlay and restore the borrowed music voice. Safe when idle. */
void atm_fx_compact_stop_sfx(void);

uint8_t atm_fx_compact_is_sfx_active(void);
#endif /* ATM_FX_COMPACT_SFX */

#ifdef __cplusplus
}
#endif

#endif /* ATM_FX_COMPACT_H */
