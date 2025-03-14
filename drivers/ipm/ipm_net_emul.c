/*
 * Copyright (c) 2025 Lukas Ostermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_ipm_net_emul

#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/logging/log.h>

#include <stdint.h>

LOG_MODULE_REGISTER(ipm_net_emul, CONFIG_IPM_LOG_LEVEL);

struct ipm_net_emul_config {
    const char *ip;
    uint16_t port;
    bool is_master;
};

struct ipm_net_emul_data {
    int test;
};

static int ipm_net_emul_init(const struct device *dev)
{
    LOG_DBG("initialising net emul driver");
    return 0;
}

static int ipm_net_emul_send(const struct device *dev, int wait, uint32_t id,
    const void *data, int size)
{
    ARG_UNUSED(wait);
    ARG_UNUSED(data);
    ARG_UNUSED(size);

    struct ipm_net_emul_config *dev_cfg = (struct ipm_net_emul_config *)dev->config;

    LOG_DBG("sending notification to the peer id 0x%x\n", id);
    return -1;
}

static void ipm_net_emul_register_callback(const struct device *dev,
    ipm_callback_t cb, void *user_data)
{
    struct ipm_net_emul_data *dev_data = (struct ipm_net_emul_data *)dev->data;
    LOG_DBG("register callback in ipm net emul");
}

static int ipm_net_emul_set_enabled(const struct device *dev, int enable)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(enable);

    LOG_DBG("ipm net emul enabled");
    return 0;
}

static DEVICE_API(ipm, ipm_net_emul_driver_api) = {
    .send = ipm_net_emul_send,
    .register_callback = ipm_net_emul_register_callback,
    .set_enabled = ipm_net_emul_set_enabled
};

#define IPM_NET_EMUL_INIT(inst) \
    static const struct ipm_net_emul_config ipm_net_emul_cfg_##inst = { \
        .ip = DT_INST_PROP(inst, ip), \
        .port = DT_INST_PROP(inst, port) \
    }; \
    static struct ipm_net_emul_data ipm_net_emul_data_##inst = { \
        .test = 0 \
    }; \
    DEVICE_DT_INST_DEFINE(inst, \
                ipm_net_emul_init, \
                NULL, \
                &ipm_net_emul_data_##inst, &ipm_net_emul_cfg_##inst, \
                POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, \
                &ipm_net_emul_driver_api); \

DT_INST_FOREACH_STATUS_OKAY(IPM_NET_EMUL_INIT);
