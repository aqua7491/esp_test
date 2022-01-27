#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_task_wdt.h"

#define TIME_TO_WAIT_MS         5000            //time to wait for an increment
#define SENDER_TASK_PRIOTITY    2
#define RECV_TASK_PRIORITY      1

#define TEST_TAG                ""
static QueueHandle_t xQueue1 = NULL;

 
typedef struct                                  //main queue message
{
    uint32_t cnt_res;
}xMsg_t;

typedef void (*f_log_cb)(uint32_t);         //pointer of our callback
 
/**
 *  I guess, I didn't understood the test task well. 
 *  I assumed the "time between tasks" is about general time passed by.
 */
static void log_req_cb(uint32_t cnt)          // Log function
{
    ESP_LOGI(TEST_TAG, "Counter is %d now", cnt);
    ESP_LOGI(TEST_TAG, "Time passed: %d s", pdTICKS_TO_MS(xTaskGetTickCount()) / 1000);
}

static void sender_task(void *pvArg)
{
    uint32_t counter = 0;                       // counter to increase
    portTickType xLastWakeTime;                 // used for accurate time delay
    xLastWakeTime = xTaskGetTickCount();

    for(;;) 
    {
        counter++;
        xMsg_t xMsg = {0};
        xMsg.cnt_res = counter;
        //send data via queue
        if (xQueue1 != NULL)
        {
            xQueueSend(xQueue1, (void*)&xMsg, 0);
            vTaskDelayUntil( &xLastWakeTime, ( TIME_TO_WAIT_MS / portTICK_RATE_MS ) );
        }
    }
}

static void rcv_task(void *pvArg)
{   
    f_log_cb log_cb = NULL;                    //pointer to a log callback;
    log_cb = (f_log_cb)pvArg;
    for(;;) 
    {
        if (xQueue1 != NULL)
        {
            xMsg_t xMsg_rcv = {0};
            if (xQueueReceive(xQueue1, &(xMsg_rcv), portMAX_DELAY))     //task sleeps till smth is in the queue
            {
                log_cb(xMsg_rcv.cnt_res);
            }
        }
    }
}


void app_main(void)
{
    TaskHandle_t xHandle1 = NULL;
    TaskHandle_t xHandle2 = NULL;
    BaseType_t xResult = pdFAIL;
    xQueue1 = xQueueCreate( 5, sizeof(xMsg_t));
    
    //SENDER TASK
    xResult = xTaskCreate(sender_task, 
                "sender_tsk", 
                4096, 
                NULL, 
                SENDER_TASK_PRIOTITY, 
                &xHandle1);
    if (xResult != pdPASS)
    {
        vTaskDelete(xHandle1);
    }

    //RECEIVER TASK
    xResult = xTaskCreate(rcv_task, 
                "receiver_tsk", 
                4096, 
                (void*)log_req_cb,             //log callback
                RECV_TASK_PRIORITY, 
                &xHandle2);

    if( xResult != pdPASS )
    {
        vTaskDelete( xHandle2 );
    }
}
