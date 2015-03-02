#include "m051_wdt.h"
#include "../../userspace/sys.h"
#include "../../userspace/wdt.h"
#include "m051_config.h"


void m051_wdt_init(){
    // enable Wdog
    SYSCLK->CLKSEL1.WDT_S = 0x03;		// Internal 10K
    SYSCLK->APBCLK.WDT_EN = 1;			// Enable WatchDog Timer Clock
    UNLOCKREG();
    // Select WatchDog Timer Interval
//    WDT->WTCR.WTIS = 3;		// 100 to 200 ms
    WDT->WTCR.WTIS = 5;		// 1.6 to 1.75 s
    WDT->WTCR.WTRE = 0;					// Step 3. Disable Watchdog Timer Reset function
    WDT->WTCR.WTIF = 1;					// Write 1 to clear for safety
//    WDT->WTCR.WTIE = 1;					// Enable WDT interrupt
    WDT->WTCR.WTIE = 0;
//    NVIC_EnableIRQ(WDT_IRQn);			// Enable WDT Interrupt
    WDT->WTCR.WTR = 1;					// reset Wdog Timer
    WDT->WTCR.WTRE = 1;					// enable Watchdog Timer Reset function
    WDT->WTCR.WTE = 1;					//
    LOCKREG();
}
void m051_wdt_kick(){
    UNLOCKREG();
    WDT->WTCR.WTR = 1;	// reset Wdog Timer
    LOCKREG();
}


bool m051_wdt_request(IPC* ipc){
    bool need_post = false;
    switch (ipc->cmd)    {
    case WDT_KICK:
        m051_wdt_kick();
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}
