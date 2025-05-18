#include "press.h"

const struct device *press = DEVICE_DT_GET(DT_ALIAS(press));

K_MSGQ_DEFINE(press_msgq, sizeof(float), 1, 4);

int press_init(void)
{
    if (!device_is_ready(press))
    {
        printf("Pressure Sensor not Ready\n");
        return -ENODEV;
    }
    return 0;
}

void press_get(float *data)
{
    k_msgq_get(&press_msgq, data, K_FOREVER);
}

void press_thread(void)
{
    int ret = press_init();
    if (ret) {
        printf("Failed to Initialize Press (err %d)\n", ret);
        return;
    }
    float data;
    while (1)
    {
        ret = sensor_sample_fetch(press);
        if (ret)
        {
            printk("Error 1 - %d\n", ret);
            continue;
        }

        struct sensor_value val;

        ret = sensor_channel_get(press, SENSOR_CHAN_PRESS, &val);
        data = sensor_value_to_float(&val);

        if (ret)
        {
            printk("Error 2 - %d\n", ret);
            continue;
        }

        while (k_msgq_put(&press_msgq, &data, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&press_msgq);
        }
    }
}

K_THREAD_DEFINE(press_id, STACKSIZE, press_thread, NULL, NULL, NULL,
                PRIORITY-1, 0, 0);