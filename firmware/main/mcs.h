#ifndef _MCS_H
#define _MCS_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Media Control Service (0x1848), Watch = GATT-Server, Phone (Gadgetbridge) is writing.
 *
 * Diverging from the standard: the Characteristics are READ|NOTIFY in the standard
 *  and are coming from the media player device (Phone). Here they are READ|WRITE
 * so Gadgetbridge can write to the watch as GATT-client (same direction as with CTS).
 *
 * Characteristics:
 *   0x2B93 Media Player Name   READ|WRITE  UTF-8, max MCS_STR_MAX Byte
 *   0x2B97 Track Title         READ|WRITE  UTF-8, max MCS_STR_MAX Byte
 *   0x2B98 Track Duration      READ|WRITE  int32 LE, 1/100 s, 0xFFFFFFFF = unknown
 *   0x2B99 Track Position      READ|WRITE  int32 LE, 1/100 s, 0xFFFFFFFF = unavailable
 *   0x2BA3 Media State         READ|WRITE  uint8: 0 Inactive, 1 Playing, 2 Paused, 3 Seeking
 *   6d697366-6974-7300-a000-000000000001
 *          Track Artist (custom; MCS doesn't know artist)  READ|WRITE  UTF-8
 *
 * Position only has to be written for play/pause/seek/track switching: while
 * "playing", the watch is calculating on it's own (mcs_get() delivers extrapolated
 * position). A new track should be sent with duration and position == 0
 */

#define MCS_STR_MAX     64
#define MCS_TIME_UNKNOWN (-1)

typedef enum {
    MCS_STATE_INACTIVE = 0,
    MCS_STATE_PLAYING  = 1,
    MCS_STATE_PAUSED   = 2,
    MCS_STATE_SEEKING  = 3,
} mcs_state_t;

typedef struct {
    char     player[MCS_STR_MAX + 1];
    char     title[MCS_STR_MAX + 1];
    char     artist[MCS_STR_MAX + 1];
    uint8_t  state;          /* mcs_state_t */
    int32_t  duration_cs;    /* 1/100 s, MCS_TIME_UNKNOWN if unknown */
    int32_t  position_cs;    /* 1/100 s, extrapolated, MCS_TIME_UNKNOWN if unknown */
} mcs_snapshot_t;

/** registers service. call before nimble_port_freertos_init(). 0 = OK. */
int mcs_init(void);

/** consistent snapshot of player state. */
void mcs_get(mcs_snapshot_t *out);

/**
 * true, if title/artist/player/status/duration changed since last function call
 * (resets flag).
 */
bool mcs_take_changed(void);

#endif //_MCS_H
