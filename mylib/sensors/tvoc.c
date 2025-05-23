#include "tvoc.h"

const struct device *tvoc = DEVICE_DT_GET(DT_ALIAS(tvoc));

K_MSGQ_DEFINE(tvoc_msgq, sizeof(float), 1, 4);

int tvoc_init(void)
{
    if (!device_is_ready(tvoc))
    {
        printf("tvocure Sensor not Ready\n");
        return -ENODEV;
    }
    return 0;
}

void tvoc_get(float *data)
{
    k_msgq_get(&tvoc_msgq, data, K_FOREVER);
}

void tvoc_thread(void)
{
    int ret = tvoc_init();
    if (ret) {
        printf("Failed to Initialize tvoc (err %d)\n", ret);
        return;
    }
    float data;
    while (1)
    {
        ret = sensor_sample_fetch(tvoc);
        if (ret)
        {
            printk("TVOC Error 1 - %d\n", ret);
            continue;
        }

        struct sensor_value val;

        ret = sensor_channel_get(tvoc, SENSOR_CHAN_VOC, &val);
        data = sensor_value_to_float(&val);

        if (ret)
        {
            printk("Error 2 - %d\n", ret);
            continue;
        }

        while (k_msgq_put(&tvoc_msgq, &data, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&tvoc_msgq);
        }
    }
}

K_THREAD_DEFINE(tvoc_id, STACKSIZE, tvoc_thread, NULL, NULL, NULL, PRIORITY-1, 0, 0);