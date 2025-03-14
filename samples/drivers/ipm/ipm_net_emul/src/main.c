/*
 * Copyright (c) 2015 Lukas Ostermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>

#define DATA_SIZE 4

static void ipm_callback(const struct device *dev, void *context,
    uint32_t id, volatile void *data)
{
    int i;
    int status;
    uint32_t *data32 = (uint32_t *)data;

    printk("%s: id = %u, data = 0x", __func__, id);
    for (i = 0; i < (DATA_SIZE / 4); i++) {
        printk("%08x", data32[i]);
    }
    printk("\n");

    status = ipm_send(dev, 1, id, (const void *)data, DATA_SIZE);

    if (status) {
        printk("ipm_send() failed: %d\n", status);
    }
}

int main(void)
{
    const struct device *ipm = DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));

    if (device_is_ready(ipm) == false) {
        printk("Error: IPM device not ready\n");
        return -1;
    }

    ipm_register_callback(ipm, ipm_callback, NULL);
    ipm_set_enabled(ipm, 1);

    printk("IPM initialized, data size = %d\n", DATA_SIZE);

    while (true);
    return 0;
}
