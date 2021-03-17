#include "driver/gpio.h"
#include "device_control.h"
#include <stdio.h>


void iot_gpio_init(void)
{
    /*
     * GPIO Initializer
     * */
    gpio_config_t io_conf;

    // Main Button
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1 << GPIO_INPUT_BUTTON;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_set_intr_type(GPIO_INPUT_BUTTON, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
}