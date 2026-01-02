#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include "ots_ota.h"

LOG_MODULE_REGISTER(main, CONFIG_BT_L2CAP_LOG_LEVEL);

#define DEVICE_NAME      CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN  (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_OTS_VAL)),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Bluetooth: Connection failed (err 0x%02x)", err);
	} else {
		LOG_INF("Bluetooth: Connected");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Bluetooth: Disconnected (reason 0x%02x)", reason);
    
    // Restart advertising
    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Bluetooth: Advertising failed to restart (err %d)", err);
    } else {
        LOG_INF("Bluetooth: Advertising restarted");
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	int err;

	LOG_INF("*** Starting OTS OTA App v2.0.1 ***");

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth: Init failed (err %d)", err);
		return 0;
	}

    err = ots_ota_init();
    if (err) {
        LOG_ERR("OTS OTA Init failed (err %d)", err);
    }

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Bluetooth: Advertising start failed (err %d)", err);
		return 0;
	}

	LOG_INF("Bluetooth: Advertising started");

	return 0;
}
