/*
 * Copyright (c) 2025 Lukas Ostermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_ipm_net_emul

#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <arpa/inet.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netdb.h>

LOG_MODULE_REGISTER(ipm_net_emul, CONFIG_IPM_LOG_LEVEL);

struct ipm_net_emul_config {
    const char *ip;
    uint16_t port;
    bool is_master;
};

struct ipm_net_emul_data {
    int sock_fd;
    struct sockaddr_in address;
};

/**
 * @brief Register and configure the socket on the host
 *
 * @param data Private data of this instance of the device
 * @return 0 if successful, negative error code otherwise
 */
static int ipm_net_emul_init_socket(struct ipm_net_emul_data *data)
{
    const int opt = 1;

    /* Create socket file descriptor */
    data->sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (data->sock_fd < 0) {
        LOG_ERR("Failed to create socket in the host network stack");
        return -EAGAIN;
    }

    /* Allow immediate reuse of the address upon restart */
    if (setsockopt(data->sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt)) != 0) {
        LOG_ERR("Failed to set socket options");
        shutdown(data->sock_fd, SHUT_RDWR);
        close(data->sock_fd);
        data->sock_fd = -1;
        return -EAGAIN;
    }

    return 0;
}

/**
 * @brief Enable the IPM device in master role
 *
 * @param dev Device structure
 * @return 0 if successful, negative error code otherwise
 */
static int ipm_net_emul_enable_master(const struct device *dev)
{
    struct ipm_net_emul_data *data = (struct ipm_net_emul_data *)dev->data;
    const struct ipm_net_emul_config *conf = (const struct ipm_net_emul_config *)dev->config;

    /* Listen for incoming connections */
    if (listen(data->sock_fd, 1) < 0) {
        LOG_ERR("Failed to listen on socket");
        return -EAGAIN;
    }

    return 0;
}

/**
 * @brief Enable the IPM device in slave role
 *
 * @param dev Device structure
 * @return 0 if successful, negative error code otherwise
 */
static int ipm_net_emul_enable_slave(const struct device *dev)
{
    struct ipm_net_emul_data *data = (struct ipm_net_emul_data *)dev->data;
    const struct ipm_net_emul_config *conf = (const struct ipm_net_emul_config *)dev->config;

    return 0;
}

extern void ipm_net_emul_handle_connections(void *pdata, void *pcfg, void *punused)
{
    struct ipm_net_emul_data *data = (struct ipm_net_emul_data *)pdata;
    const struct ipm_net_emul_config *conf = (const struct ipm_net_emul_config *)pcfg;

    /* Resolve hostname to IP address */
    struct hostent *he = gethostbyname(conf->ip);
    if (he == NULL) {
        /* conf->ip is not a hostname, so assume its an IP address */
    }

    while (true);
}

static int ipm_net_emul_init(const struct device *dev)
{
    struct ipm_net_emul_data *data = (struct ipm_net_emul_data *)dev->data;
    const struct ipm_net_emul_config *conf = (const struct ipm_net_emul_config *)dev->config;

    /* Register socket on the host */
    if (ipm_net_emul_init_socket(data) != 0) {
        return -EAGAIN;
    }

    /* Set socket options in master role */
    if (conf->is_master) {
        data->address.sin_family = AF_INET;
        data->address.sin_addr.s_addr = INADDR_ANY;
        data->address.sin_port = htons(conf->port);

        if (bind(data->sock_fd, (struct sockaddr *)&data->address, sizeof(data->address)) < 0) {
            LOG_ERR("Failed to bind socket in master role");
            printk("bind failed\n");
            return -EAGAIN;
        }
    }

    LOG_DBG("Successfully initialized");
    return 0;
}

static int ipm_net_emul_send(const struct device *dev, int wait, uint32_t id,
    const void *data, int size)
{
    ARG_UNUSED(wait);
    ARG_UNUSED(data);
    ARG_UNUSED(size);

    const struct ipm_net_emul_config *dev_cfg = (const struct ipm_net_emul_config *)dev->config;

    LOG_DBG("sending notification to the peer id 0x%x\n", id);
    return -1;
}

static void ipm_net_emul_register_callback(const struct device *dev,
    ipm_callback_t cb, void *user_data)
{
    struct ipm_net_emul_data *data = (struct ipm_net_emul_data *)dev->data;
    LOG_DBG("register callback in ipm net emul");
}

static int ipm_net_emul_set_enabled(const struct device *dev, int enable)
{
    struct ipm_net_emul_data *data = (struct ipm_net_emul_data *)dev->data;
    const struct ipm_net_emul_config *conf = (const struct ipm_net_emul_config *)dev->config;

    /* If the socket file descriptor is invalid, there is nothing to do */
    if (data->sock_fd < 0) {
        return -EINVAL;
    }

    // TODO: Create msg queue and mutex in the device data, then use to send commands to the thread

    /* Enable in specified role master */
    if ((enable == true) && (conf->is_master == true)) {
        ipm_net_emul_enable_master(dev);
    } else if ((enable == true) && (conf->is_master == false)) {
        ipm_net_emul_enable_slave(dev);
    } else {
        /* Disable by closing the socket */
        shutdown(data->sock_fd, SHUT_RDWR);
        close(data->sock_fd);
        data->sock_fd = -1;
        LOG_DBG("Disabling IPM device");
    }

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
        .port = DT_INST_PROP(inst, port), \
        .is_master = DT_INST_PROP(inst, is_master) \
    }; \
    static struct ipm_net_emul_data ipm_net_emul_data_##inst = { \
        .sock_fd = -1 \
    }; \
    K_THREAD_DEFINE(ipm_net_emul_thread_##inst, \
        CONFIG_IPM_NET_EMUL_STACK_SIZE, \
        ipm_net_emul_handle_connections, \
        &ipm_net_emul_data_##inst, &ipm_net_emul_cfg_##inst, NULL, \
        K_PRIO_PREEMPT(CONFIG_IPM_NET_EMUL_THREAD_PRIO), 0, 0); \
    DEVICE_DT_INST_DEFINE(inst, \
        ipm_net_emul_init, \
        NULL, \
        &ipm_net_emul_data_##inst, &ipm_net_emul_cfg_##inst, \
        POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, \
        &ipm_net_emul_driver_api); \

DT_INST_FOREACH_STATUS_OKAY(IPM_NET_EMUL_INIT);
