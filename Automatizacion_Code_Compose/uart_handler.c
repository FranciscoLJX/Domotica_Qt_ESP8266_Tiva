#include "uart_handler.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

#include "logic_handler.h"

QueueHandle_t uartRxQueues[NUM_ROOMS];

typedef struct {
    uint32_t base;
    uint32_t uartPeriph;
    uint32_t gpioPeriph;
    uint32_t portBase;
    uint8_t  pinRx;
    uint8_t  pinTx;
    uint32_t pinRxConf;
    uint32_t pinTxConf;
    bool     needsPD7Unlock;
} UART_Config;

// UART1: PB0/PB1 — UART2: PD6/PD7 (PD7 requiere unlock)
static const UART_Config uartCfg[NUM_ROOMS] = {
    { UART1_BASE, SYSCTL_PERIPH_UART1, SYSCTL_PERIPH_GPIOB, GPIO_PORTB_BASE,
      GPIO_PIN_0, GPIO_PIN_1, GPIO_PB0_U1RX, GPIO_PB1_U1TX, false },

    { UART2_BASE, SYSCTL_PERIPH_UART2, SYSCTL_PERIPH_GPIOD, GPIO_PORTD_BASE,
      GPIO_PIN_6, GPIO_PIN_7, GPIO_PD6_U2RX, GPIO_PD7_U2TX, true  }
};

static void unlockPD7IfNeeded(const UART_Config* c) {
    if (c->needsPD7Unlock && c->portBase == GPIO_PORTD_BASE && (c->pinTx & GPIO_PIN_7)) {
        HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
        HWREG(GPIO_PORTD_BASE + GPIO_O_CR)  |= GPIO_PIN_7;
        HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = 0;
    }
}

void UART_Init(void) {
    int i;
    for (i = 0; i < NUM_ROOMS; ++i) {
        const UART_Config* c = &uartCfg[i];

        SysCtlPeripheralEnable(c->uartPeriph);
        SysCtlPeripheralEnable(c->gpioPeriph);
        while(!SysCtlPeripheralReady(c->uartPeriph)){}
        while(!SysCtlPeripheralReady(c->gpioPeriph)){}

        unlockPD7IfNeeded(c);

        GPIOPinConfigure(c->pinRxConf);
        GPIOPinConfigure(c->pinTxConf);
        GPIOPinTypeUART(c->portBase, c->pinRx | c->pinTx);

        UARTConfigSetExpClk(c->base, SysCtlClockGet(), 9600,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
        UARTFIFOEnable(c->base);

        // Cola de recepción por habitación (8 mensajes de 64 bytes)
        uartRxQueues[i] = xQueueCreate(8, RX_MSG_MAX);
        configASSERT(uartRxQueues[i] != NULL);
    }
}

void UART_SendToRoom(const char* msg, int roomId)
{
    if (roomId < 0 || roomId >= NUM_ROOMS || msg == NULL) return;

    const char* p = msg;
    while (*p) {
        UARTCharPut(uartCfg[roomId].base, *p++);
    }
}

// Tarea de RX por habitación: bufferiza hasta '\n' y encola la línea
void UART_Receive_Handler(void* pvParameters) {
    int roomId = (int)(intptr_t)pvParameters;
    char buffer[RX_MSG_MAX];
    int index = 0;

    for (;;) {
        if (roomId < 0 || roomId >= NUM_ROOMS) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }

        // Vaciar FIFO en ráfaga para no perder caracteres
        while (UARTCharsAvail(uartCfg[roomId].base)) {
            char c = (char)UARTCharGet(uartCfg[roomId].base);
            if (c == '\r') continue;

            if (c == '\n') {
                buffer[index] = '\0';  // cerrar la cadena

                // Comandos de control del modo automático
                if (strcmp(buffer, "AUTO_ON_ALL") == 0) {
                    int i;
                    for (i = 0; i < NUM_ROOMS; ++i)
                        Logic_Enable(i);
                } else if (strcmp(buffer, "AUTO_ON") == 0) {
                    Logic_Enable(roomId);
                } else if (strcmp(buffer, "AUTO_OFF") == 0) {
                    Logic_Disable(roomId);
                } else {
                    // Mensaje “normal” hacia la lógica
                    (void)xQueueSend(uartRxQueues[roomId], buffer, 0);
                }

                index = 0; // reset para el siguiente mensaje
            } else if (index < (RX_MSG_MAX - 1)) {
                buffer[index++] = c; // guardar y avanzar
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

BaseType_t UART_WaitForMessage(char* outBuffer, int roomId, TickType_t ticksToWait) {
    if (roomId < 0 || roomId >= NUM_ROOMS) return pdFALSE;
    return xQueueReceive(uartRxQueues[roomId], outBuffer, ticksToWait);
}


