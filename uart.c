
/***************************** Include files *******************************/
#include "FreeRTOS.h"
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "emp_type.h"
#include "tmodel.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/*****************************    Defines    *******************************/
extern QueueHandle_t xQueue_UARTtx;
extern QueueHandle_t xQueue_UARTtxnr;
extern QueueHandle_t xQueue_UARTrx;
extern SemaphoreHandle_t xSemaphore_UART;
/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

BOOLEAN uart0_rx_rdy()
/*****************************************************************************
 *   Function : See module specification (.h-file).
 *****************************************************************************/
{
    return (UART0_FR_R & UART_FR_RXFF);
}

INT8U uart0_getc()
/*****************************************************************************
 *   Function : See module specification (.h-file).
 *****************************************************************************/
{
    return (UART0_DR_R);
}

BOOLEAN uart0_tx_rdy()
/*****************************************************************************
 *   Function : See module specification (.h-file).
 *****************************************************************************/
{
    return (UART0_FR_R & UART_FR_TXFE);
}

INT32U lcrh_databits(INT8U antal_databits)
/*****************************************************************************
 *   Input    :
 *   Output   :
 *   Function : sets bit 5 and 6 according to the wanted number of data bits.
 *               5: bit5 = 0, bit6 = 0.
 *               6: bit5 = 1, bit6 = 0.
 *               7: bit5 = 0, bit6 = 1.
 *               8: bit5 = 1, bit6 = 1  (default).
 *              all other bits are returned = 0
 ******************************************************************************/
{
    if ((antal_databits < 5) || (antal_databits > 8))
        antal_databits = 8;
    return (((INT32U)antal_databits - 5) << 5); // Control bit 5-6, WLEN
}

INT32U lcrh_stopbits(INT8U antal_stopbits)
/*****************************************************************************
 *   Input    :
 *   Output   :
 *   Function : sets bit 3 according to the wanted number of stop bits.
 *               1 stpobit:  bit3 = 0 (default).
 *               2 stopbits: bit3 = 1.
 *              all other bits are returned = 0
 ******************************************************************************/
{
    if (antal_stopbits == 2)
        return (0x00000008); // return bit 3 = 1
    else
        return (0x00000000); // return all zeros
}

INT32U lcrh_parity(INT8U parity)
/*****************************************************************************
 *   Input    :
 *   Output   :
 *   Function : sets bit 1, 2 and 7 to the wanted parity.
 *               'e':  00000110b.
 *               'o':  00000010b.
 *               '0':  10000110b.
 *               '1':  10000010b.
 *               'n':  00000000b.
 *              all other bits are returned = 0
 ******************************************************************************/
{
    INT32U result;

    switch (parity)
    {
    case 'e':
        result = 0x00000006;
        break;
    case 'o':
        result = 0x00000002;
        break;
    case '0':
        result = 0x00000086;
        break;
    case '1':
        result = 0x00000082;
        break;
    case 'n':
    default:
        result = 0x00000000;
    }
    return (result);
}

void uart0_fifos_enable()
/*****************************************************************************
 *   Input    :
 *   Output   :
 *   Function : Enable the tx and rx fifos
 ******************************************************************************/
{
    UART0_LCRH_R |= 0x00000010;
}

void uart0_fifos_disable()
/*****************************************************************************
 *   Input    :
 *   Output   :
 *   Function : Enable the tx and rx fifos
 ******************************************************************************/
{
    UART0_LCRH_R &= 0xFFFFFFEF;
}

void uart_send_string(const char *str)
{
    if (uxQueueSpacesAvailable(xQueue_UARTtx))
    {
        if (xSemaphoreTake(xSemaphore_UART, portMAX_DELAY))
        {
            if (xQueueSend(xQueue_UARTtx, &str, portMAX_DELAY))
            {
                xSemaphoreGive(xSemaphore_UART);
            }
        }
    }
}

void uart_send_nr(INT8U num)
{
    if (uxQueueSpacesAvailable(xQueue_UARTtxnr))
    {
        if (xSemaphoreTake(xSemaphore_UART, portMAX_DELAY))
        {
            if (xQueueSend(xQueue_UARTtxnr, &num, portMAX_DELAY))
            {
                xSemaphoreGive(xSemaphore_UART);
            }
        }
    }
}

void uart0_init()
/*****************************************************************************
 *   Function : See module specification (.h-file).
 *****************************************************************************/
{
    INT32U BRD;

#ifndef E_PORTA
#define E_PORTA
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // Enable clock for Port A
#endif

#ifndef E_UART0
#define E_UART0
    SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART0; // Enable clock for UART 0
#endif

    GPIO_PORTA_AFSEL_R = 0x00000003;
    // GPIO_PORTA_DIR_R   = 0x00000002;
    // GPIO_PORTA_DEN_R   = 0x00000003;
    GPIO_PORTA_PUR_R = 0x00000002;

    BRD = 64000000 / 9600;
    UART0_IBRD_R = BRD / 64;
    UART0_FBRD_R = BRD & 0x0000003F;

    UART0_LCRH_R = lcrh_databits(8);
    UART0_LCRH_R += lcrh_stopbits(1);
    UART0_LCRH_R += lcrh_parity('n');

    uart0_fifos_disable();

    UART0_CTL_R |= (UART_CTL_UARTEN | UART_CTL_TXE); // Enable UART
}

void UART_task(void *pvParameters)
{
    uart0_init();
    char *strtx;
    // INT8U chrx;
    INT8U nr;
    while (1)
    {

        // if(uxQueueSpacesAvailable(xQueue_UARTrx))
        //{
        //   if( uart0_rx_rdy() )
        //{
        //   chrx = uart0_getc();
        // if(xSemaphoreTake(xSemaphore_UART, portMAX_DELAY))
        // {
        //   if(xQueueSend(xQueue_UARTrx, &chrx, portMAX_DELAY))
        // {
        //   xSemaphoreGive(xSemaphore_UART);

        // }
        // }
        // }
        // }

        if (uxQueueMessagesWaiting(xQueue_UARTtx))
        {
            if (xSemaphoreTake(xSemaphore_UART, portMAX_DELAY))
            {
                if (xQueueReceive(xQueue_UARTtx, &strtx, portMAX_DELAY))
                {
                    while (*strtx != '\0')
                    {
                        if (uart0_tx_rdy())
                        {
                            UART0_DR_R = *strtx;
                            strtx++;
                        }
                        xSemaphoreGive(xSemaphore_UART);
                    }
                }
            }
        }

        if (uxQueueMessagesWaiting(xQueue_UARTtxnr))
        {
            if (xSemaphoreTake(xSemaphore_UART, portMAX_DELAY))
            {

                if (xQueueReceive(xQueue_UARTtxnr, &nr, portMAX_DELAY))
                {
                    if (uart0_tx_rdy())
                    {
                        UART0_DR_R = nr;
                    }
                    xSemaphoreGive(xSemaphore_UART);
                }
            }
        }
    }
}

/****************************** End Of Module *******************************/
