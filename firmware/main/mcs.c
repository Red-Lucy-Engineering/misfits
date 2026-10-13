#include "mcs.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "os/os_mbuf.h"

#include "ble_sec.h"

static const char *TAG = "misfits_mcs";

static const ble_uuid16_t mcs_svc_uuid        = BLE_UUID16_INIT(0x1848);
static const ble_uuid16_t chr_player_uuid     = BLE_UUID16_INIT(0x2B93);
static const ble_uuid16_t chr_title_uuid      = BLE_UUID16_INIT(0x2B97);
static const ble_uuid16_t chr_duration_uuid   = BLE_UUID16_INIT(0x2B98);
static const ble_uuid16_t chr_position_uuid   = BLE_UUID16_INIT(0x2B99);
static const ble_uuid16_t chr_state_uuid      = BLE_UUID16_INIT(0x2BA3);

/* 6d697366-6974-7300-a000-000000000001 ("misfits"), BLE_UUID128_INIT expects little-endian */
static const ble_uuid128_t chr_artist_uuid = BLE_UUID128_INIT(
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
    0x00, 0x73, 0x74, 0x69, 0x66, 0x73, 0x69, 0x6d);

/* Zugriff kommt aus dem NimBLE-Host-Task, gelesen wird aus app_main -> Spinlock. */
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;

/* position_cs iis base position at time of s_pos_tick. */
static mcs_snapshot_t s_st;
static TickType_t     s_pos_tick;
static bool           s_changed;

/* current position; caller holds s_lock. */
static int32_t position_now_locked(void)
{
    if (s_st.position_cs < 0) {
        return MCS_TIME_UNKNOWN;
    }
    int64_t p = s_st.position_cs;
    if (s_st.state == MCS_STATE_PLAYING) {
        uint64_t dt_ms = (uint64_t)(TickType_t)(xTaskGetTickCount() - s_pos_tick) * portTICK_PERIOD_MS;
        p += (int64_t)(dt_ms / 10);
    }
    if (s_st.duration_cs >= 0 && p > s_st.duration_cs) {
        p = s_st.duration_cs;
    }
    return (int32_t)p;
}

/* cuts off at max bytes, without breaking UTF-8 characters */
static size_t utf8_clip(const uint8_t *s, size_t len, size_t max)
{
    if (len <= max) {
        return len;
    }
    size_t n = max;
    while (n > 0 && (s[n] & 0xC0) == 0x80) {
        n--;
    }
    return n;
}

static int append(struct ble_gatt_access_ctxt *ctxt, const void *data, size_t len)
{
    return os_mbuf_append(ctxt->om, data, len) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int32_t le_to_i32(const uint8_t b[4])
{
    return (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8) |
                     ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

static void i32_to_le(int32_t v, uint8_t b[4])
{
    uint32_t u = (uint32_t)v; /* -1 -> 0xFFFFFFFF = "unknown" */
    b[0] = u & 0xFF;
    b[1] = (u >> 8) & 0xFF;
    b[2] = (u >> 16) & 0xFF;
    b[3] = (u >> 24) & 0xFF;
}

/* handler for player name, title, artist; arg points to target field in s_st. */
static int string_access(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *field = arg;

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        uint8_t buf[MCS_STR_MAX + 1];

        /* Zu lange Strings werden gekuerzt statt abgelehnt. */
        uint16_t n = len > sizeof(buf) ? sizeof(buf) : len;
        if (os_mbuf_copydata(ctxt->om, 0, n, buf) != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        size_t tlen = utf8_clip(buf, n, MCS_STR_MAX);
        tlen = strnlen((const char *)buf, tlen);

        portENTER_CRITICAL(&s_lock);
        if (strlen(field) != tlen || memcmp(field, buf, tlen) != 0) {
            memcpy(field, buf, tlen);
            field[tlen] = '\0';
            s_changed = true;
        }
        portEXIT_CRITICAL(&s_lock);
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        char tmp[MCS_STR_MAX + 1];
        portENTER_CRITICAL(&s_lock);
        strlcpy(tmp, field, sizeof(tmp));
        portEXIT_CRITICAL(&s_lock);
        return append(ctxt, tmp, strlen(tmp));
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static int duration_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint8_t b[4];
        if (OS_MBUF_PKTLEN(ctxt->om) != sizeof(b)) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        if (os_mbuf_copydata(ctxt->om, 0, sizeof(b), b) != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        int32_t v = le_to_i32(b);
        if (v < 0) {
            v = MCS_TIME_UNKNOWN;
        }

        portENTER_CRITICAL(&s_lock);
        if (s_st.duration_cs != v) {
            s_st.duration_cs = v;
            s_changed = true;
        }
        portEXIT_CRITICAL(&s_lock);
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t b[4];
        portENTER_CRITICAL(&s_lock);
        i32_to_le(s_st.duration_cs, b);
        portEXIT_CRITICAL(&s_lock);
        return append(ctxt, b, sizeof(b));
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static int position_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint8_t b[4];
        if (OS_MBUF_PKTLEN(ctxt->om) != sizeof(b)) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        if (os_mbuf_copydata(ctxt->om, 0, sizeof(b), b) != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        int32_t v = le_to_i32(b);
        if (v < 0) {
            v = MCS_TIME_UNKNOWN;
        }

        /* new base position + timestamp. doesn't cause "changed" on purpose,
         * so ePaper doesn't do full refresh every time. */
        portENTER_CRITICAL(&s_lock);
        s_st.position_cs = v;
        s_pos_tick = xTaskGetTickCount();
        portEXIT_CRITICAL(&s_lock);
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t b[4];
        portENTER_CRITICAL(&s_lock);
        i32_to_le(position_now_locked(), b);
        portEXIT_CRITICAL(&s_lock);
        return append(ctxt, b, sizeof(b));
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static int state_access(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint8_t v;
        if (OS_MBUF_PKTLEN(ctxt->om) != 1) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        if (os_mbuf_copydata(ctxt->om, 0, 1, &v) != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        if (v > MCS_STATE_SEEKING) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        portENTER_CRITICAL(&s_lock);
        if (s_st.state != v) {
            /* freeze position at state change,
             * so extrapolation doesn't run past the pause */
            s_st.position_cs = position_now_locked();
            s_pos_tick = xTaskGetTickCount();
            s_st.state = v;
            s_changed = true;
        }
        portEXIT_CRITICAL(&s_lock);
        ESP_LOGI(TAG, "media state %u", v);
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        portENTER_CRITICAL(&s_lock);
        uint8_t v = s_st.state;
        portEXIT_CRITICAL(&s_lock);
        return append(ctxt, &v, 1);
    }

    return BLE_ATT_ERR_UNLIKELY;
}

#define MCS_RW_FLAGS (BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_DATA_SEC_FLAGS)

static const struct ble_gatt_svc_def mcs_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &mcs_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &chr_player_uuid.u,
                .access_cb = string_access,
                .arg = s_st.player,
                .flags = MCS_RW_FLAGS,
            },
            {
                .uuid = &chr_title_uuid.u,
                .access_cb = string_access,
                .arg = s_st.title,
                .flags = MCS_RW_FLAGS,
            },
            {
                .uuid = &chr_artist_uuid.u,
                .access_cb = string_access,
                .arg = s_st.artist,
                .flags = MCS_RW_FLAGS,
            },
            {
                .uuid = &chr_duration_uuid.u,
                .access_cb = duration_access,
                .flags = MCS_RW_FLAGS,
            },
            {
                .uuid = &chr_position_uuid.u,
                .access_cb = position_access,
                .flags = MCS_RW_FLAGS,
            },
            {
                .uuid = &chr_state_uuid.u,
                .access_cb = state_access,
                .flags = MCS_RW_FLAGS,
            },
            { 0 },
        },
    },
    { 0 },
};

int mcs_init(void)
{
    memset(&s_st, 0, sizeof(s_st));
    s_st.state = MCS_STATE_INACTIVE;
    s_st.duration_cs = MCS_TIME_UNKNOWN;
    s_st.position_cs = MCS_TIME_UNKNOWN;
    s_pos_tick = xTaskGetTickCount();
    s_changed = false;

    int rc = ble_gatts_count_cfg(mcs_svcs);
    if (rc != 0) {
        return rc;
    }
    return ble_gatts_add_svcs(mcs_svcs);
}

void mcs_get(mcs_snapshot_t *out)
{
    portENTER_CRITICAL(&s_lock);
    *out = s_st;
    out->position_cs = position_now_locked();
    portEXIT_CRITICAL(&s_lock);
}

bool mcs_take_changed(void)
{
    portENTER_CRITICAL(&s_lock);
    bool c = s_changed;
    s_changed = false;
    portEXIT_CRITICAL(&s_lock);
    return c;
}
