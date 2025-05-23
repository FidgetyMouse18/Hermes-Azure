#include "advertise.h"

K_MSGQ_DEFINE(ble_msgq, sizeof(struct ble_adv), 2, 4);

static uint8_t ble_data[] = {

    0xA3, 0xF9, 0xC2, 0xB7,     // prefix
    UUID0, UUID1, UUID2, UUID3, // uuid
    0x00, 0x00,                 // timestamp
    0x00,                       // pressure
    0x00,                       // humidity
    0x00,                       // temperature
    0x00, 0x00, 0x00,           // rgb
    0x00, 0x00,                 // tvoc
    0x00, 0x00, 0x00            // acceleration
};

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, ble_data, sizeof(ble_data)),
};
// west build -b <board> <application_path> -- -D<VARIABLE_NAME>=<value>
// west build -b thingy52/nrf52832 mobile/ --pristine -- -DUUID0=0xDE -DUUID1=0xAD -DUUID2=0xBE -DUUID3=0xEF

void queue_data(uint16_t timestamp, uint8_t pressure, uint8_t humidity, uint8_t temeprature, uint8_t r, uint8_t g, uint8_t b, uint16_t tvoc, int8_t accel_x, int8_t accel_y, int8_t accel_z)
{
    struct ble_adv data = {
        .timestamp = timestamp,
        .pressure = pressure,
        .humidity = humidity,
        .temperature = temeprature,
        .r = r,
        .g = g,
        .b = b,
        .tvoc = tvoc,
        .accel_x = accel_x,
        .accel_y = accel_y,
        .accel_z = accel_z};
    while (k_msgq_put(&ble_msgq, &data, K_NO_WAIT) != 0)
    {
        k_msgq_purge(&ble_msgq);
    }
}

int ble_init(void)
{

    int ret = bt_enable(NULL);
    if (ret)
    {
        printf("Bluetooth initialization failed (err %d)\n", ret);
        return ret;
    }
    printf("Bluetooth initialized successfully\n");

    return 0;
}

void adv_thread(void)
{
    ble_init();
    printf("Advertise thread started\n");
    int err;
    struct ble_adv *current = k_malloc(sizeof(struct ble_adv));
    while (1)
    {
        if (k_msgq_get(&ble_msgq, current, K_NO_WAIT) == 0)
        {
            err = bt_le_adv_stop();
            if (err)
            {
                printf("Failed to stop Adv (err %d)\n", err);
            }
            ble_data[8] = current->timestamp & 0xFF;
            ble_data[9] = (current->timestamp >> 8) & 0xFF;
            ble_data[10] = current->pressure;
            ble_data[11] = current->humidity;
            ble_data[12] = current->temperature;
            ble_data[13] = current->r;
            ble_data[14] = current->g;
            ble_data[15] = current->b;
            ble_data[16] = current->tvoc & 0xFF;
            ble_data[17] = (current->tvoc >> 8) & 0xFF;
            ble_data[18] = current->accel_x;
            ble_data[19] = current->accel_y;
            ble_data[20] = current->accel_z;

            printf("*** Transmitting ***\n");
            printf("Humidity: %d\n", current->humidity);
            printf("Pressure: %d\n", current->pressure);
            printf("Temperature: %d\n\n", current->temperature);
            printf("X: %d    Y: %d    Z: %d\n", current->accel_x, current->accel_y, current->accel_z);
            err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
            if (err)
            {
                printf("Failed to start Adv (err %d)\n", err);
            }
            k_msleep(ADVERTISE_MS);
        }
    }
}

K_THREAD_DEFINE(adv_id, STACKSIZE, adv_thread, NULL, NULL, NULL, PRIORITY, 0, 0);