#include <stdio.h>

#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "iot_gpio.h" 

#define LED_TASK_GPIO 9
#define LED_TASK_STACK_SIZE 1024
#define LED_TASK_PRIO 25

static void* GpioTask(const char* arg)
{
    (void)arg;
    IoTGpioInit(LED_TASK_GPIO);
    IoTGpioSetDir(LED_TASK_GPIO,IOT_GPIO_DIR_OUT);

    while (1) {
            // printf(" LED_SPARK! \n");
            IoTGpioSetOutputVal(LED_TASK_GPIO,0);           
            osDelay(50);
            IoTGpioSetOutputVal(LED_TASK_GPIO,1);       
            osDelay(50);
        }
    return NULL;
}

static void GpioExampleEntry(void)
{
    osThreadAttr_t attr;

    attr.name = "GpioTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = LED_TASK_STACK_SIZE;
    attr.priority = LED_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)GpioTask, NULL, &attr) == NULL) {
        printf("[GpioExample] Falied to create GpioTask!\n");
    }
}

SYS_RUN(GpioExampleEntry); // if test add it