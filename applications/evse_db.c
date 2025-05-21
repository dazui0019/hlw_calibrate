#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <fal.h>
#include <flashdb.h>
#include "evse_db.h"

#define LOG_TAG "app.evse_db"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

#include <sfud.h>
#if (RTTHREAD_VERSION >= RT_VERSION_CHECK(5, 2, 0))
    #include <dev_spi_flash.h>
    #include <dev_spi_flash_sfud.h>
#else
    #include <spi_flash.h>
    #include <spi_flash_sfud.h>
#endif

const struct fal_partition *fdb_part = RT_NULL;

/* hlw8032 */
static uint32_t VparamXK[3] = {0, 0, 0};
static uint32_t IparamXK[3] = {0, 0, 0};
static uint32_t PparamXK[3] = {0, 0, 0};

static uint32_t boot_count = 0;
static uint32_t calibrated = 0;
/* default KV nodes */
static struct fdb_default_kv_node default_kv_table[] = {
        {"boot_count", &boot_count, sizeof(boot_count)},    /* 重启次数 */
        {"calibrated", &calibrated, sizeof(calibrated)},    /* 是否校准 */
        {"VparamXK", &VparamXK, sizeof(VparamXK)},          /* 电压校准参数 */
        {"IparamXK", &IparamXK, sizeof(IparamXK)},          /* 电流校准参数 */
        {"PparamXK", &PparamXK, sizeof(PparamXK)},          /* 功率校准参数 */
};
/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };

static struct fdb_default_kv default_kv;

int evse_db_init(void)
{
    rt_ssize_t result = 0;
    struct fdb_blob blob;

    /* SFUD initialize */
    rt_sfud_flash_probe("gd25q32", "spi10");

    /* FAL initialize */
    fal_init();

// #define FDB_PART_ERASE
#ifdef FDB_PART_ERASE
    /* erase the FAL partition */
    log_d("erase the FAL partition");
    fdb_part = fal_partition_find("fdb_kvdb1");
    if (fdb_part == RT_NULL) {
        log_e("fdb_kvdb1 partition not found");
        return -1;
    }
    result = fal_partition_erase(fdb_part, 0, fdb_part->len);
    if(result == -1) {
        log_e("fal_partition_erase failed!");
        return result;
    }else{
        log_d("fal_partition_erase success!");
    }
#endif

    /* default KV initialize */
    default_kv.kvs = default_kv_table;
    default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

    /* FDB initialize */
    result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);
    if(result != FDB_NO_ERR) {
        log_e("fdb_kvdb_init failed!");
        return -1;
    }

    /* get the "boot_count" KV value */
    fdb_kv_get_blob(&kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
    /* the blob.saved.len is more than 0 when get the value successful */
    if (blob.saved.len > 0) {
        log_d("get the 'boot_count' value is %d", boot_count);
    } else {
        log_e("get the 'boot_count' failed");
    }

    /* increase the boot count */
    boot_count ++;
    /* change the "boot_count" KV's value */
    fdb_kv_set_blob(&kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
    log_d("set the 'boot_count' value to %d", boot_count);

    return 0;
}
INIT_APP_EXPORT(evse_db_init);

int evse_db_get_hlw_param(uint32_t VparamXK[], uint32_t IparamXK[], uint32_t PparamXK[])
{
    rt_ssize_t result = 0;
    struct fdb_blob blob;

    result = fdb_kv_get_blob(&kvdb, "VparamXK", fdb_blob_make(&blob, VparamXK, 3*sizeof(uint32_t)));
    if(result <= 0) {
        log_e("Get VparamXK failed!");
        return -RT_ERROR;
    }
    result = fdb_kv_get_blob(&kvdb, "IparamXK", fdb_blob_make(&blob, IparamXK, 3*sizeof(uint32_t)));
    if(result <= 0) {
        log_e("Get IparamXK failed!");
        return -RT_ERROR;
    }
    result = fdb_kv_get_blob(&kvdb, "PparamXK", fdb_blob_make(&blob, PparamXK, 3*sizeof(uint32_t)));
    if(result <= 0) {
        log_e("Get PparamXK failed!");
        return -RT_ERROR;
    }

    log_d("Get hlw param success!");
    return 0;
}

int evse_db_set_hlw_param(uint32_t VparamXK[], uint32_t IparamXK[], uint32_t PparamXK[])
{
    rt_ssize_t result = 0;
    struct fdb_blob blob;
    
    result = fdb_kv_set_blob(&kvdb, "VparamXK", fdb_blob_make(&blob, VparamXK, 3*sizeof(uint32_t)));
    if(result != FDB_NO_ERR) {
        log_e("Set VparamXK failed!");
        return -RT_ERROR;
    }
    result = fdb_kv_set_blob(&kvdb, "IparamXK", fdb_blob_make(&blob, IparamXK, 3*sizeof(uint32_t)));
    if(result != FDB_NO_ERR) {
        log_e("Set IparamXK failed!");
        return -RT_ERROR;
    }
    result = fdb_kv_set_blob(&kvdb, "PparamXK", fdb_blob_make(&blob, PparamXK, 3*sizeof(uint32_t)));
    if(result != FDB_NO_ERR) {
        log_e("Set PparamXK failed!");
        return -RT_ERROR;
    }
    evse_db_set_calibrated(1);
    log_d("Set hlw param success!");
    return 0;
}

int evse_db_get_calibrated(uint32_t *calibrated)
{
    struct fdb_blob blob;
    return fdb_kv_get_blob(&kvdb, "calibrated", fdb_blob_make(&blob, calibrated, sizeof(calibrated)));
}

int evse_db_set_calibrated(uint32_t calibrated)
{
    struct fdb_blob blob;
    return fdb_kv_set_blob(&kvdb, "calibrated", fdb_blob_make(&blob, &calibrated, sizeof(calibrated)));
}
