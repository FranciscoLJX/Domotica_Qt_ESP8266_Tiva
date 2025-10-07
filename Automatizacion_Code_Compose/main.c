#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "driverlib/sysctl.h"
#include "uart_handler.h"
#include "logic_handler.h"

 int main(void) {


    UART_Init();

    // Una tarea de recepción por habitación (cola por UART)
    xTaskCreate(UART_Receive_Handler, "UartR1", 256, (void*)0, 2, NULL);
    xTaskCreate(UART_Receive_Handler, "UartR2", 256, (void*)1, 2, NULL);

    vTaskStartScheduler();
    for(;;);
}

// Hooks de FreeRTOS
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) { while (1); }
void vApplicationIdleHook(void) { SysCtlSleep(); }
void vApplicationMallocFailedHook(void) { while (1); }
void __error__(char *pcFilename, uint32_t ui32Line) { while (1); }

