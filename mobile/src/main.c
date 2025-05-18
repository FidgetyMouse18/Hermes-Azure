#include "../../mylib/advertise.h"
// #include "../../mylib/sensors/accel.h"
// #include "../../mylib/sensors/humid.h"
#include "../../mylib/sensors/light.h"
// #include "../../mylib/sensors/press.h"
// #include "../../mylib/sensors/temp.h"
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	// accel_init();

	// light_init();

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
	struct light_data data2;
	// struct accel_data data3;
	while (1)
	{
		printf("Fetching Data\n");
		// humid_get(&data);
		// printf("Humidity: %.2f\n", data);
		// press_get(&data);
		// printf("Pressure: %.2f\n", data);
		// temp_get(&data);
		// printf("Temperature: %.2f\n\n", data);

		light_get(&data2);
		printf("R: %d    G: %d    B: %d    W: %d\n", data2.r, data2.g, data2.b, data2.w);

		// accel_get(&data3);
		// printf("X: %.2f    Y: %.2f    Z: %.2f\n", data3.x, data3.y, data3.z);
		k_msleep(2000);
	}
}

K_THREAD_DEFINE(run_id, STACKSIZE, run, NULL, NULL, NULL,
				PRIORITY - 1, 0, 0);