#include "../../mylib/advertise.h"
#include "../../mylib/sensors/accel.h"
#include "../../mylib/sensors/humid.h"
#include "../../mylib/sensors/light.h"
#include "../../mylib/sensors/press.h"
#include "../../mylib/sensors/temp.h"
#include "../../mylib/sensors/tvoc.h"
#include "../../mylib/sensors/mic.h"
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

int main(void)
{
	if (!gpio_is_ready_dt(&led1) || !gpio_is_ready_dt(&led2) || !gpio_is_ready_dt(&led3))
	{
		return 0;
	}

	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	// gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
	// gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE);

	gpio_pin_set_dt(&led1, 1);
	// gpio_pin_set_dt(&led2, 1);
	// gpio_pin_set_dt(&led3, 1);

	return 0;
}

void run(void)
{
	float data;
	uint8_t pressure, humidity, temperature, accel_x, accel_y, accel_z;
	uint16_t tvoc;
	struct light_data light_data;
	struct accel_data accel_data;

	mic_init();
	while (1)
	{
		// printf("Fetching Data\n");
		// humid_get(&data);
		// humidity = (uint8_t)data;
		// press_get(&data);
		// pressure = (uint8_t)data;
		// temp_get(&data);
		// temperature = (uint8_t)data;
		// tvoc_get(&data);
		// tvoc = (uint16_t)data;
		// accel_get(&accel_data);
		// accel_x = (int8_t)accel_data.x;
		// accel_y = (int8_t)accel_data.y;
		// accel_z = (int8_t)accel_data.z;
		// light_get(&light_data);
		mic_read(&data);

		// printf("Sound: %0.2fdB\n", data);

		// printf("Humidity: %d\n", humidity);
		// printf("Pressure: %d\n", pressure);
		// printf("Temperature: %d\n\n", temperature);
		// printf("X: %d    Y: %d    Z: %d\n", accel_x, accel_y, accel_z);
		// printf("R: %d    G: %d    B: %d    W: %d\n", light_data.r, light_data.g, light_data.b, light_data.w);

		// queue_data(pressure, humidity, temperature, light_data.r, light_data.g, light_data.b, tvoc, accel_x, accel_y, accel_z);
		k_msleep(10);
		// k_msleep(5000);
	}
}

K_THREAD_DEFINE(run_id, STACKSIZE, run, NULL, NULL, NULL,
				PRIORITY - 1, 0, 0);