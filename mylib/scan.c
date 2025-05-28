#include "scan.h"

static struct bt_le_scan_param scan_params = {
    .type = BT_LE_SCAN_TYPE_PASSIVE,
    .options = BT_LE_SCAN_OPT_NONE | BT_LE_SCAN_OPT_FILTER_DUPLICATE,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window = BT_GAP_SCAN_FAST_WINDOW,
};

void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type, struct net_buf_simple *buf)
{
    // char uuid[32];

    // if (adv_type != BT_GAP_ADV_TYPE_ADV_IND)
    // {
    //     return;
    // }
    // printk("Size: %d\n", buf->len);
    // k_msleep(1000);
    // return;
    char json_buf[2048];
    char uuid_buf[32];
    char timestamp_buf[16];
    char pressure_buf[16];
    char humidity_buf[16];
    char temperature_buf[16];
    char r_buf[16];
    char g_buf[16];
    char b_buf[16];
    char tvoc_buf[16];
    char x_buf[16];
    char y_buf[16];
    char z_buf[16];

    if (buf->len < sizeof(struct ble_adv))
    {
        return; // Skip incomplete packets
    }
    struct ble_adv ble;

    memcpy(&ble, net_buf_simple_pull_mem(buf, sizeof(ble)), sizeof(ble));

    // snprintk(uuid, sizeof(uuid), "%02X:%02X:%02X:%02X", ble.uuid[0], ble.uuid[1], ble.uuid[2], ble.uuid[3]);

    if (ble.prefix[0] != 0xA3 || ble.prefix[1] != 0xF9 || ble.prefix[2] != 0xC2 || ble.prefix[3] != 0xB7)
    {
        return;
    }

    int ret = check_node(ble.uuid, ble.timestamp);
    switch (ret)
    {
    case 0:
        return;
        break;
    case -2:
        node_remove(ble.uuid);
        node_add(ble.uuid, ble.timestamp);
        break;
    case -1:
        node_add(ble.uuid, ble.timestamp);
        break;

    default:
        break;
    }

    // printk("*** Data Received ***\n");
    // printk("UUID: %02X:%02X:%02X:%02X\n", ble.uuid[0], ble.uuid[1], ble.uuid[2], ble.uuid[3]);
    // printk("TimeStamp: %d\n", ble.timestamp);
    // printk("Humidity: %d\n", ble.humidity);
    // printk("Pressure: %d\n", ble.pressure);
    // printk("Temperature: %d\n", ble.temperature);
    // printk("TVOC: %d\n", ble.tvoc);
    // printk("X: %d    Y: %d    Z: %d\n", ble.accel_x, ble.accel_y, ble.accel_z);
    // printk("R: %d    G: %d    B: %d\n", ble.r, ble.g, ble.b);

    sprintf(uuid_buf, "%c%c%c%c", ble.uuid[0], ble.uuid[1], ble.uuid[2], ble.uuid[3]);
    sprintf(timestamp_buf, "%d", ble.timestamp);
    sprintf(pressure_buf, "%d", ble.pressure);
    sprintf(humidity_buf, "%d", ble.humidity);
    sprintf(temperature_buf, "%d", ble.temperature);
    sprintf(r_buf, "%d", ble.r);
    sprintf(g_buf, "%d", ble.g);
    sprintf(b_buf, "%d", ble.b);
    sprintf(tvoc_buf, "%d", ble.tvoc);
    sprintf(x_buf, "%d", ble.accel_x);
    sprintf(y_buf, "%d", ble.accel_y);
    sprintf(z_buf, "%d", ble.accel_z);

    struct sensor_data s_data = {.uuid = uuid_buf, .timestamp = timestamp_buf, .pressure = pressure_buf, .humidity = humidity_buf, .temperature = temperature_buf, .r = r_buf, .g = g_buf, .b = b_buf, .tvoc = tvoc_buf, .accel_x = x_buf, .accel_y = y_buf, .accel_z = z_buf};

    json_obj_encode_buf(sensor_descr, ARRAY_SIZE(sensor_descr), &s_data, json_buf, sizeof(json_buf));
    printk("%s\n", json_buf);
    ret = http_post(TARGET_IP, TARGET_PORT, "/reading", json_buf);
    if(ret) {
        printk("Error sending HTTP %d\n", ret);
    } else {
        printk("Data Sent Sucessfully\n");
    }
    // k_msleep(1000);
}

void scan_thread(void)
{
    // settings_subsys_init();

    int ret = bt_enable(NULL);
    if (ret)
    {
        printk("Bluetooth initialization failed (err %d)\n", ret);
        return;
    }
    printk("Bluetooth initialized successfully\n");
    k_msleep(50);
    // settings_load();
    // k_msleep(50);
    wifi_init();

    while (1)
    {
        bt_le_scan_start(&scan_params, scan_cb);
        k_msleep(SCAN_TIME);
    }
}

K_THREAD_DEFINE(scan_id, STACKSIZE, scan_thread, NULL, NULL, NULL,
                PRIORITY - 4, 0, 0);