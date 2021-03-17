#include <stdio.h>
/*#include <stdbool.h>*/
/*#include <stdlib.h>*/
/*#include <string.h>*/

#include "driver/gpio.h"
#include "device_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "st_dev.h"

#include "caps_button.h"

// onboarding_config_start is null-terminated string
extern const uint8_t onboarding_config_start[] asm("_binary_onboarding_config_json_start");
extern const uint8_t onboarding_config_end[] asm("_binary_onboarding_config_json_end");
// device_info_start is null-terminated string
extern const uint8_t device_info_start[]    asm("_binary_device_info_json_start");
extern const uint8_t device_info_end[]        asm("_binary_device_info_json_end");

static iot_status_t g_iot_status = IOT_STATUS_IDLE;
static iot_stat_lv_t g_iot_stat_lv;

IOT_CTX* ctx = NULL;
/*
 * Capability Initialization
 * and futher handling.
 * */
static caps_button_data_t *cap_button_data;

static void capability_init()
{
    cap_button_data = caps_button_initialize(ctx, "main", NULL, NULL);
    cap_button_data->set_numberOfButtons_value(cap_button_data, 1);
    cap_button_data->set_button_value(
            cap_button_data,
            caps_helper_button.attr_button.value_pushed);
    cap_button_data->attr_button_send(cap_button_data);
}

static void iot_status_cb(iot_status_t status,
                          iot_stat_lv_t stat_lv, void *usr_data)
{
    g_iot_status = status;
    g_iot_stat_lv = stat_lv;

    printf("status: %d, stat: %d\n", g_iot_status, g_iot_stat_lv);
}

static void button_event_task(void)
{
    static uint32_t button_count = 0;
    static uint32_t button_last_state = RELEASED;
    // Timeout pointers
    static TimeOut_t button_timeout;
    static TickType_t button_delay;

    while (1) {
        uint32_t gpio_level;
        gpio_level = gpio_get_level(GPIO_INPUT_BUTTON);
        if (button_last_state != gpio_level) {
            // debounce button event
            vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME));
            gpio_level = gpio_get_level(GPIO_INPUT_BUTTON);
            if (button_last_state != gpio_level) {
                button_last_state = gpio_level;
                if (gpio_level == RELEASED) {
                    button_count++;
                    printf("button event, val: %d, button count: %d, tick: %u\n",
                        gpio_level,
                        button_count,
                        (uint32_t)xTaskGetTickCount());
                }
            /*
             * Initialize TimeOut after every button event
             * */
                vTaskSetTimeOutState(&button_timeout);
                button_delay = pdMS_TO_TICKS(BUTTON_DELAY_MS);
            }
        } else if (button_count > 0) {
            /*
             * Check for TimeOut
             * */
            if (xTaskCheckForTimeOut(&button_timeout, &button_delay) != pdFALSE) {
                switch (button_count) {
                    case 1:
                        cap_button_data->set_button_value(
                                cap_button_data,
                                caps_helper_button.attr_button.value_pushed);
                        break;
                    case 2:
                        cap_button_data->set_button_value(
                                cap_button_data,
                                caps_helper_button.attr_button.value_double);
                        break;
                    default:
                        cap_button_data->set_button_value(
                                cap_button_data,
                                caps_helper_button.attr_button.value_pushed_3x);
                        break;
                }
                cap_button_data->attr_button_send(cap_button_data);
                /*
                 * Clear TimeOut
                 * and button counter
                 * */
                button_count = 0;
                vTaskSetTimeOutState(&button_timeout);
            }
        }
    }
}

static void connection_start(void)
{
    iot_pin_t *pin_num = NULL;
    int err;

#if defined(SET_PIN_NUMBER_CONFRIM)
    pin_num = (iot_pin_t *) malloc(sizeof(iot_pin_t));
    if (!pin_num)
        printf("failed to malloc for iot_pin_t\n");

    //[>to decide the pin confirmation number(ex. "12345678"). It will use for easysetup.<]
    // pin confirmation number must be 8 digit number.
    pin_num_memcpy(pin_num, "12345678", sizeof(iot_pin_t));
#endif

     //process on-boarding procedure. There is nothing more to do on the app side than call the API.
    err = st_conn_start(ctx, (st_status_cb)&iot_status_cb, IOT_STATUS_ALL, NULL, pin_num);
    if (err) {
        printf("fail to start connection. err:%d\n", err);
    }
    if (pin_num) {
        free(pin_num);
    }
}



static void iot_noti_cb(iot_noti_data_t *noti_data, void *noti_usr_data)
{
    printf("Notification message received\n");

    if (noti_data->type == IOT_NOTI_TYPE_DEV_DELETED) {
        printf("[device deleted]\n");
    } else if (noti_data->type == IOT_NOTI_TYPE_RATE_LIMIT) {
        printf("[rate limit] Remaining time:%d, sequence number:%d\n",
               noti_data->raw.rate_limit.remainingTime, noti_data->raw.rate_limit.sequenceNumber);
    }
}

void app_main(void)
{
    unsigned char *onboarding_config = (unsigned char *) onboarding_config_start;
    unsigned int onboarding_config_len = onboarding_config_end - onboarding_config_start;
    unsigned char *device_info = (unsigned char *) device_info_start;
    unsigned int device_info_len = device_info_end - device_info_start;

    int iot_err;

    /*
     * Initialize context
     * */
    ctx = st_conn_init(
        onboarding_config,
        onboarding_config_len,
        device_info,
        device_info_len);

    if (ctx != NULL) {
        iot_err = st_conn_set_noti_cb(ctx, iot_noti_cb, NULL);
        if (iot_err)
            printf("fail to set notification callback function\n");
    } else {
        printf("fail to create the iot_context\n");
    }

    capability_init();
    iot_gpio_init();
    /*
     * Register Tasks
     * */
    xTaskCreate(button_event_task, "button_event_task", 4096, NULL, 0, NULL);

    // connect to server
    connection_start();
}
