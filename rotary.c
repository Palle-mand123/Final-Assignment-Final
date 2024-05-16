/***************************** Include files *******************************/
#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "tm4c123gh6pm.h"
#include "emp_type.h"
#include "lcd.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "tmodel.h"
#include "adc.h"
#include "emp_type.h"
#include "glob_def.h"
#include "ATM.h"
/*****************************    Defines    *******************************/
extern QueueHandle_t xQueue_step_value;
extern SemaphoreHandle_t xSemaphore_value;
extern enum ATM_states ATM_state;
extern INT16U WithdrawAmount;
/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/
INT16U step_values[] = {100, 50, 10}; // Array of step values
INT16U step_index = 0;                // Index to track current step value
INT16U current_step_value = 100;
/*****************************   Functions   *******************************/



void init_rotary(void)
{
    volatile int8_t dummy;
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // Enable clock for Port A
    dummy = SYSCTL_RCGC2_R;               // Read back to introduce a delay

    // Set PA5-PA7 as input pins
    GPIO_PORTA_DIR_R &= ~0xE0;

    // Enable digital functionality for PA5-PA7
    GPIO_PORTA_DEN_R |= 0xE0;

    // Disable analog functionality for PA5-PA7
    GPIO_PORTA_AMSEL_R &= ~0xE0;
}

void rotary_task(void *pvParameters)
{
    init_rotary();
    const TickType_t xDelay = 1 / portTICK_PERIOD_MS;
    static int prevA = 0;
    static int prevB = 0;
    static int A;
    static int B;
    static int C;
    home_LCD();
    vTaskDelay(10);

    while (1)
    {
        vTaskDelay(xDelay / 1000);

        if ((0b10000000) & (GPIO_PORTA_DATA_R))
        {
            C = 1;
        }
        else
        {
            C = 0;
        }

        if ((0b00100000) & (GPIO_PORTA_DATA_R))
        {
            A = 1;
        }
        else
        {
            A = 0;
        }


        if ((0b01000000) & (GPIO_PORTA_DATA_R))
        {
            B = 1;
        }
        else
        {
            B = 0;
        }

        int AB = (A << 1) | B;
        int prevAB = (prevA << 1) | prevB;
        INT8U YY = AB ^ prevAB;

        if (AB == prevAB)
        {
        }
        else if (A == B)
        {
            if (YY == 0x01)
            {

                step_index = (step_index - 1 + sizeof(step_values) / sizeof(step_values[0])) % (sizeof(step_values) / sizeof(step_values[0]));
                current_step_value = step_values[step_index];
            }
            if (YY == 0x02)
            {
                step_index = (step_index + 1) % (sizeof(step_values) / sizeof(step_values[0]));
                current_step_value = step_values[step_index];
            }
        }
        else
        {
            if (YY == 0x02)
            {
                step_index = (step_index - 1 + sizeof(step_values) / sizeof(step_values[0])) % (sizeof(step_values) / sizeof(step_values[0]));
                current_step_value = step_values[step_index];
            }
            else if (YY == 0x01)
            {

                step_index = (step_index + 1) % (sizeof(step_values) / sizeof(step_values[0]));
                current_step_value = step_values[step_index];
            }
        }

        prevA = A;
        prevB = B;

        if (current_step_value <= WithdrawAmount)
        {
            if (C == 0 && ATM_state == WITHDRAWAL_OPTIONS_PRINT)
            {
                if (uxQueueSpacesAvailable(xQueue_step_value))
                {
                    if (xSemaphoreTake(xSemaphore_value, portMAX_DELAY))
                    {

                        if (xQueueSend(xQueue_step_value, &current_step_value, portMAX_DELAY))
                        {
                            xSemaphoreGive(xSemaphore_value);
                            vTaskDelay(100);
                        }
                    }
                }
            }
        }
    }
}
/****************************** End Of Module *******************************/
