#include "common.h"

#include "drivers.hpp"

namespace {

class driver : public Runnable, public MspDriver {

	static const int HX711_GAIN_A_128	= 1; // 25 pulses
	static const int HX711_GAIN_B_32	= 2; // 26 pulses
	static const int HX711_GAIN_A_64	= 3; // 27 pulses

	int _gain, _bias = 0, _lastval = 0;

	uint32_t read(void) {
		uint32_t v = 0;

		__disable_irq();

		for (int i = 0; i < 24; i++) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
			v = (v << 1) | HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_10);
		}

		for (int i = 0; i < _gain; i++) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		}

		__enable_irq();

		return ((v & 0x00800000) ? (v | 0xff000000) : v);
	}

	long do_init(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOC_CLK_ENABLE(); // PC10, PC11

		gpio.Pin = GPIO_PIN_10; // input
		gpio.Mode = GPIO_MODE_INPUT;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(GPIOC, &gpio);

		gpio.Pin = GPIO_PIN_11; // output (clk)
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(GPIOC, &gpio);

		// reset
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);

		return HAL_OK;
	}

public:
	driver() : _gain(HX711_GAIN_A_128) {}

	long init(void) {
		return do_init();
	}

	long mspInit(Handle *) {
		return HAL_OK;
	}

	void tare(void) {
		_bias = _lastval;
	}

	void run(uint32_t millis) {
		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_10) == GPIO_PIN_RESET) {
			_lastval = read();
			telemetry_set(Load, _lastval - _bias);
		}
		Runnable::arm();
	}
};

} // namespace

Driver *hx711() {
	static driver drv;
	return &drv; // singleton
}
