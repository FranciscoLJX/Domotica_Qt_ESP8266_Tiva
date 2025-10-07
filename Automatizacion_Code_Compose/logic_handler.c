#include "logic_handler.h"
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "uart_handler.h"

#define MAX_ROOMS   NUM_ROOMS
#define TASK_STACK  256

// Handle de la tarea de lógica por habitación (activación vía AUTO_ON/AUTO_OFF)
static TaskHandle_t logicTaskHandles[MAX_ROOMS] = {0};



// ---- Reglas de negocio sobre cada línea recibida para roomId ----
static void handleCommand(int roomId, const char* s)
{
    if (!s || !*s) return;

    // Evita ping-pong si alguna unidad ecoa estos comandos
    //if (strcmp(s, "PEDIR_DATOS") == 0 || strcmp(s, "ENVIA_DATOS") == 0) return;

    // Overrides directos (desde App/ESP)
    if (strcmp(s, "RELE1_ON")  == 0) { UART_SendToRoom("RELE1_ON\n",  roomId); return; }
    if (strcmp(s, "RELE1_OFF") == 0) { UART_SendToRoom("RELE1_OFF\n", roomId); return; }
    if (strcmp(s, "RELE2_ON")  == 0) { UART_SendToRoom("RELE2_ON\n",  roomId); return; }
    if (strcmp(s, "RELE2_OFF") == 0) { UART_SendToRoom("RELE2_OFF\n", roomId); return; }

    // Reglas AUTO:
    // 1) Persiana "natural" por pasos desde la luz ambiente
    if (strstr(s, "LUMINOSIDAD_MEDIA")) {
        UART_SendToRoom("RELE1_OFF\n", roomId);
    }else if (strstr(s, "LUMINOSIDAD_BAJA")){
        UART_SendToRoom("RELE1_ON\n", roomId);
        UART_SendToRoom("BLIND_STEP_DOWN\n", roomId);
    } else if (strstr(s, "LUMINOSIDAD_ALTA")) {
        UART_SendToRoom("RELE1_OFF\n", roomId);
        UART_SendToRoom("BLIND_STEP_UP\n", roomId);
    }

    // 3) Temperatura (Relé 2)
    if (strstr(s, "TEMPERATURA_ALTA")) {
        UART_SendToRoom("RELE2_ON\n", roomId);
    } else if (strstr(s, "TEMPERATURA_BAJA")) {
        UART_SendToRoom("RELE2_OFF\n", roomId);
    }
}

// ---- Tarea por habitación: sondea y procesa la cola RX ----
void Logic_Task(void* pvParameters)
{
    int roomId = (int)(intptr_t)pvParameters;
    char buffer[RX_MSG_MAX];

    const TickType_t period = pdMS_TO_TICKS(1000); // 1 s entre sondeos
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        // 1) Sondear al ESP de esta habitación
        UART_SendToRoom("PEDIR_DATOS\n", roomId);

        // 2) Ventana corta para drenar y procesar mensajes recibidos
        TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(300);
        while (xTaskGetTickCount() < deadline) {
            if (UART_WaitForMessage(buffer, roomId, pdMS_TO_TICKS(50)) == pdTRUE) {
                handleCommand(roomId, buffer);
            } else {
                break;
            }
        }

        // 3) Mantener periodicidad estable
        vTaskDelayUntil(&last, period);
    }
}

void Logic_Enable(int roomId) {
    if (roomId < 0 || roomId >= MAX_ROOMS) return;
    if (logicTaskHandles[roomId] == NULL) {
        char name[8]; snprintf(name, sizeof(name), "Room%d", roomId+1);
        xTaskCreate(Logic_Task, name, TASK_STACK, (void*)roomId, 1, &logicTaskHandles[roomId]);
    }
}

void Logic_Disable(int roomId) {
    if (roomId < 0 || roomId >= MAX_ROOMS) return;
    if (logicTaskHandles[roomId] != NULL) {
        vTaskDelete(logicTaskHandles[roomId]);
        logicTaskHandles[roomId] = NULL;
    }
}

