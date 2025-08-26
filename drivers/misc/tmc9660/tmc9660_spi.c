/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc9660_spi


#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

#include <app/drivers/misc/tmc9660/tmc9660.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc9660_spi, CONFIG_TMC9660_SPI_LOG_LEVEL);

struct tmc9660_spi_config {
    const struct spi_dt_spec spi;
};

struct tmc9660_spi_data {
    struct k_mutex mutex; // Prevent two threads performing a command concurrently, so the request-response pairs don't get tangled
};

enum tmc9660_spi_status {
    SPI_STATUS_OK = 0xff,
    SPI_STATUS_CHECKSUM_ERROR = 0x00,
    SPI_STATUS_FIRST_CMD = 0x0c,
    SPI_STATUS_NOT_READY = 0xf0
};

static uint8_t tmc9660_spi_checksum(uint8_t buffer[8])
{
    uint8_t res = 0;
    for(int i = 0; i < 7; i++)
        res += buffer[i];
    return res;
}

static int tmc9660_spi_tmcl_command(
    const struct device *dev,
    enum tmc9660_operation_id operation,
    uint16_t type,
    uint8_t motorbank,
    uint32_t value_send,
    uint32_t *value_recv
)
{
    const struct tmc9660_spi_config *config = dev->config;
    const struct spi_dt_spec *spi = &config->spi;
    struct tmc9660_spi_data *ddata = dev->data;

    k_mutex_lock(&ddata->mutex, K_FOREVER);

    uint8_t noop[8] = {
        GetInfo, 0, 0, 0, 0, 0, 0, GetInfo
    };
    uint8_t send[8] = {
        operation,
        (type & 0xff),
        ((type&0xf00)>>4) | motorbank,
        (value_send >> 24) & 0xff,
        (value_send >> 16) & 0xff,
        (value_send >> 8) & 0xff,
        (value_send >> 0) & 0xff,
        0
    };
    uint8_t recv[8] = { 0 };

    struct spi_buf tx_buf[2] = {
        { .buf = send, .len = 8 }
    };
    struct spi_buf_set tx_bufs = {
        .buffers = tx_buf, .count = 1
    };
    struct spi_buf rx_buf[2] = {
        { .buf = recv, .len = 8 },
    };
    struct spi_buf_set rx_bufs = {
        .buffers = rx_buf, .count = 1
    };
    struct spi_buf noop_buf[2] = {
        { .buf = noop, .len = 8 },
    };
    struct spi_buf_set noop_bufs = {
        .buffers = noop_buf, .count = 1
    };

    send[7] = tmc9660_spi_checksum(send);
    
    uint8_t spi_status, tmcl_status, reply_operation;
    uint32_t data;

    k_usleep(20);

    int retries = 5;
    do {
        spi_transceive_dt(spi, &tx_bufs, &rx_bufs);

        if(recv[0] == SPI_STATUS_CHECKSUM_ERROR) {
            LOG_DBG("Previously sent bad checksum...");
            LOG_HEXDUMP_DBG(send, 8, "SPI send");
            LOG_HEXDUMP_DBG(recv, 8, "SPI recv");
        }
        if(recv[0] == SPI_STATUS_NOT_READY) {
            k_usleep(20);
        }
    }
    while((recv[0] == SPI_STATUS_NOT_READY || recv[0] == SPI_STATUS_CHECKSUM_ERROR) && retries-- > 0);
    
    k_usleep(10);

    if(retries <= 0) {
        LOG_WRN("TMC9660 took 5 retries for submitting a command");
        k_mutex_unlock(&ddata->mutex);
        return -EIO;
    }

    retries = 5;
    do {
        spi_transceive_dt(spi, &noop_bufs, &rx_bufs);
        if(recv[0] == SPI_STATUS_NOT_READY) {
            k_usleep(10);
        }
    } while(recv[0] == SPI_STATUS_NOT_READY && retries-- > 0);
    
    if(retries <= 0) {
        LOG_WRN("TMC9660 took 5 retries for answering a command");
        k_mutex_unlock(&ddata->mutex);
        return -EIO;
    }

    spi_status = recv[0];
    tmcl_status = recv[1];
    reply_operation = recv[2];
    data = (recv[3] << 24) | (recv[4] << 16) | (recv[5] << 8) | recv[6];

    if(recv[7] != tmc9660_spi_checksum(recv))
    {
        // bad checksum - rest of datagram will be malformed, incl the checksul
        LOG_HEXDUMP_ERR(recv, 8, "Checksum error in received");
        k_mutex_unlock(&ddata->mutex);
        return -EIO;
    }
    
    if(spi_status != SPI_STATUS_OK || retries < 4) {
        LOG_HEXDUMP_DBG(recv, 8, "SPI recv");
        LOG_WRN("tmcl(%d, %d, %d, %d) -> spi %d (want 255), tmcl %d (want 100), reply %d, data %08x, retries %d (want 5)", 
            operation, type, motorbank, value_send,
            spi_status, tmcl_status, reply_operation, data, retries);
    }
    /*if(tmcl_status != 100) {
        LOG_ERR("tmcl(%d, %d, %d, %d) -> spi %d (want 255), tmcl %d (want 100), reply %d, data %08x, retries %d (want 5)", 
            operation, type, motorbank, value_send,
            spi_status, tmcl_status, reply_operation, data, retries);
    }*/

    if(value_recv) *value_recv = data;

    k_mutex_unlock(&ddata->mutex);

    // TODO: sanity check for returned values...
    switch(spi_status)
    {
    case SPI_STATUS_OK: break;
    case SPI_STATUS_CHECKSUM_ERROR: return -EIO;
    case SPI_STATUS_FIRST_CMD: return -EIO; // Should never happen outside of init
    case SPI_STATUS_NOT_READY: return -EAGAIN;
    }

    switch(tmcl_status)
    {
    case REPLY_OK: return 0;
    case REPLY_CMD_LOADED: return 0;
    case REPLY_CHKERR: return -EINVAL;
    case REPLY_INVALID_CMD: return -EOPNOTSUPP;
    case REPLY_WRONG_TYPE: return -EINVAL;
    case REPLY_INVALID_VALUE: return -EINVAL;
    case REPLY_CMD_NOT_AVAILABLE: return -EOPNOTSUPP;
    case REPLY_CMD_LOAD_ERROR: return -EPERM;
    case REPLY_MAX_EXCEEDED: return -EPERM;
    case REPLY_DOWNLOAD_NOT_POSSIBLE: return -EPERM;
    }

    CODE_UNREACHABLE;
}

static int tmc9660_spi_init(
    const struct device *dev
)
{
    const struct tmc9660_spi_config *config = dev->config;
    struct tmc9660_spi_data *data = dev->data;

    k_mutex_init(&data->mutex);
    k_mutex_lock(&data->mutex, K_FOREVER);

    // Sent MST as an initial message, and check for OK or FIRST_CMD response, validating SPI communication
    char tx[8] = { 3, 0, 0, 0, 0, 0, 0, 3 };
    char rx[8];

    struct spi_buf tx_buf[1] = {
        { .buf = tx, .len = 8 }
    };
    struct spi_buf_set tx_bufs = { .buffers = tx_buf, .count = 1 };
    struct spi_buf rx_buf[1] = {
        { .buf = rx, .len = 8 }
    };
    struct spi_buf_set rx_bufs = { .buffers = rx_buf, .count = 1 };

    int retries = 3;
    do {
        LOG_HEXDUMP_DBG(tx, 8, "Init SPI send");
        spi_transceive_dt(&config->spi, &tx_bufs, &rx_bufs);
        LOG_HEXDUMP_DBG(rx, 8, "Init SPI recv");
    } while(!(rx[0] == SPI_STATUS_FIRST_CMD || rx[0] == SPI_STATUS_OK || (k_usleep(100), false)) && retries-- > 0);

    k_mutex_unlock(&data->mutex);
    
    if(rx[0] != SPI_STATUS_FIRST_CMD && rx[0] != SPI_STATUS_OK) {
        LOG_ERR("Invalid TMC9660 SPI wakeup");
        return -EIO;
    }

    return 0;
}

static const struct tmc9660_driver_api tmc9660_spi_api = {
    .tmcl_command = tmc9660_spi_tmcl_command
};

#define TMC9660_SPI_DEFINE(n)                                                              \
    static const struct tmc9660_spi_config tmc9660_spi_config_##n = {                      \
        .spi = SPI_DT_SPEC_INST_GET(n, SPI_MODE_CPHA | SPI_MODE_CPOL | SPI_WORD_SET(8), 1) \
    };                                                                                     \
                                                                                           \
    static struct tmc9660_spi_data tmc9660_spi_data_##n;                                   \
                                                                                           \
    DEVICE_DT_INST_DEFINE(n, tmc9660_spi_init, NULL,                                       \
        &tmc9660_spi_data_##n, &tmc9660_spi_config_##n,                                    \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                   \
        &tmc9660_spi_api);

DT_INST_FOREACH_STATUS_OKAY(TMC9660_SPI_DEFINE)
