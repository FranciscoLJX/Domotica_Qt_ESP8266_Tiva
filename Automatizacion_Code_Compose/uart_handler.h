#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include "FreeRTOS.h"
#include "queue.h"
#include <stdint.h>

#define NUM_ROOMS   2
#define RX_MSG_MAX  64

void UART_Init(void);
void UART_SendToRoom(const char* msg, int roomId);
void UART_Receive_Handler(void* pvParameters);
BaseType_t UART_WaitForMessage(char* outBuffer, int roomId, TickType_t ticksToWait);

extern QueueHandle_t uartRxQueues[NUM_ROOMS];

#endif
