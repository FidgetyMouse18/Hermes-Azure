#include "light.h"

const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

K_MSGQ_DEFINE(light_msgq, sizeof(struct light_data), 1, 4);

static int bh1745_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_write(i2c_dev, buf, 2, BH1745_I2C_ADDR);
}

static int bh1745_read_reg16(uint8_t reg, uint16_t *val)
{
    uint8_t buf[2];
    int ret = i2c_burst_read(i2c_dev, BH1745_I2C_ADDR, reg, buf, 2);
    if (ret < 0)
        return ret;
    *val = (buf[1] << 8) | buf[0];
    return 0;
}

int light_init(void)
{
    if (!device_is_ready(i2c_dev))
    {
        printf("I2C not ready (Light Sensor)\n");
        return -1;
    }

    k_msleep(10);

    bh1745_write_reg(REG_SYS_CTRL, 0x80);
    k_msleep(2);
    bh1745_write_reg(REG_SYS_CTRL, 0x00);

    uint8_t cfg[] = {REG_MODECTL1, 0x00, /* 160 ms IT  */
                     0x10,               /* gain 1× + RGBC_EN */
                     0x02};              /* START conversions  */
    i2c_write(i2c_dev, cfg, sizeof(cfg), BH1745_I2C_ADDR);

    k_msleep(200);

    return 0;
}

void light_get(struct light_data *data)
{
    k_msgq_get(&light_msgq, data, K_FOREVER);
}

void light_thread(void)
{
    int ret = light_init();
    if (ret)
    {
        printf("Failed to Initialize Light (err %d)\n", ret);
        return;
    }
    struct light_data data;
    while (1)
    {
        ret |= bh1745_read_reg16(BH1745_REG_RED, &data.r);
        ret |= bh1745_read_reg16(BH1745_REG_GREEN, &data.g);
        ret |= bh1745_read_reg16(BH1745_REG_BLUE, &data.b);
        ret |= bh1745_read_reg16(BH1745_REG_WHITE, &data.w);

        if (ret)
        {
            printk("Error 2 - %d\n", ret);
            continue;
        }

        //convert to lux
        data.r = (uint8_t)(data.r * 0.4f);
        data.g = (uint8_t)(data.g * 0.4f);
        data.b = (uint8_t)(data.b * 0.4f);
        data.w = (uint8_t)(data.w * 0.4f);

        while (k_msgq_put(&light_msgq, &data, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&light_msgq);
        }
    }
}

K_THREAD_DEFINE(light_id, STACKSIZE, light_thread, NULL, NULL, NULL,
                PRIORITY - 1, 0, 0);