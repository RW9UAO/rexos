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

//bool display_status = false;

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

//unsigned char _ssd1306buffer[1024];

//=======================================================================
void ssd1306SendByte(unsigned char byte){
    unsigned char i;
      // Write from MSB to LSB
      for (i=0; i<8; i++){
        // Set clock pin low
	  gpio_reset_pin(SSD1306_SCLK_PORT);
        // Set data pin high or low depending on the value of the current bit
        if( byte & 0x80 )
    	gpio_set_pin(SSD1306_SDAT_PORT);
        else
    	gpio_reset_pin(SSD1306_SDAT_PORT);
        byte = byte << 1;
        // Set clock pin high
        gpio_set_pin(SSD1306_SCLK_PORT);
      }
}
//=======================================================================
void ssd1306SendCommand(unsigned char byte){

  // Make sure clock pin starts high
    gpio_set_pin( SSD1306_SCLK_PORT);

    gpio_reset_pin( SSD1306_CS_PORT);			// set SC

    gpio_reset_pin( SSD1306_DC_PORT);		// command mode

    ssd1306SendByte(byte);

    gpio_set_pin( SSD1306_CS_PORT);			// clr SC
}
//=======================================================================
void ssd1306SendData(unsigned char byte){

  // Make sure clock pin starts high
    gpio_set_pin( SSD1306_SCLK_PORT);

    gpio_reset_pin( SSD1306_CS_PORT);			// set SC

    gpio_set_pin( SSD1306_DC_PORT);		// data mode

    ssd1306SendByte(byte);

    gpio_set_pin( SSD1306_CS_PORT);			// clr SC
}
//=========================================================================



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
/*
    //horizontal addr mode
    ssd1306SendCommand(0x00 | 0x0);  // low col = 0
    ssd1306SendCommand(0x10 | 0x0);  // hi col = 0
    ssd1306SendCommand(0x40 | 0x0); // line #0

  unsigned short i;
  for (i=0; i<1024; i++)  {
      ssd1306SendData(i);
  }*/
    unsigned char page, col;
    for(page = 0; page < 8; page++){
	ssd1306SendCommand(0xB0 | page);  // Set the page start address of the target display location by command B0h to B7h
	ssd1306SendCommand(0x00);	// Set the lower start column address of pointer by command 00h~0Fh.
	ssd1306SendCommand(0x10);	//  Set the upper start column address of pointer by command 10h~1Fh.
	for(col = 0; col < 128; col++){
    	    ssd1306SendData(0);
	}
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
	ssd1306SendCommand(0xAF);	//SSD1306_DISPLAYON
//	display_status = true;
    }else{
	ssd1306SendCommand(0xAE);	//SSD1306_DISPLAYOFF
//	display_status = false;
    }
}

bool oled_is_on(){
    return true;//display_status;
}

void oled_init(void){
    gpio_enable_pin(SSD1306_RST_PORT, E_IO_OUTPUT);
    gpio_enable_pin(SSD1306_DC_PORT, E_IO_OUTPUT);
    gpio_enable_pin(SSD1306_CS_PORT, E_IO_OUTPUT);
    gpio_enable_pin(SSD1306_SCLK_PORT, E_IO_OUTPUT);
    gpio_enable_pin(SSD1306_SDAT_PORT, E_IO_OUTPUT);

    oled_reset();

    ssd1306SendCommand(0xAE);	//SSD1306_DISPLAYOFF

    ssd1306SendCommand(0xD5);	//SSD1306_SETDISPLAYCLOCKDIV
    ssd1306SendCommand(0x80);

    ssd1306SendCommand(0xA8);	//SSD1306_SETMULTIPLEX
    ssd1306SendCommand(0x3F);

    ssd1306SendCommand(0xD3);	//SSD1306_SETDISPLAYOFFSET
    ssd1306SendCommand(0x00);

    ssd1306SendCommand(0x40);

    ssd1306SendCommand(0x8D);	//SSD1306_CHARGEPUMP
    ssd1306SendCommand(0x14);

    ssd1306SendCommand(0xA1);
    ssd1306SendCommand(0xC8);

    ssd1306SendCommand(0xD1);
    ssd1306SendCommand(0x12);

    ssd1306SendCommand(0x81);	//SSD1306_SETCONTRAST
    ssd1306SendCommand(0xCF);

    ssd1306SendCommand(0xD9);	//SSD1306_SETPRECHARGE
    ssd1306SendCommand(0xF1);

    ssd1306SendCommand(0xDB);	//SSD1306_SETVCOMDETECT
    ssd1306SendCommand(0x40);

    ssd1306SendCommand(0xA4);	//SSD1306_DISPLAYALLON_RESUME
    ssd1306SendCommand(0xA6);	//SSD1306_NORMALDISPLAY

    ssd1306SendCommand(0x20);	//Set Memory Addressing Mode
//    ssd1306SendCommand(0x00);	//Horizontal Addressing Mode
    ssd1306SendCommand(0x02);	//Page Addressing Mode

    oled_show(true);
    oled_cls();
}
//=================================================================================
// x: 0-127 in pixel
// y: 0-7 in display string num
void ssd1306DrawChar(unsigned char x, unsigned char y, unsigned char c){
    unsigned char col, column[font_u8Width];

  // Check if the requested character is available
  if ((c >= font_u8FirstChar) && (c <= font_u8LastChar)) {
    // Retrieve appropriate columns from font data
    for (col = 0; col < font_u8Width; col++)    {
      column[col] = au8FontSystem5x8[((c - 32) * font_u8Width) + col];    // Get first column of appropriate character
    }
  }  else  {
    // Requested character is not available in this font ... send a space instead
    for (col = 0; col < font_u8Width; col++)    {
      column[col] = 0x00;    // Send solid space
    }
  }

    ssd1306SendCommand(0xB0 | y);// set page
    ssd1306SendCommand(0x00 | (x & 0x0F) );	// Set the lower start column address of pointer by command 00h~0Fh.
    ssd1306SendCommand(0x10 | (x >>4) );	//  Set the upper start column address of pointer by command 10h~1Fh.
  // Render each column
  unsigned char xoffset;
  for (xoffset = 0; xoffset < font_u8Width; xoffset++)  {
    	    ssd1306SendData(column[xoffset]);
  }
}
//=================================================================================
void ssd1306DrawString(unsigned char x, unsigned char y, char* text){
    unsigned char l = 0;
  do{
    ssd1306DrawChar(x + (l * (font_u8Width + 1)), y, text[l]);
    l++;
  }while(text[l]);
}
//===================================================================================

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
    oled_init();
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
