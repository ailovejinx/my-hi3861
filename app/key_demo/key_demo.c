#include <unistd.h>
#include "stdio.h"
#include "ohos_init.h"
#include "cmsis_os2.h"

#include "iot_errno.h"
#include "iot_gpio.h"

#include <hi_io.h>

#include <hi_gpio.h>

#define KEY_TEST_GPIO 5 // for hispark_pegasus

/* gpio callback func */
hi_void gpio_isr_func(hi_void *arg)
{
    hi_unref_param(arg);
    printf("----- gpio isr success -----\r\n");
}

//按键线程
void *KeyTask(const char *arg)
{
    (void)arg;
#if 0
    IotGpioValue keyval;

    //初始化GPIO
    IoTGpioInit(KEY_TEST_GPIO);

    //设置为GPIO模式
    //hi_io_set_func(KEY_TEST_GPIO, HI_IO_FUNC_GPIO_0_GPIO);

    //设置为输入
    IoTGpioSetDir(KEY_TEST_GPIO, IOT_GPIO_DIR_IN);


    while (1) 
    {
        IoTGpioGetInputVal(KEY_TEST_GPIO, &keyval);
        printf("_____>>>>>> keyval is %d\r\n", keyval);
        usleep(10000);
    }
#else
    hi_u32 ret;

    printf("________>>>>>>>>>>>> ----- gpio isr demo -----\r\n");

    (hi_void)hi_gpio_init();

    ret = hi_gpio_set_dir(HI_GPIO_IDX_0, HI_GPIO_DIR_IN);
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ======gpio -> hi_gpio_set_dir1 ret:%d\r\n", ret);
    }

    ret = hi_gpio_register_isr_function(HI_GPIO_IDX_0, HI_INT_TYPE_LEVEL,
                                        HI_GPIO_EDGE_FALL_LEVEL_LOW, gpio_isr_func, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ======gpio -> hi_gpio_register_isr_function ret:%d\r\n", ret);
    }

    while (1) 
    {
        usleep(10000);
    }
#endif
    return NULL;
}

void key_demo(void)
{
    osThreadAttr_t attr;
    
    attr.name = "KeyTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 2048;
    attr.priority = 26;

    if (osThreadNew((osThreadFunc_t)KeyTask, NULL, &attr) == NULL) {
        printf("[key_demo] Falied to create KeyTask!\n");
    }
    
}


SYS_RUN(key_demo);

