#include "accel.h"

const struct device *accel = DEVICE_DT_GET(DT_ALIAS(accel));

K_MSGQ_DEFINE(accel_msgq, sizeof(struct accel_data), 1, 4);

int accel_init(void)
{
    if (!device_is_ready(accel))
    {
        printf("Acceleration Sensor not Ready\n");
        return -ENODEV;
    }
    return 0;
}

void accel_get(struct accel_data *data)
{
    k_msgq_get(&accel_msgq, data, K_FOREVER);
}

void accel_thread(void)
{
    int ret = accel_init();
    if (ret) {
        printf("Failed to Initialize Accel (err %d)\n", ret);
        return;
    }
    struct accel_data data;
    while (1)
    {
        ret = sensor_sample_fetch(accel);
        if (ret)
        {
            printk("Accel Error 1 - %d\n", ret);
            continue;
        }

        struct sensor_value val;

        ret |= sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &val);
        data.x = sensor_value_to_float(&val);

        ret |= sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &val);
        data.y = sensor_value_to_float(&val);

        ret |= sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &val);
        data.z = sensor_value_to_float(&val);

        if (ret)
        {
            printk("Error 2 - %d\n", ret);
            continue;
        }

        while (k_msgq_put(&accel_msgq, &data, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&accel_msgq);
        }
    }
}

K_THREAD_DEFINE(accel_id, STACKSIZE, accel_thread, NULL, NULL, NULL, PRIORITY-1, 0, 0);