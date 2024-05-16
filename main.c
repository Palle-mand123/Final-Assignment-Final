/***************************** Include files *******************************/
#include <stdint.h>
#include "FreeRTOS.h"
#include "tm4c123gh6pm.h"
#include "emp_type.h"
#include "gpio.h"
#include "lcd.h"
#include "task.h"
#include "queue.h"
#include "adc.h"
#include "semphr.h"
#include "systick_frt.h"
#include "rotary.h"
#include "keypad.h"
#include "leds.h"
#include "ATM.h"
#include "userSwitch.h"
#include "uart.h"
/*****************************    Defines    *******************************/
#define USERTASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define IDLE_PRIO 0
#define LOW_PRIO  1
#define MED_PRIO  2
#define HIGH_PRIO 3
#define QUEUE_LEN 128
QueueHandle_t xQueue_button;
SemaphoreHandle_t xSemaphore_button;
QueueHandle_t xQueue_lcd;
SemaphoreHandle_t xSemaphore_lcd;
QueueHandle_t xQueue_keypad;
SemaphoreHandle_t xSemaphore_keypad;
QueueHandle_t xQueue_ATM;
SemaphoreHandle_t xSemaphore_ATM;
SemaphoreHandle_t xSemaphore_value;
QueueHandle_t xQueue_step_value;
QueueHandle_t xQueue_UARTtx;
QueueHandle_t xQueue_UARTtxnr;
QueueHandle_t xQueue_UARTrx;
SemaphoreHandle_t xSemaphore_UART;
/*****************************   Constants   *******************************/
/*****************************   Variables   *******************************/
INT16U WithdrawAmount;
/*****************************   Functions   *******************************/
static void setupHardware(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :
*****************************************************************************/
{

  init_systick();
  init_gpio();
  init_rotary();
  init_adc();
  red_led_init();
  yellow_led_init();
  green_led_init();
  xQueue_lcd = xQueueCreate(QUEUE_LEN, sizeof(INT8U));
  xQueue_keypad = xQueueCreate(QUEUE_LEN, sizeof(INT8U));
  xQueue_ATM = xQueueCreate(QUEUE_LEN, sizeof(INT16U));
  xQueue_step_value = xQueueCreate(QUEUE_LEN, sizeof(INT16U));
  xQueue_button = xQueueCreate(QUEUE_LEN, sizeof(INT8U));
  xQueue_UARTtx = xQueueCreate(QUEUE_LEN, sizeof(char *));
  xQueue_UARTtxnr = xQueueCreate(QUEUE_LEN, sizeof(INT8U));
  xQueue_UARTrx = xQueueCreate(QUEUE_LEN, sizeof(INT8U));
  xSemaphore_lcd = xSemaphoreCreateMutex();
  xSemaphore_keypad = xSemaphoreCreateMutex();
  xSemaphore_ATM = xSemaphoreCreateMutex();
  xSemaphore_value = xSemaphoreCreateMutex();
  xSemaphore_button = xSemaphoreCreateMutex();
  xSemaphore_UART = xSemaphoreCreateMutex();
}
int main(void) {
    // Initialize hardware peripherals (e.g., GPIO) here
    setupHardware();
    // Initialize the LCD module
    xTaskCreate(button_task, "button", configMINIMAL_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate(rotary_task, "rotary", configMINIMAL_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate(key_task, "keyboard", configMINIMAL_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate(lcd_task, "LCDTask", configMINIMAL_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate(led_task, "LEDTASK", configMINIMAL_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate(ATM_task, "ATMTASK", configMINIMAL_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate(UART_task, "uart", configMINIMAL_STACK_SIZE, NULL, LOW_PRIO, NULL);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();
    return 0;
}
/****************************** End Of Module *******************************/
