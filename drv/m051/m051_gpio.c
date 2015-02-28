#include "m051_gpio.h"
#include "../../userspace/m051_driver.h"
#include "m051_core_private.h"
#include "sys_config.h"
#if (SYS_INFO)
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#endif
#include <string.h>

#include "lib_gpio.h"

#define PORT_OFFSET   0x40


void m051_gpio_init(CORE* core){
    memset(&core->gpio, 0, sizeof (GPIO_DRV));
}

bool m051_gpio_request(CORE* core, IPC* ipc){
    bool need_post = false;

//    printk("m051_gpio_request cmd: %#X\r\n", ipc->cmd);
    switch (ipc->cmd){
#if (SYS_INFO)
    case IPC_GET_INFO:
//        M051_gpio_info(&core->gpio);
        need_post = true;
        break;
#endif
    case M051_GPIO_DISABLE_PIN:
//        M051_gpio_disable_pin(&core->gpio, (PIN)ipc->param1);
        need_post = true;
        break;
    case M051_GPIO_ENABLE_PIN:
//        m051_gpio_enable_pin(&core->gpio, (PIN)ipc->param1, (M051_GPIO_MODE)ipc->param2, ipc->param3);
        need_post = true;
        break;
//    case M051_GPIO_ENABLE_EXTI:
//        M051_gpio_enable_exti(&core->gpio, (PIN)ipc->param1, ipc->param2);
//        need_post = true;
//        break;
//    case M051_GPIO_DISABLE_EXTI:
//        M051_gpio_disable_exti(&core->gpio, (PIN)ipc->param1);
//        need_post = true;
//        break;
//    case M051_GPIO_DISABLE_JTAG:
//        M051_gpio_disable_jtag(&core->gpio);
//        need_post = true;
//        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}


//void m051_gpio_enable_pin(GPIO_DRV* gpio, PIN pin, M051_GPIO_MODE mode, bool pullup){}

void m051_lib_gpio_enable_pin(unsigned int pin, /*E_DRVGPIO_IO*/ GPIO_MODE Mode){
    E_DRVGPIO_IO mode = Mode;
    unsigned int port = 0;
    volatile uint32_t u32Reg;

    if(pin <= P07)port = 0;
    else if(pin >= P10 && pin <= P17)port = 1;
    else if(pin >= P20 && pin <= P27)port = 2;
    else if(pin >= P30 && pin <= P37)port = 3;
    else if(pin >= P40 && pin <= P47)port = 4;
    pin -= 8 * port;

    u32Reg = (uint32_t)((uint32_t)&PORT0->PMD + (port*PORT_OFFSET));

    if ((mode == E_IO_INPUT) ||  (mode == E_IO_OUTPUT) || (mode == E_IO_OPENDRAIN))  {
        outpw(u32Reg, inpw(u32Reg) & ~(0x3<<(pin*2)));

        if (mode == E_IO_OUTPUT)        {
            outpw(u32Reg, inpw(u32Reg) | (0x1<<(pin*2)));
        }else
        if (mode == E_IO_OPENDRAIN)    {
            outpw(u32Reg, inpw(u32Reg) | (0x2<<(pin*2)));
        }
    }else
    if (mode == E_IO_QUASI)   {
        outpw(u32Reg, inpw(u32Reg) | (0x3<<(pin*2)));
    }

}
void m051_lib_gpio_set_pin(unsigned int pin){
    unsigned int port = 0;

    if(pin <= P07)port = 0;
    else if(pin >= P10 && pin <= P17)port = 1;
    else if(pin >= P20 && pin <= P27)port = 2;
    else if(pin >= P30 && pin <= P37)port = 3;
    else if(pin >= P40 && pin <= P47)port = 4;
    pin -= 8 * port;

    outpw((PORT_BIT_DOUT + (port*0x20) + (pin*4)), 1);
}
void m051_lib_gpio_reset_pin(unsigned int pin){
    unsigned int port = 0;

    if(pin <= P07)port = 0;
    else if(pin >= P10 && pin <= P17)port = 1;
    else if(pin >= P20 && pin <= P27)port = 2;
    else if(pin >= P30 && pin <= P37)port = 3;
    else if(pin >= P40 && pin <= P47)port = 4;
    pin -= 8 * port;

    outpw((PORT_BIT_DOUT + (port*0x20) + (pin*4)), 0);
}
void m051_lib_gpio_enable_mask(unsigned int port, GPIO_MODE mode, unsigned int mask){}
void m051_lib_gpio_disable_pin(unsigned int pin){}
void m051_lib_gpio_disable_mask(unsigned int port, unsigned int mask){}
void m051_lib_gpio_set_mask(unsigned int port, unsigned int mask){}
void m051_lib_gpio_reset_mask(unsigned int port, unsigned int mask){}
bool m051_lib_gpio_get_pin(unsigned int pin){return 0;}
unsigned int m051_lib_gpio_get_mask(unsigned port, unsigned int mask){return 0;}
void m051_lib_gpio_set_data_out(unsigned int port, unsigned int wide){}
void m051_lib_gpio_set_data_in(unsigned int port, unsigned int wide){}

const LIB_GPIO __LIB_GPIO = {
    m051_lib_gpio_enable_pin,
    m051_lib_gpio_enable_mask,
    m051_lib_gpio_disable_pin,
    m051_lib_gpio_disable_mask,
    m051_lib_gpio_set_pin,
    m051_lib_gpio_set_mask,
    m051_lib_gpio_reset_pin,
    m051_lib_gpio_reset_mask,
    m051_lib_gpio_get_pin,
    m051_lib_gpio_get_mask,
    m051_lib_gpio_set_data_out,
    m051_lib_gpio_set_data_in
};
