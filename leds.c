
/***************************** Include files *******************************/
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "Task.h"
#include "queue.h"
#include "semphr.h"
#include "emp_type.h"
#include "adc.h"
#include "ATM.h"
#include "lcd.h"
// #include "glob_def.h"
// #include "status_led.h"

/*****************************    Defines    *******************************/
extern SemaphoreHandle_t xSemaphore_value;
extern QueueHandle_t xQueue_step_value;
extern INT16U WithdrawAmount;
extern enum ATM_states ATM_state;
INT8U frequency;

/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

void red_led_init(void)
/*****************************************************************************
 *   Input    :  -
 *   Output   :  -
 *   Function :
 *****************************************************************************/
{
    INT8S dummy;
    // Enable the GPIO port that is used for the on-board LED.
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;

    // Do a dummy read to insert a few cycles after enabling the peripheral.
    dummy = SYSCTL_RCGC2_R;

    GPIO_PORTF_DIR_R |= 0x02;
    GPIO_PORTF_DEN_R |= 0x02;
    GPIO_PORTF_DATA_R ^= 0x02;
}

void yellow_led_init(void)
/*****************************************************************************
 *   Input    :  -
 *   Output   :  -
 *   Function :
 *****************************************************************************/
{
    INT8S dummy;
    // Enable the GPIO port that is used for the on-board LED.
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;

    // Do a dummy read to insert a few cycles after enabling the peripheral.
    dummy = SYSCTL_RCGC2_R;

    GPIO_PORTF_DIR_R |= 0x04;
    GPIO_PORTF_DEN_R |= 0x04;
    GPIO_PORTF_DATA_R ^= 0x04;
}

void green_led_init(void)
/*****************************************************************************
 *   Input    :  -
 *   Output   :  -
 *   Function :
 *****************************************************************************/
{
    INT8S dummy;
    // Enable the GPIO port that is used for the on-board LED.
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;

    // Do a dummy read to insert a few cycles after enabling the peripheral.
    dummy = SYSCTL_RCGC2_R;

    GPIO_PORTF_DIR_R |= 0x08;
    GPIO_PORTF_DEN_R |= 0x08;
    GPIO_PORTF_DATA_R ^= 0x08;
}

void red_led_toggle()
{
    INT16U adc_value;
    portTickType delay;
    int period_ms;
    int blinks;

    blinks = WithdrawAmount * 2 / 100;

    int i = 0;
    while (i < blinks)
    {
        GPIO_PORTF_DATA_R ^= 0x02;
        adc_value = get_adc();
        frequency = adc_value * 0.0022 + 1.0;
        period_ms = 1000 / frequency;

        delay = period_ms / 2;
        vTaskDelay(delay / portTICK_RATE_MS);
        i++;
    }
}

void green_led_toggle()
{
    INT16U adc_value;
    portTickType delay;
    int blinks;
    int period_ms;
    blinks = WithdrawAmount * 2 / 10;

    int i = 0;

    while (i < blinks)
    {
        GPIO_PORTF_DATA_R ^= 0x08;
        adc_value = get_adc();
        frequency = adc_value * 0.0022 + 1.0;
        period_ms = 1000 / frequency;

        delay = period_ms / 2;
        vTaskDelay(delay / portTICK_RATE_MS);
        i++;
    }
}

void yellow_led_toggle()
{
    INT16U adc_value;
    portTickType delay;
    int blinks;
    int period_ms;
    blinks = WithdrawAmount * 2 / 50;

    int i = 0;
    while (i < blinks)
    {
        GPIO_PORTF_DATA_R ^= 0x04;
        adc_value = get_adc();
        frequency = adc_value * 0.0022 + 1.0;
        period_ms = 1000 / frequency;

        delay = period_ms / 2;
        vTaskDelay(delay / portTICK_RATE_MS);
        i++;
    }
}

void led_task(void *pvParameters)
{

    int value;

    while (1)
    {
        if (uxQueueMessagesWaiting(xQueue_step_value))
        {

            if (xSemaphoreTake(xSemaphore_value, portMAX_DELAY))
            {

                if (xQueueReceive(xQueue_step_value, &value, portMAX_DELAY))
                {

                    xSemaphoreGive(xSemaphore_value);

                    if (value == 100 && WithdrawAmount >= 0x64)
                    {
                        red_led_toggle();
                    }

                    if (value == 50 && WithdrawAmount >= 0x32)
                    {
                        yellow_led_toggle();
                    }

                    if (value == 10 && WithdrawAmount >= 0x0A)
                    {
                        green_led_toggle();
                    }
                    ATM_state = PIN_CODE;
                    vTaskDelay(10);
                }
            }
        }
    }
}

/****************************** End Of Module *******************************/
