/*
 * GPIO definitions
 * */
#define GPIO_INPUT_BUTTON 4
#define BUTTON_SINGLE_PRESS 1
#define BUTTON_DOUBLE_PRESS 2
#define BUTTON_TRIPLE_PRESS 3
#define BUTTON_DEBOUNCE_TIME 20
#define BUTTON_DELAY_MS 700

enum button_state {
    RELEASED = 1,
    PRESSED = 0
};


void iot_gpio_init(void);
