#include "m051_core.h"
#include "../../rexos/userspace/process.h"
#include "../../rexos/userspace/time.h"
#include "../../rexos/userspace/stdlib.h"
#include "../../rexos/userspace/stdio.h"
#include "../../rexos/userspace/heap.h"
#include "../../rexos/userspace/timer.h"
#include "../../rexos/userspace/ipc.h"
#include "../../rexos/userspace/stream.h"
#include "../../rexos/userspace/direct.h"
#include "../../rexos/userspace/object.h"
#include "../../rexos/userspace/sys.h"
#include "../../rexos/userspace/gpio.h"
#include "../../rexos/userspace/rtc.h"
#include "../../rexos/userspace/wdt.h"
#include "../../rexos/userspace/m051_driver.h"
#include "../../rexos/drv/m051/m051_uart.h"
#include "../../rexos/drv/ssd1306.h"
#include "../../rexos/drv/m051/m051_gpio.h"
#include "../../rexos/userspace/file.h"

#define TEST_ROUNDS                                 10000

void app();
void test_thread3();

const REX __APP = {
    //name
    "App main",
    //size
    512,
    //priority
    200,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    app
};

const REX test3 = {
    //name
    "test3",
    //size
    512,
    //priority
    202,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    test_thread3
};
void test_thread3(){
    TIME uptime;
    open_stdout();
    printf("test thread3 started\n\r");
    printf("my name is: %s\n\r", process_name());

//    gpio_enable_pin(B9, GPIO_MODE_OUT);
    gpio_enable_pin(P40, E_IO_OUTPUT);

    wdt_kick();

        sleep_ms(1000);
        process_info();
    bool set = true;
    for (;;){
        get_uptime(&uptime);
        printf("uptime: %02d:%02d.%06d\n\r", uptime.sec / 60, uptime.sec % 60, uptime.usec);
        printf("rtc: %d\n\r", rtc_get());
        set ? gpio_set_pin(P40) : gpio_reset_pin(P40);
        wdt_kick();
        set = !set;
        sleep_ms(1000);
//        process_info();
    }
}
//-------------------------------------------------------------------------------
void app(){
    HANDLE core;

	gpio_enable_pin(P34, E_IO_OUTPUT);
	gpio_enable_pin(P40, E_IO_OUTPUT);
	gpio_enable_pin(P41, E_IO_OUTPUT);
	gpio_set_pin(P40);
	gpio_set_pin(P41);
	gpio_reset_pin(P41);

    core = process_create(&__M051_CORE);
#if !(MONOLITH_UART)
    HANDLE uart = process_create(&__M051_UART);
#endif

//	gpio_reset_pin(P41);	//red LED
	gpio_reset_pin(P40);	//green LED


    TIME uptime;
    int i;
    unsigned int diff;
    open_stdout();
    open_stdin();

    printf("App started\n\r");
//    wdt_kick();

    //first second signal may go faster
    sleep_ms(1000);
//    wdt_kick();

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        svc_test();
//    wdt_kick();
    diff = time_elapsed_us(&uptime);
    printf("average kernel call time: %d.%dus\n\r", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        process_switch_test();
//    wdt_kick();
    diff = time_elapsed_us(&uptime);
    printf("average switch time: %d.%dus\n\r", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    process_create(&test3);
        sleep_ms(1000);
//        process_info();

    ack(core, IPC_GET_INFO, 0, 0, 0);
#if !(MONOLITH_UART)
    ack(uart, IPC_GET_INFO, 0, 0, 0);
#endif

    bool set = true;
    for (;;){
        sleep_ms(1000);
        set ? gpio_set_pin(P34) : gpio_reset_pin(P34);
	set = !set;
    }
}
