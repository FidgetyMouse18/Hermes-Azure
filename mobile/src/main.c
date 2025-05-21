#include "../../mylib/advertise.h"
#include "../../mylib/sensors/accel.h"
#include "../../mylib/sensors/humid.h"
#include "../../mylib/sensors/light.h"
#include "../../mylib/sensors/press.h"
#include "../../mylib/sensors/temp.h"
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{

	if (!gpio_is_ready_dt(&led))
	{
		return 0;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set_dt(&led, 1);

	return 0;
}

void run(void)
{
	float data;
	uint8_t pressure, humidity, temperature, r, g, b, accel_x, accel_y, accel_z;
	uint16_t tvoc;
	struct light_data light_data;
	struct accel_data accel_data;
	while (1)
	{
		// printf("Fetching Data\n");
		humid_get(&data);
		humidity = (uint8_t)data;
		press_get(&data);
		pressure = (uint8_t)data;
		temp_get(&data);
		temperature = (uint8_t)data;
		accel_get(&accel_data);
		accel_x = (int8_t)accel_data.x;
		accel_y = (int8_t)accel_data.y;
		accel_z = (int8_t)accel_data.z;

		//TEMP
		r = 127;
		g = 12;
		b = 94;
		tvoc = 1143;

		// printf("Humidity: %d\n", humidity);
		// printf("Pressure: %d\n", pressure);
		// printf("Temperature: %d\n\n", temperature);
		// printf("X: %d    Y: %d    Z: %d\n", accel_x, accel_y, accel_z);

		// light_get(&light_data);
		// printf("R: %d    G: %d    B: %d    W: %d\n", light_data.r, light_data.g, light_data.b, light_data.w);
		// queue_data(pressure, humidity, temperature, r, g, b, tvoc, accel_x, accel_y, accel_z);
		k_msleep(150);
	}
}

K_THREAD_DEFINE(run_id, STACKSIZE, run, NULL, NULL, NULL,
				PRIORITY - 1, 0, 0);