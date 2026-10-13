#ifndef _ANS_H
#define _ANS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Alert Notification Service (0x1811), Watch = GATT-Server, Phone (Gadgetbridge) writes.
 *
 * Diverging from standard: 0x2A45 / 0x2A46 are "Notify only" in the standard
 * and would come from the phone. Here, they're READ|WRITE, so Gadgetbridge 
 * can write them to the watch as GATT-Client (same as CTS).
 *
 * Characteristics:
 *   0x2A47 Supported New Alert Category     READ   uint16 Bitmask (0x03FF)
 *   0x2A48 Supported Unread Alert Category  READ   uint16 Bitmask (0x03FF)
 *   0x2A45 Unread Alert Status              READ|WRITE
 *            [category:u8][count:u8]   count 0 = clear, 255 = ">=255"
 *            category 0xFF + count 0   = clear all categories
 *   0x2A46 New Alert                        READ|WRITE
 *            [category:u8][count:u8][text: 0..18 Byte UTF-8]  -> "last Alert"-Text
 *
 * Categories (Bluetooth Assigned Numbers):
 *   0 Simple Alert, 1 Email, 2 News, 3 Call, 4 Missed Call,
 *   5 SMS/MMS, 6 Voice Mail, 7 Schedule, 8 High Prioritized, 9 Instant Message
 */

#define ANS_NUM_CATEGORIES   10
#define ANS_CATEGORY_ALL     0xFF
#define ANS_TEXT_MAX         18

/** registers service. call before nimble_port_freertos_init(). 0 = OK. */
int ans_init(void);

/** sum of all unread notifications (all categories). */
uint16_t ans_get_unread_total(void);

/** unread notifications of a category (0..9), otherwise 0. */
uint8_t ans_get_unread(uint8_t category);

/**
 * copies text of last incoming alert (NUL-terminated) to out.
 * Returns false, when there's no alert yet. Category is allowed to be NULL
 */
bool ans_get_last_alert(char *out, size_t out_size, uint8_t *category);

/** true if something changed since last call (resets Flag). */
bool ans_take_changed(void);

#endif //_ANS_H
