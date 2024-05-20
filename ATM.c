/***************************** Include files *******************************/
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "emp_type.h"
#include "tmodel.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "adc.h"
#include "lcd.h"
#include "keypad.h"
#include "rotary.h"
#include "userSwitch.h"
#include "ATM.h"
#include "uart.h"

/*****************************    Defines    *******************************/

extern SemaphoreHandle_t xSemaphore_ATM;
extern QueueHandle_t xQueue_ATM;
extern SemaphoreHandle_t xSemaphore_keypad;
extern QueueHandle_t xQueue_keypad;
extern QueueHandle_t xQueue_button;
extern SemaphoreHandle_t xSemaphore_button;

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/
volatile static INT32U Saldo = 0;
volatile static INT16U Pin;
extern INT16U WithdrawAmount;
extern INT8U frequency;
extern INT16U current_step_value;
/*****************************   Functions   *******************************/



void send_saldo(INT32U f)
{
        uart_send_nr(f / 100000000 + '0');
        f = f % 100000000;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        uart_send_nr(f / 10000000 + '0');
        f = f % 10000000;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        uart_send_nr(f / 1000000 + '0');
        f = f % 1000000;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        uart_send_nr(f / 100000 + '0');
        f = f % 100000;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        uart_send_nr(f / 10000 + '0');
        f = f % 10000;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        uart_send_nr(f / 1000 + '0');
        f = f % 1000;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        uart_send_nr(f / 100 + '0');
        f = f % 100;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        uart_send_nr(f / 10 + '0');
        f = f % 10;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        uart_send_nr(f + '0');
}

void ADC_show(INT8U s)
{
    if (s != 0)
    {
        Set_cursor(0x40 + 0x80 + 12);
        wr_ch_LCD(s / 10 + '0');
        vTaskDelay(10);
        s = s % 10;
        Set_cursor(0x40 + 0x80 + 13);
        wr_ch_LCD(s + '0');
        vTaskDelay(10);
        Set_cursor(0x40 + 0x80 + 14);
        wr_ch_LCD('H');
        vTaskDelay(10);
        Set_cursor(0x40 + 0x80 + 15);
        wr_ch_LCD('z');
        vTaskDelay(10);
    }
}

void send_encoder_position(INT16U p)
{
    if (p != 0)
    {
        Set_cursor(0x40 + 0x80);
        p = p % 1000;
        wr_ch_LCD(p / 100 + '0');
        vTaskDelay(10);
        Set_cursor(0x40 + 0x80 + 1);
        p = p % 100;
        wr_ch_LCD(p / 10 + '0');
        vTaskDelay(10);
        Set_cursor(0x40 + 0x80 + 2);
        p = p % 10;
        wr_ch_LCD(p + '0');
        vTaskDelay(10);
    }
}

enum ATM_states ATM_state = CREDIT_CARD_SALDO;

void set_ATM_state(enum ATM_states new_state)
{
    ATM_state = new_state;
}

void ATM_task(void *pvParameters)
{
    wr_str_LCD("Enter Saldo: ");
    static char key;
    static int step = 0;
    static INT8U pos = 0;
    static INT8U buttonEvent;

    while (1)
    {
        switch (ATM_state)
        {
        case CREDIT_CARD_SALDO:
            if (uxQueueMessagesWaiting(xQueue_button))
            {
                if (xSemaphoreTake(xSemaphore_button, portMAX_DELAY))
                {
                    if (xQueueReceive(xQueue_button, &buttonEvent, portMAX_DELAY))
                    {
                        xSemaphoreGive(xSemaphore_button);
                    }
                }
            }

            while (pos < 9)
            {
                if (uxQueueMessagesWaiting(xQueue_button))
                {
                    if (xSemaphoreTake(xSemaphore_button, portMAX_DELAY))
                    {
                        if (xQueueReceive(xQueue_button, &buttonEvent, portMAX_DELAY))
                        {
                            xSemaphoreGive(xSemaphore_button);
                        }
                    }
                }
            if (uxQueueMessagesWaiting(xQueue_keypad))
            {
                if (xSemaphoreTake(xSemaphore_keypad, (TickType_t)10) == pdTRUE)
                {
                    if (xQueueReceive(xQueue_keypad, &key, portMAX_DELAY))
                    {
                        Set_cursor(0x40 + 0x80 + pos);
                        wr_ch_LCD(key);
                        pos++;

                        Saldo = Saldo * 10 + (key - '0');

                        xSemaphoreGive(xSemaphore_keypad);
                    }
                }
            }
            if (buttonEvent == 0x01)
            {
                set_ATM_state(PIN_CODE);
                clr_LCD();
                buttonEvent = 0x00;
                break;
            }

            }
            if (buttonEvent == 0x01)
            {
                set_ATM_state(PIN_CODE);
                clr_LCD();
                buttonEvent = 0x00;
            }
            break;

        case PIN_CODE:
            xQueue_keypad = xQueueCreate(128, sizeof(INT8U));
            uart_send_string("Your saldo is:  ");
            vTaskDelay(100 / portTICK_PERIOD_MS);
            send_saldo(Saldo);
            vTaskDelay(10 / portTICK_PERIOD_MS);
            UART0_DR_R = 0x0A;
            UART0_DR_R = 0x0D;
            pos = 0;
            Pin = 0;
            clr_LCD();
            vTaskDelay(30);
            home_LCD();
            vTaskDelay(30);
            wr_str_LCD("Enter pin: ");
            vTaskDelay(30);
            while (ATM_state == PIN_CODE)
            {
                if (uxQueueMessagesWaiting(xQueue_button))
                {
                    if (xSemaphoreTake(xSemaphore_button, portMAX_DELAY))
                    {
                        if (xQueueReceive(xQueue_button, &buttonEvent, portMAX_DELAY))
                        {
                            xSemaphoreGive(xSemaphore_button);
                        }
                    }
                }
                while (pos < 4)
                {
                    if (uxQueueMessagesWaiting(xQueue_keypad))
                    {
                        if (xSemaphoreTake(xSemaphore_keypad, (TickType_t)10) == pdTRUE)
                        {
                            if (xQueueReceive(xQueue_keypad, &key, portMAX_DELAY))
                            {
                                Set_cursor(0x40 + 0x80 + pos);
                                wr_ch_LCD(key);
                                pos++;

                                Pin = Pin * 10 + (key - '0');

                                xSemaphoreGive(xSemaphore_keypad);
                            }
                        }
                    }
                }
                if (buttonEvent == 0x01)
                {
                    clr_LCD();
                    set_ATM_state(CHECK_PIN);
                    vTaskDelay(10);
                    buttonEvent = 0x00;
                }
            }

            break;
        case CHECK_PIN:
            if ((Pin % 8) == 0 && Pin != 0)
            {
                set_ATM_state(WITHDRAWAL_AMOUNTS);
            }
            else
            {
                wr_str_LCD("Wrong Pin-Code");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                clr_LCD();
                vTaskDelay(10);
                set_ATM_state(PIN_CODE);
            }
            break;

        case WITHDRAWAL_AMOUNTS:
            step = 0;
            WithdrawAmount = 0x00;
            home_LCD();
            vTaskDelay(10);
            wr_str_LCD("Withdraw:");
            vTaskDelay(10);
            Set_cursor(0x40 + 0x80 + 1);
            wr_str_LCD("500");
            vTaskDelay(10);
            Set_cursor(0x40 + 0x80 + 5);
            wr_str_LCD("200");
            vTaskDelay(10);
            Set_cursor(0x40 + 0x80 + 9);
            wr_str_LCD("100");
            vTaskDelay(10);
            Set_cursor(0x40 + 0x80 + 13);
            wr_str_LCD("50");
            while (ATM_state == WITHDRAWAL_AMOUNTS)
            {
                if (uxQueueMessagesWaiting(xQueue_button))
                {
                    if (xSemaphoreTake(xSemaphore_button, portMAX_DELAY))
                    {
                        if (xQueueReceive(xQueue_button, &buttonEvent, portMAX_DELAY))
                        {
                            xSemaphoreGive(xSemaphore_button);
                        }
                    }
                }

                if (step == 0)
                {
                    vTaskDelay(10);
                    Set_cursor(0x40 + 0x80);
                    wr_str_LCD(">");
                    vTaskDelay(10);
                    home_LCD();

                    if (buttonEvent == 0x02)
                    {
                        Set_cursor(0x40 + 0x80);
                        wr_str_LCD(" ");
                        step++;
                        buttonEvent = 0x00;
                    }
                    if (buttonEvent == 0x03)
                    {
                        WithdrawAmount = WithdrawAmount + 0x1F4;
                        if (WithdrawAmount <= Saldo)
                        {
                            Saldo = Saldo - WithdrawAmount;
                            set_ATM_state(WITHDRAWAL_OPTIONS_PRINT);
                            buttonEvent = 0x00;
                        }
                        else
                        {
                            clr_LCD();
                            vTaskDelay(250 / portTICK_PERIOD_MS);
                            wr_str_LCD("Not enough saldo!");
                            vTaskDelay(1000 / portTICK_PERIOD_MS);
                            clr_LCD();
                            vTaskDelay(300 / portTICK_PERIOD_MS);
                            WithdrawAmount = 0x00;
                            buttonEvent = 0x00;
                            set_ATM_state(CHECK_PIN);
                        }
                    }
                }

                if (step == 1)
                {
                    vTaskDelay(10);
                    Set_cursor(0x40 + 0x80 + 4);
                    wr_str_LCD(">");
                    vTaskDelay(10);
                    home_LCD();

                    if (buttonEvent == 0x02)
                    {
                        Set_cursor(0x40 + 0x80 + 4);
                        wr_str_LCD(" ");
                        step++;
                        buttonEvent = 0x00;
                    }

                    if (buttonEvent == 0x01)
                    {
                        Set_cursor(0x40 + 0x80 + 4);
                        wr_str_LCD(" ");
                        step--;
                        buttonEvent = 0x00;
                    }
                    if (buttonEvent == 0x03)
                    {
                        WithdrawAmount = WithdrawAmount + 0xc8;
                        if (WithdrawAmount <= Saldo)
                        {
                            Saldo = Saldo - WithdrawAmount;
                            set_ATM_state(WITHDRAWAL_OPTIONS_PRINT);
                            buttonEvent = 0x00;
                        }
                        else
                        {
                            clr_LCD();
                            vTaskDelay(250 / portTICK_PERIOD_MS);
                            wr_str_LCD("Not enough saldo!");
                            vTaskDelay(1000 / portTICK_PERIOD_MS);
                            clr_LCD();
                            vTaskDelay(300 / portTICK_PERIOD_MS);
                            WithdrawAmount = 0x00;
                            buttonEvent = 0x00;
                            set_ATM_state(CHECK_PIN);
                        }
                    }
                }

                if (step == 2)
                {
                    vTaskDelay(10);
                    Set_cursor(0x40 + 0x80 + 8);
                    wr_str_LCD(">");
                    vTaskDelay(10);
                    home_LCD();
                    if (buttonEvent == 0x02)
                    {
                        Set_cursor(0x40 + 0x80 + 8);
                        wr_str_LCD(" ");
                        step++;
                        buttonEvent = 0x00;
                    }
                    if (buttonEvent == 0x01)
                    {
                        Set_cursor(0x40 + 0x80 + 8);
                        wr_str_LCD(" ");
                        step--;
                        buttonEvent = 0x00;
                    }
                    if (buttonEvent == 0x03)
                    {
                        WithdrawAmount = WithdrawAmount + 0x64;
                        if (WithdrawAmount <= Saldo)
                        {
                            Saldo = Saldo - WithdrawAmount;
                            set_ATM_state(WITHDRAWAL_OPTIONS_PRINT);
                            buttonEvent = 0x00;
                        }
                        else
                        {
                            clr_LCD();
                            vTaskDelay(250 / portTICK_PERIOD_MS);
                            wr_str_LCD("Not enough saldo!");
                            vTaskDelay(1000 / portTICK_PERIOD_MS);
                            clr_LCD();
                            vTaskDelay(300 / portTICK_PERIOD_MS);
                            WithdrawAmount = 0x00;
                            buttonEvent = 0x00;
                            set_ATM_state(CHECK_PIN);
                        }
                    }
                }

                if (step == 3)
                {
                    vTaskDelay(10);
                    Set_cursor(0x40 + 0x80 + 12);
                    wr_str_LCD(">");
                    vTaskDelay(10);
                    home_LCD();

                    if (buttonEvent == 0x01)
                    {
                        Set_cursor(0x40 + 0x80 + 12);
                        wr_str_LCD(" ");
                        step--;
                        buttonEvent = 0x00;
                    }
                    if (buttonEvent == 0x03)
                    {
                        WithdrawAmount = WithdrawAmount + 0x32;
                        if (WithdrawAmount <= Saldo)
                        {
                            Saldo = Saldo - WithdrawAmount;
                            set_ATM_state(WITHDRAWAL_OPTIONS_PRINT);
                            buttonEvent = 0x00;
                        }
                        else
                        {
                            clr_LCD();
                            vTaskDelay(250 / portTICK_PERIOD_MS);
                            wr_str_LCD("Not enough saldo!");
                            vTaskDelay(1000 / portTICK_PERIOD_MS);
                            clr_LCD();
                            vTaskDelay(300 / portTICK_PERIOD_MS);
                            WithdrawAmount = 0x00;
                            buttonEvent = 0x00;
                            set_ATM_state(CHECK_PIN);
                        }
                    }
                }
            }
            break;

        case WITHDRAWAL_OPTIONS_PRINT:
            clr_LCD();
            vTaskDelay(10);
            home_LCD();
            vTaskDelay(10);
            wr_str_LCD("Choose Notes:");
            vTaskDelay(10);
            while (ATM_state == WITHDRAWAL_OPTIONS_PRINT)
            {
                // rotary and led task is working here
                frequency = get_adc() * 0.0022 + 1.0;
                ADC_show(frequency);
                send_encoder_position(current_step_value);
            }
            break;
        }
    }
}
/****************************** End Of Module *******************************/
