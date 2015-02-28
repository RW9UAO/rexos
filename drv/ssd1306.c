#include "ssd1306.h"
#include "../userspace/gpio.h"
#if (SYS_INFO) || (MT_TEST)
#include "../userspace/stdio.h"
#include "../userspace/timer.h"
#endif
#if (MT_DRIVER)
#include "../userspace/block.h"
#include "../userspace/direct.h"
#endif


#define CLKS_TEST_ROUNDS                10000000

bool display_status = false;

#if (MT_DRIVER)
void oled();

const REX __OLED = {
    //name
    "SSD1306 driver",
    //size
    MT_STACK_SIZE,
    //priority - driver priority
    90,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    MT_IPC_COUNT,
    //function
    oled
};
#endif

#define MT_PAGES_COUNT                  8
#define MT_SIZE_X                       64
#define MT_SIZE_Y                       128

#if (X_MIRROR)
#define X_TRANSFORM(x, size)            (MT_SIZE_X - (x) - (size))
#define PAGE_TRANSFORM(page)            (MT_PAGES_COUNT - (page) - 1)
#define BIT_OUT_TRANSFORM(byte)         (byte)
#else
#define X_TRANSFORM(x, size)            (x)
#define PAGE_TRANSFORM(page)            (page)
#define BIT_OUT_TRANSFORM(byte)         (mt_bitswap(byte))
#endif

unsigned char _ssd1306buffer[1024];


uint8_t mt_bitswap(uint8_t x){
    x = ((x & 0x55) << 1) | ((x & 0xAA) >> 1);
    x = ((x & 0x33) << 2) | ((x & 0xCC) >> 2);
    x = ((x & 0x0F) << 4) | ((x & 0xF0) >> 4);
    return x;
}
static void delay_clks(unsigned int clks){
    int i;
    for (i = 0; i < clks; ++i)
        __NOP();
}

void oled_set_backlight(bool on); // OLED haven`t

void oled_cls(){
/*    ssd1306SendCommand(0x00 | 0x0);  // low col = 0
    ssd1306SendCommand(0x10 | 0x0);  // hi col = 0
    ssd1306SendCommand(0x40 | 0x0); // line #0
*/
  unsigned short i;
  for (i=0; i<1024; i++)  {
//      ssd1306SendData(0);
    _ssd1306buffer[i] = 77;
  }
}

void oled_reset(){
    gpio_set_pin(SSD1306_RST_PORT);
    delay_clks(TRI);
    gpio_reset_pin(SSD1306_RST_PORT);
    delay_clks(TR);
    gpio_set_pin(SSD1306_RST_PORT);
}

void oled_show(bool on){
    if (on){
//	ssd1306SendCommand(0xAF);	//SSD1306_DISPLAYON
	display_status = true;
    }else{
//	ssd1306SendCommand(0xAE);	//SSD1306_DISPLAYOFF
	display_status = false;
    }
}

bool oled_is_on(){
    return display_status;
}

#if (MT_TEST)
void mt_clks_test(){
//    TIME uptime;
//    get_uptime(&uptime);
//    delay_clks(CLKS_TEST_ROUNDS);
//    printf("clks average time: %dns\n\r", time_elapsed_us(&uptime) * 1000 / CLKS_TEST_ROUNDS);
}

static inline void mt_set_pixel(unsigned int x, unsigned int y, bool set){
    uint8_t data;
    unsigned int cs, page, xr, yr;
    if (x >= MT_SIZE_X || y >= MT_SIZE_Y){
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    //find page & CS
    xr = X_TRANSFORM(x, 1);
    page = xr >> 3;
//    if (y >= MT_SIZE_X) {
//        yr = y - MT_SIZE_X;
//        cs = MT_CS2;
//    } else   {
//        yr = y;
//        cs = MT_CS1;
//    }
//    mt_cmd(cs, MT_CMD_SET_PAGE | page);
//    mt_cmd(cs, MT_CMD_SET_ADDRESS | yr);
//    mt_datain(cs);
//    data = mt_datain(cs);
    if (set)
        data |= 1 << (xr & 7);
    else
        data &= ~(1 << (xr & 7));
//    mt_cmd(cs, MT_CMD_SET_ADDRESS | yr);
//    mt_dataout(cs, data);
}

static void mt_poly_test(unsigned int y, unsigned int size){
    int i, j;
    for (i = 0; i < MT_SIZE_X; ++i){
        for (j = y; j < y + size; ++j)
            mt_set_pixel(i, j, true);
    }
}

void mt_pixel_test(){
    unsigned int off, sz;
    off = 0;
    sz = 8;
    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 6;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 5;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 4;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 3;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 2;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 1;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
}
#endif //MT_TEST

//here goes in each chip coords
static void oled_clear_rect_cs(unsigned int cs, RECT* rect){}

void oled_clear_rect(RECT* rect){}

//here goes in each chip coords
static void oled_read_rect_cs(unsigned int cs, RECT* rect, uint8_t* data, unsigned int bpl, unsigned int offset){}

#if (MT_DRIVER)
void oled(){
    bool need_post;
    IPC ipc;
//    oled_init();
#if (SYS_INFO) || (MT_TEST)
    open_stdout();
#endif
    for (;;){
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        switch (ipc.cmd) {
        case IPC_PING:
            need_post = true;
            break;
//        case IPC_CALL_ERROR:
//            break;
#if (SYS_INFO)
        case IPC_GET_INFO:
//            oled_info(drv);
            need_post = true;
            break;
#endif
        case MT_RESET:
            oled_reset();
            need_post = true;
            break;
        case MT_SHOW:
            oled_show(ipc.param1);
            need_post = true;
            break;
        case MT_BACKLIGHT:
//	    oled_set_backlight(ipc.param1);
            need_post = true;
            break;
        case MT_CLS:
            oled_cls();
            need_post = true;
            break;
        case MT_CLEAR_RECT:
//            mt_clear_rect_driver(ipc.process);
            need_post = true;
            break;
        case MT_WRITE_RECT:
//            mt_write_rect_driver(ipc.process);
            need_post = true;
            break;
        case MT_READ_CANVAS:
//            mt_read_canvas_driver((HANDLE)ipc.param1, ipc.param2, ipc.param3);
            need_post = true;
            break;
        case MT_WRITE_CANVAS:
//            mt_write_canvas_driver((HANDLE)ipc.param1, ipc.param2, ipc.param3);
            need_post = true;
            break;
#if (MT_TEST)
        case MT_PIXEL_TEST:
//            mt_pixel_test();
            need_post = true;
            break;
#endif
        default:
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}

#endif
