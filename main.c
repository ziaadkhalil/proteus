#define F_CPU 8000000UL

#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "UART.h"
#include "LM35.h"
#include "motor.h"
#include "Button.h"

/* ---------------- MOTOR STATES ---------------- */
typedef enum {
    MOTOR_STATE_STOP,
    MOTOR_STATE_FORWARD
} MotorState_t;

/* ---------------- GLOBAL HANDLES ---------------- */
QueueHandle_t xMotorStateQueue;
SemaphoreHandle_t xUART_Mutex;

/* ---------------- TASK 1: BUTTON MONITOR (HIGHEST PRIORITY) ---------------- */
void vTask_ButtonMonitor(void *pv)
{
    uint8_t button_override_active = 0;
    MotorState_t stop_state = MOTOR_STATE_STOP;

    for (;;)
    {
        /* Button is active-low, so BUTTON_read() returns 1 when pressed */
        if (BUTTON_read() == 1)
        {
            if (!button_override_active)
            {
                button_override_active = 1;
                
                /* Send STOP command to override temperature control */
                xQueueSend(xMotorStateQueue, &stop_state, portMAX_DELAY);
                
                if (xSemaphoreTake(xUART_Mutex, portMAX_DELAY) == pdTRUE)
                {
                    UART_sendString("[Button] Pressed: OVERRIDE - Motor Forced OFF\r\n");
                    xSemaphoreGive(xUART_Mutex);
                }
            }
        }
        else
        {
            if (button_override_active)
            {
                button_override_active = 0;
                
                if (xSemaphoreTake(xUART_Mutex, portMAX_DELAY) == pdTRUE)
                {
                    UART_sendString("[Button] Released: Resuming Temperature Control\r\n");
                    xSemaphoreGive(xUART_Mutex);
                }
            }
        }
        
        /* Debounce delay */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ---------------- TASK 2: TEMPERATURE MONITOR (LOWEST PRIORITY) ---------------- */
void vTask_TemperatureMonitor(void *pv)
{
    uint8_t current_temp;
    MotorState_t next_state;

    for (;;)
    {
        current_temp = LM35_read();
        
        if (current_temp > 25)
        {
            next_state = MOTOR_STATE_FORWARD;
        }
        else
        {
            next_state = MOTOR_STATE_STOP;
        }

        /* Log temperature and desired state */
        if (xSemaphoreTake(xUART_Mutex, portMAX_DELAY) == pdTRUE)
        {
            UART_sendString("[Temp] Reading: ");
            uart_print_number(current_temp);
            UART_sendString(" C -> Desired State: ");
            UART_sendString(next_state == MOTOR_STATE_FORWARD ? "FORWARD" : "STOP");
            UART_sendString("\r\n");
            xSemaphoreGive(xUART_Mutex);
        }

        /* Send state to queue - will be ignored by motor task if button override is active logic-wise */
        /* Actually, the motor task just follows the last command. 
           Since button task has higher priority, its STOP command will arrive after any temp command 
           if both happen at the same time. But the button task logic here is to only send ONCE on press.
           To ensure override, we could have the motor task check button state, 
           but the requirement says "button must be implemented with higher priority... immediately overrides".
           So button task sending STOP is correct. */
        xQueueSend(xMotorStateQueue, &next_state, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ---------------- TASK 3: MOTOR CONTROL (MEDIUM PRIORITY) ---------------- */
void vTask_MotorControl(void *pv)
{
    MotorState_t received_state;

    for (;;)
    {
        /* Wait for a command from either the button or temperature task */
        if (xQueueReceive(xMotorStateQueue, &received_state, portMAX_DELAY) == pdTRUE)
        {
            /* Check button state again to ensure override if button is still held */
            if (BUTTON_read() == 1)
            {
                motor_stop();
                if (xSemaphoreTake(xUART_Mutex, portMAX_DELAY) == pdTRUE)
                {
                    UART_sendString("[Motor] Override Active: Staying STOPPED\r\n");
                    xSemaphoreGive(xUART_Mutex);
                }
            }
            else
            {
                if (received_state == MOTOR_STATE_FORWARD)
                {
                    motor_forward();
                }
                else
                {
                    motor_stop();
                }

                if (xSemaphoreTake(xUART_Mutex, portMAX_DELAY) == pdTRUE)
                {
                    UART_sendString("[Motor] Executing: ");
                    UART_sendString(received_state == MOTOR_STATE_FORWARD ? "FORWARD" : "STOP");
                    UART_sendString("\r\n");
                    xSemaphoreGive(xUART_Mutex);
                }
            }
        }
    }
}

/* ---------------- MAIN ---------------- */
int main(void)
{
    /* Initialize Hardware */
    UART_init(9600);
    LM35_init();
    motor_init();
    BUTTON_init();

    /* Create IPC Mechanisms */
    xMotorStateQueue = xQueueCreate(5, sizeof(MotorState_t));
    xUART_Mutex = xSemaphoreCreateMutex();

    if (xMotorStateQueue != NULL && xUART_Mutex != NULL)
    {
        /* Create Tasks */
        /* Button Monitor: Highest Priority (3) */
        xTaskCreate(vTask_ButtonMonitor, "BTN", 100, NULL, 3, NULL);
        
        /* Motor Control: Medium Priority (2) */
        xTaskCreate(vTask_MotorControl, "MTR", 100, NULL, 2, NULL);
        
        /* Temperature Monitor: Lowest Priority (1) */
        xTaskCreate(vTask_TemperatureMonitor, "TMP", 100, NULL, 1, NULL);

        /* Start Scheduler */
        vTaskStartScheduler();
    }
    else
    {
        /* Initialization failed */
        UART_sendString("ERROR: Failed to create Queue or Mutex!\r\n");
    }

    while (1);
}
