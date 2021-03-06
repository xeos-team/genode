/*
 * \brief  Test GPIO driver with Leds
 * \author Reinier Millo Sánchez <rmillo@uclv.cu>
 * \date   2015-07-26
 */

/*
 * Copyright (C) 2012-2015 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/printf.h>
#include <base/signal.h>
#include <gpio_session/connection.h>
#include <irq_session/client.h>
#include <util/mmio.h>
#include <timer_session/connection.h>

#include <os/config.h>

int main(int, char **)
{
	unsigned int _delay = 1000;
	unsigned int _gpio_pin = 16;
	unsigned int _times = 10;

	try {
		Genode::config()->xml_node().attribute("gpio_pin").value(&_gpio_pin);
	} catch (...) { }

	try {
		Genode::config()->xml_node().attribute("delay").value(&_delay);
	} catch (...) { }

	try {
		Genode::config()->xml_node().attribute("times").value(&_times);
	} catch (...) { }

	PDBG("--- GPIO Led test [GPIO Pin: %d, Timer delay: %d, Times: %d] ---",_gpio_pin, _delay, _times);

	Gpio::Connection _led(_gpio_pin);
	Timer::Connection _timer;

	while(_times--)
	{
		PDBG("Remains blinks: %d",_times);
		_led.write(false);
		_timer.msleep(_delay);
		_led.write(true);
		_timer.msleep(_delay);
	}

	Genode::printf("Test finished\n");
	return 0;
}
