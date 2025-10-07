#ifndef LOGIC_HANDLER_H
#define LOGIC_HANDLER_H

#include "FreeRTOS.h"
#include "task.h"

void Logic_Init(void);
void Logic_Task(void* pvParameters);
void Logic_Enable(int roomId);
void Logic_Disable(int roomId);

#endif
