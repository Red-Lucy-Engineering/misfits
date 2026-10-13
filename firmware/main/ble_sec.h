#ifndef _BLE_SEC_H
#define _BLE_SEC_H

#include "host/ble_gatt.h"

/*
 * Encryption (Bonding) for ANS/MCS Characteristics must be enforced.
 * Otherwise, any device in range could display Notifications/ Song Titles on the Clock.
 * For debugging with nRF Connect etc., set to 0.
 */
#ifndef MISFITS_REQUIRE_ENCRYPTION
#define MISFITS_REQUIRE_ENCRYPTION 1
#endif

#if MISFITS_REQUIRE_ENCRYPTION
#define BLE_DATA_SEC_FLAGS (BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_WRITE_ENC)
#else
#define BLE_DATA_SEC_FLAGS 0
#endif

#endif //_BLE_SEC_H
