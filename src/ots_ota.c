#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

#include "ots_ota.h"

LOG_MODULE_REGISTER(ots_ota, CONFIG_BT_OTS_LOG_LEVEL);

#define SLOT1_LABEL		slot1_partition
#define SLOT1_ID		FIXED_PARTITION_ID(SLOT1_LABEL)
#define SLOT1_DEV		FIXED_PARTITION_DEVICE(SLOT1_LABEL)
#define STREAM_FLASH_BUF_SIZE 1024

static struct stream_flash_ctx stream_ctx;
static uint8_t stream_flash_buf[STREAM_FLASH_BUF_SIZE];
static struct bt_ots *ots_instance;

static int ots_obj_created(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
			   const struct bt_ots_obj_add_param *param,
			   struct bt_ots_obj_created_desc *created_desc)
{
    static char firmware_name[] = "firmware.bin";

	LOG_INF("OTS Callback: Object Created ID 0x%llx, Size %u", id, param->size);

    created_desc->name = firmware_name;
    created_desc->size.alloc = param->size;
    created_desc->size.cur = 0;
    
    BT_OTS_OBJ_SET_PROP_WRITE(created_desc->props);
    BT_OTS_OBJ_SET_PROP_READ(created_desc->props);

	LOG_INF("OTS Callback: Object registered successfully. Ready for Write.");

	return 0;
}

static int ots_obj_write(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
			 const void *data, size_t len, off_t offset, size_t rem)
{
	int err;

	LOG_DBG("OTS Callback: Write Request. Offset %ld, Len %zu, Rem %zu", (long)offset, len, rem);

    if (offset == 0) {
        LOG_INF("OTS: Initializing Flash for new transfer...");
        
        if (!device_is_ready(SLOT1_DEV)) {
		    LOG_ERR("OTS: Error - Flash device not ready");
            return -ENODEV;
	    }

        err = stream_flash_init(&stream_ctx, SLOT1_DEV,
                    stream_flash_buf, sizeof(stream_flash_buf),
                    FIXED_PARTITION_OFFSET(SLOT1_LABEL),
                    FIXED_PARTITION_SIZE(SLOT1_LABEL),
                    NULL);
        if (err) {
            LOG_ERR("OTS: Error - Stream flash init failed (err %d)", err);
            return err;
        }
    }

	err = stream_flash_buffered_write(&stream_ctx, data, len, (rem == 0));
	if (err) {
		LOG_ERR("OTS: Error - Stream flash write failed (err %d)", err);
		return err;
	}

	if (rem == 0) {
		LOG_INF("OTS: Transfer complete! Finalizing...");
		
#ifdef CONFIG_BOOTLOADER_MCUBOOT
        err = boot_request_upgrade(BOOT_UPGRADE_TEST);
        if (err) {
            LOG_ERR("OTS: Error - Failed to request upgrade (err %d)", err);
        } else {
            LOG_INF("OTS: Upgrade requested. Rebooting...");
            k_sleep(K_SECONDS(2));
            sys_reboot(SYS_REBOOT_COLD);
        }
#else
        LOG_INF("OTS: MCUboot not enabled. Rebooting in 2s...");
        k_sleep(K_SECONDS(2));
        sys_reboot(SYS_REBOOT_COLD);
#endif
	}

	return len;
}

static struct bt_ots_cb ots_callbacks = {
	.obj_created = ots_obj_created,
	.obj_write = ots_obj_write,
};

int ots_ota_init(void)
{
	int err;

	ots_instance = bt_ots_free_instance_get();
	if (!ots_instance) {
		LOG_ERR("OTS: Error - Failed to retrieve OTS instance");
		return -ENOTCONN;
	}

    struct bt_ots_init_param ots_init = {0};
    
    BT_OTS_OACP_SET_FEAT_READ(ots_init.features.oacp);
    BT_OTS_OACP_SET_FEAT_WRITE(ots_init.features.oacp);
    BT_OTS_OACP_SET_FEAT_CREATE(ots_init.features.oacp);
    BT_OTS_OLCP_SET_FEAT_GO_TO(ots_init.features.olcp);
    
	ots_init.cb = &ots_callbacks;

    err = bt_ots_init(ots_instance, &ots_init);
    if (err) {
         LOG_ERR("OTS: Error - Failed to init OTS (err %d)", err);
         return err;
    }

    static struct bt_ots_obj_add_param add_param;
    add_param.size = FIXED_PARTITION_SIZE(SLOT1_LABEL);
    add_param.type.uuid.type = BT_UUID_TYPE_16;
    add_param.type.uuid_16.val = BT_UUID_OTS_TYPE_UNSPECIFIED_VAL;
    
    err = bt_ots_obj_add(ots_instance, &add_param);
    if (err < 0) {
        LOG_ERR("OTS: Error - Failed to add firmware object (err %d)", err);
        return err;
    }

    LOG_INF("OTS: Firmware object added (Max Size: %u)", add_param.size);

    return 0;
}
