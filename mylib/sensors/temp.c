#include "temp.h"

const struct device *temp = DEVICE_DT_GET(DT_ALIAS(temp));

K_MSGQ_DEFINE(temp_msgq, sizeof(float), 1, 4);

int temp_init(void)
{
    if (!device_is_ready(temp))
    {
        printf("Temp Sensor not Ready\n");
        return -ENODEV;
    }
    return 0;
}

void temp_get(float *data)
{
    k_msgq_get(&temp_msgq, data, K_FOREVER);
}

void temp_thread(void)
{
    int ret = temp_init();
    if (ret) {
        printf("Failed to Initialize Temp (err %d)\n", ret);
        return;
    }
    float data;
    while (1)
    {
        ret = sensor_sample_fetch(temp);
        if (ret)
        {
            printk("Temp Error 1 - %d\n", ret);
            continue;
        }

        struct sensor_value val;

        ret = sensor_channel_get(temp, SENSOR_CHAN_AMBIENT_TEMP, &val);
        data = sensor_value_to_float(&val);

        if (ret)
        {
            printk("Error 2 - %d\n", ret);
            continue;
        }

        while (k_msgq_put(&temp_msgq, &data, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&temp_msgq);
        }
    }
}

K_THREAD_DEFINE(temp_id, STACKSIZE, temp_thread, NULL, NULL, NULL, PRIORITY-1, 0, 0);