#include "humid.h"

const struct device *humid = DEVICE_DT_GET(DT_ALIAS(humid));

K_MSGQ_DEFINE(humid_msgq, sizeof(float), 1, 4);

int humid_init(void)
{
    if (!device_is_ready(humid))
    {
        printf("Humidity Sensor not Ready\n");
        return -ENODEV;
    }
    return 0;
}

void humid_get(float *data)
{
    k_msgq_get(&humid_msgq, data, K_FOREVER);
}

void humid_thread(void)
{
    int ret = humid_init();
    if (ret) {
        printf("Failed to Initialize Humid (err %d)\n", ret);
        return;
    }
    float data;
    while (1)
    {
        ret = sensor_sample_fetch(humid);
        if (ret)
        {
            printk("Error 1 - %d\n", ret);
            continue;
        }

        struct sensor_value val;

        ret = sensor_channel_get(humid, SENSOR_CHAN_HUMIDITY, &val);
        data = sensor_value_to_float(&val);

        if (ret)
        {
            printk("Error 2 - %d\n", ret);
            continue;
        }

        while (k_msgq_put(&humid_msgq, &data, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&humid_msgq);
        }
    }
}

K_THREAD_DEFINE(humid_id, STACKSIZE, humid_thread, NULL, NULL, NULL,
                PRIORITY-1, 0, 0);