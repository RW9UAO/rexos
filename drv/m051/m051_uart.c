#include "DrvUART.h"
#include "m051_uart.h"
#include "../../userspace/sys.h"
#include "error.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stream.h"
#include "../../userspace/direct.h"
#include "../../userspace/irq.h"
#include "../../userspace/object.h"
#include <string.h>
#if (MONOLITH_UART)
#include "m051_core_private.h"

#else

void m051_uart();

const REX __M051_UART = {
    //name
    "M051 uart",
    //size
    M051_UART_STACK_SIZE,
    //priority - driver priority. Setting priority lower than other drivers can cause IPC overflow on SYS_INFO
    89,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    M051_DRIVERS_IPC_COUNT,
    //function
    m051_uart
};
#endif

static const unsigned int UART_VECTORS[UARTS_COUNT] =       {UART0_IRQn, UART1_IRQn};

//--------------------------------------------------------------------------
void m051_uart_on_isr(int vector, void* param){
    IPC ipc;
    UART_T * tUART;
    UART_PORT port;
    SHARED_UART_DRV* drv = (SHARED_UART_DRV*)param;
    unsigned int IntStatus = 0;
    //find port by vector
    if(vector == UART_VECTORS[0]){
	 port = 0;
	IntStatus = inpw(&UART0->ISR);
    }
    if(vector == UART_VECTORS[1]){
        port = 1;
	IntStatus = inpw(&UART1->ISR);
    }
    tUART = (UART_T *)((uint32_t)UART0 + port);

    unsigned int temp = tUART->DATA;

    //transmit more
    if (tUART->ISR.THRE_INT && tUART->FSR.TX_EMPTY && drv->uart.uarts[port]->tx_chunk_size){
	tUART->DATA = drv->uart.uarts[port]->tx_buf[drv->uart.uarts[port]->tx_chunk_pos++];
        //no more
        if (drv->uart.uarts[port]->tx_chunk_pos >= drv->uart.uarts[port]->tx_chunk_size){
            drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;
            ipc.process = process_iget_current();
            ipc.cmd = IPC_UART_ISR_TX;
            ipc.param1 = HAL_HANDLE(HAL_UART, port);
            ipc.param3 = 0;
            ipc_ipost(&ipc);
	    tUART->IER.THRE_IEN = 0;//disable TX interrupt
        }
    }
    //transmission completed and no more data. Disable transmitter
//    else if ((UART_REGS[port]->CR1 & USART_CR1_TCIE) && (sr & USART_SR_TC))
//        UART_REGS[port]->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);
    //decode error, if any
//    if ((sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE))) {
    if(IntStatus & 0x04){	//RLS_IF
/*        if (sr & USART_SR_ORE)
            drv->uart.uarts[port]->error = ERROR_OVERFLOW;
        else {
            __REG_RC32(UART_REGS[port]->DR);
            if (sr & USART_SR_FE)
                drv->uart.uarts[port]->error = ERROR_UART_FRAME;
            else if (sr & USART_SR_PE)
                drv->uart.uarts[port]->error = ERROR_UART_PARITY;
            else if  (sr & USART_SR_NE)
                drv->uart.uarts[port]->error = ERROR_UART_NOISE;
        }*/
    	tUART->FCR.RFR =1;
    	tUART->u32FSR |= tUART->u32FSR;
    }
    //receive data
    if(IntStatus & DRVUART_RDAINT){
        ipc.param3 = temp;
        ipc.process = process_iget_current();
        ipc.cmd = IPC_UART_ISR_RX;
        ipc.param1 = HAL_HANDLE(HAL_UART, port);
        ipc_ipost(&ipc);
    }
}
//-------------------------------------------------------------------------
void uart_write_kernel(const char *const buf, unsigned int size, void* param){
    unsigned int i;
    NVIC_DisableIRQ(UART0_IRQn);
    for(i = 0; i < size; ++i){
        while (UART0->FSR.TE_FLAG !=1);
        UART0->DATA = buf[i];
    }
      NVIC_EnableIRQ(UART0_IRQn);
}
//------------------------------------------------------------
void m051_uart_set_baudrate_internal(SHARED_UART_DRV* drv, UART_PORT port, const BAUD* config){
    UART_T * tUART;

    if (port >= UARTS_COUNT){
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->uart.uarts[port] == NULL){
        error(ERROR_NOT_ACTIVE);
        return;
    }

    tUART = (UART_T *)((uint32_t)UART0 + port);

      // Set Parity & Data bits & Stop bits
    if (config->data_bits == 8){
      tUART->LCR.WLS    = DRVUART_DATABITS_8;
    }
    if (config->parity == 'N'){
        tUART->LCR.SPE    = 0; //
        tUART->LCR.EPE    = 0; // even parity
        tUART->LCR.PBE    = 0; // parity enable
    }
//        if (config->parity == 'O')
    if(config->stop_bits == 1){
      tUART->LCR.NSB    = DRVUART_STOPBITS_1;
    }

//      tUART->BAUD.BRD = 415;// for 115200 at 48 MHz
      tUART->BAUD.BRD = 434;// for 115200 at 50 MHz
}
//--------------------------------------------------------------------------
void m051_uart_set_baudrate(SHARED_UART_DRV* drv, UART_PORT port, HANDLE process){
    BAUD baud;
    if (direct_read(process, (void*)&baud, sizeof(BAUD)))
        m051_uart_set_baudrate_internal(drv, port, &baud);
}
//--------------------------------------------------------------------------
void m051_uart_open_internal(SHARED_UART_DRV* drv, UART_PORT port, UART_ENABLE* ue){
    UART_T * tUART;
    if (port >= UARTS_COUNT)    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->uart.uarts[port] != NULL){
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    drv->uart.uarts[port] = malloc(sizeof(UART));
    if (drv->uart.uarts[port] == NULL)    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }
    drv->uart.uarts[port]->tx_pin = ue->tx;
    drv->uart.uarts[port]->rx_pin = ue->rx;
    drv->uart.uarts[port]->error = ERROR_OK;
    drv->uart.uarts[port]->tx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->tx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->rx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->rx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->tx_total = 0;
    drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;

//    if (drv->uart.uarts[port]->tx_pin != PIN_UNUSED)    {
        drv->uart.uarts[port]->tx_stream = stream_create(ue->stream_size);
        if (drv->uart.uarts[port]->tx_stream == INVALID_HANDLE){
            free(drv->uart.uarts[port]);
            drv->uart.uarts[port] = NULL;
            return;
        }
        drv->uart.uarts[port]->tx_handle = stream_open(drv->uart.uarts[port]->tx_stream);
        if (drv->uart.uarts[port]->tx_handle == INVALID_HANDLE){
            stream_destroy(drv->uart.uarts[port]->tx_stream);
            free(drv->uart.uarts[port]);
            drv->uart.uarts[port] = NULL;
            return;
        }
        stream_listen(drv->uart.uarts[port]->tx_stream, (void*)HAL_HANDLE(HAL_UART, port));
//    }
//    if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)    {
        drv->uart.uarts[port]->rx_stream = stream_create(ue->stream_size);
        if (drv->uart.uarts[port]->rx_stream == INVALID_HANDLE){
            stream_close(drv->uart.uarts[port]->tx_handle);
            stream_destroy(drv->uart.uarts[port]->tx_stream);
            free(drv->uart.uarts[port]);
            drv->uart.uarts[port] = NULL;
            return;
        }
        drv->uart.uarts[port]->rx_handle = stream_open(drv->uart.uarts[port]->rx_stream);
        if (drv->uart.uarts[port]->rx_handle == INVALID_HANDLE){
            stream_destroy(drv->uart.uarts[port]->rx_stream);
            stream_close(drv->uart.uarts[port]->tx_handle);
            stream_destroy(drv->uart.uarts[port]->tx_stream);
            free(drv->uart.uarts[port]);
            drv->uart.uarts[port] = NULL;
            return;
        }
        drv->uart.uarts[port]->rx_free = stream_get_free(drv->uart.uarts[port]->rx_stream);
//    }

    //setup pins
    SYSCLK->CLKSEL1.UART_S = 1;	// from PLL
    if(port == 0){
        outpw(&SYS->P3_MFP, (inpw(&SYS->P3_MFP) & ~(0x3<<8)) | (0x3)); 	// set P3.1 and P3.0 as TXD0 and RXD0
	outpw(&SYS->P0_MFP, (inpw(&SYS->P0_MFP) & ~(0x3<<10)) & ~(0x3<<2));	// RTS, CTS not used
        // Reset IP
        SYS->IPRSTC2.UART0_RST = 1;
	SYS->IPRSTC2.UART0_RST = 0;
        // Enable UART clock
	SYSCLK->APBCLK.UART0_EN = 1;
    }else
    if(port == 1){
        outpw(&SYS->P1_MFP, (inpw(&SYS->P1_MFP) | (0x3<<10)) & ~(0x3<<2)); 		// set P1.3 and P1.2 as TXD1 and RXD1
	outpw(&SYS->P0_MFP, (inpw(&SYS->P0_MFP) & ~(0x3<<8)) & ~(0x3<<0));	// RTS, CTS not used
	// Reset IP
        SYS->IPRSTC2.UART1_RST = 1;
	SYS->IPRSTC2.UART1_RST = 0;
        // Enable UART clock
	SYSCLK->APBCLK.UART1_EN = 1;
    }
    tUART = (UART_T *)((uint32_t)UART0 + port);
    tUART->FUNSEL.FUN_SEL = 0;
    // Tx FIFO Reset & Rx FIFO Reset & FIFO Mode Enable
      tUART->FCR.TFR =1;
      tUART->FCR.RFR =1;
      // Set Rx Trigger Level
      tUART->FCR.RFITL = DRVUART_FIFO_1BYTES;
      // Set Time-Out
      tUART->TOR.TOIC    = 0x7F;
      //BAUD
      tUART->BAUD.DIV_X_EN = 1;		// mode 2
      tUART->BAUD.DIV_X_ONE   = 1;

//enable receiver
      tUART->IER.RDA_IEN        = 1; // RX buff has data
      tUART->IER.THRE_IEN        = 0;// TX buff empty
      tUART->IER.RLS_IEN        = 0;
      tUART->IER.MODEM_IEN    = 0;
      tUART->IER.TIME_OUT_EN    = 0;
      tUART->IER.RTO_IEN        = 1;
      tUART->IER.BUF_ERR_IEN    = 1;
      tUART->IER.WAKE_EN        = 0;

    m051_uart_set_baudrate_internal(drv, port, &ue->baud);
    //enable interrupts
    irq_register(UART_VECTORS[port], m051_uart_on_isr, (void*)drv);
    NVIC_EnableIRQ(UART_VECTORS[port]);
    NVIC_SetPriority(UART_VECTORS[port], (1<<__NVIC_PRIO_BITS) - 2);
}
//---------------------------------------------------------------------------------------
void m051_uart_open(SHARED_UART_DRV* drv, UART_PORT port, HANDLE process){
    UART_ENABLE ue;
    if (direct_read(process, (void*)&ue, sizeof(UART_ENABLE)))
        m051_uart_open_internal(drv, port, &ue);
}
//---------------------------------------------------------------------------------------
static inline void m051_uart_close(SHARED_UART_DRV* drv, UART_PORT port){
    UART_T * tUART;
    if (port >= UARTS_COUNT)    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->uart.uarts[port] == NULL)    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    //disable interrupts
    NVIC_DisableIRQ(UART_VECTORS[port]);
    irq_unregister(UART_VECTORS[port]);

    tUART = (UART_T *)((uint32_t)UART0 + port);

    //disable receiver
      tUART->IER.RDA_IEN        = 1; // RX buff has data
    //disable core
    //power down
    //disable pins
        stream_close(drv->uart.uarts[port]->tx_handle);
        stream_destroy(drv->uart.uarts[port]->tx_stream);
//        ack_gpio(drv, M051_GPIO_DISABLE_PIN, drv->uart.uarts[port]->tx_pin, 0, 0);
        stream_close(drv->uart.uarts[port]->rx_handle);
        stream_destroy(drv->uart.uarts[port]->rx_stream);
//        ack_gpio(drv, M051_GPIO_DISABLE_PIN, drv->uart.uarts[port]->rx_pin, 0, 0);

    free(drv->uart.uarts[port]);
    drv->uart.uarts[port] = NULL;
}
//-----------------------------------------------------------------
static inline void m051_uart_flush(SHARED_UART_DRV* drv, UART_PORT port){
    if (port >= UARTS_COUNT)    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->uart.uarts[port] == NULL)    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
	stream_flush(drv->uart.uarts[port]->tx_stream);
        stream_listen(drv->uart.uarts[port]->tx_stream, (void*)HAL_HANDLE(HAL_UART, port));
        drv->uart.uarts[port]->tx_total = 0;
        __disable_irq();
        drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;
        __enable_irq();

        stream_flush(drv->uart.uarts[port]->rx_stream);
    drv->uart.uarts[port]->error = ERROR_OK;
}
//------------------------------------------------------------------------------------------------------------
static inline HANDLE m051_uart_get_tx_stream(SHARED_UART_DRV* drv, UART_PORT port){
    if (port >= UARTS_COUNT){
        error(ERROR_INVALID_PARAMS);
        return INVALID_HANDLE;
    }
    if (drv->uart.uarts[port] == NULL){
        error(ERROR_NOT_ACTIVE);
        return INVALID_HANDLE;
    }
    return drv->uart.uarts[port]->tx_stream;
}
//------------------------------------------------------------------------------------------------------------
static inline HANDLE m051_uart_get_rx_stream(SHARED_UART_DRV* drv, UART_PORT port){
    if (port >= UARTS_COUNT)    {
        error(ERROR_INVALID_PARAMS);
        return INVALID_HANDLE;
    }
    if (drv->uart.uarts[port] == NULL)    {
        error(ERROR_NOT_ACTIVE);
        return INVALID_HANDLE;
    }
//    if (drv->uart.uarts[port]->rx_pin == PIN_UNUSED)    {
//        error(ERROR_NOT_CONFIGURED);
//        return INVALID_HANDLE;
//    }
    return drv->uart.uarts[port]->rx_stream;
}
//------------------------------------------------------------------------------------------------------------
void m051_uart_write(SHARED_UART_DRV* drv, UART_PORT port, unsigned int total){
//    printk("m051_uart_write:\r\n");
    UART_T * tUART;
    tUART = (UART_T *)((uint32_t)UART0 + port);
    if (total)
        drv->uart.uarts[port]->tx_total = total;
    if (drv->uart.uarts[port]->tx_total == 0)
        drv->uart.uarts[port]->tx_total = stream_get_size(drv->uart.uarts[port]->tx_stream);
    if (drv->uart.uarts[port]->tx_total)    {
        unsigned int to_read = drv->uart.uarts[port]->tx_total;
        if (drv->uart.uarts[port]->tx_total > UART_TX_BUF_SIZE)
            to_read = UART_TX_BUF_SIZE;
        if (stream_read(drv->uart.uarts[port]->tx_handle, drv->uart.uarts[port]->tx_buf, to_read))        {
            drv->uart.uarts[port]->tx_chunk_pos = 0;
            drv->uart.uarts[port]->tx_chunk_size = to_read;
            drv->uart.uarts[port]->tx_total -= to_read;
            //start transaction
            tUART->IER.THRE_IEN = 1;
        }
    }
    else
        stream_listen(drv->uart.uarts[port]->tx_stream, (void*)HAL_HANDLE(HAL_UART, port));
}
//------------------------------------------------------------------------------------------------------------
static inline void m051_uart_read(SHARED_UART_DRV* drv, UART_PORT port, char c){
    //caching calls to svc
    if (drv->uart.uarts[port]->rx_free == 0)
        (drv->uart.uarts[port]->rx_free = stream_get_free(drv->uart.uarts[port]->rx_stream));
    //if stream is full, char will be discarded
    if (drv->uart.uarts[port]->rx_free)    {
        stream_write(drv->uart.uarts[port]->rx_handle, &c, 1);
        drv->uart.uarts[port]->rx_free--;
    }
}
//------------------------------------------------------------------------------------------------------------

#if (SYS_INFO) && (UART_STDIO)
//we can't use printf in uart driver, because this can halt driver loop
void printu(const char *const fmt, ...){
    va_list va;
    va_start(va, fmt);
    format(fmt, va, uart_write_kernel, (void*)UART_STDIO_PORT);
    va_end(va);
}
#endif //(SYS_INFO) && (UART_STDIO)
#if (SYS_INFO)
static inline void m051_uart_info(SHARED_UART_DRV* drv){
    int i;
    printu("M051 uart driver info\n\r\n\r");
    printu("uarts count: %d\n\r", UARTS_COUNT);

    for (i = 0; i < UARTS_COUNT; ++i){
        if (drv->uart.uarts[i])
            printu("UART_%d ", i + 1);
    }
    printu("\n\r\n\r");
}
#endif //SYS_INFO

bool m051_uart_request(SHARED_UART_DRV* drv, IPC* ipc){
    bool need_post = false;

//    printk("m051_uart_request, cmd: %#X\r\n", ipc->cmd);

    switch (ipc->cmd){
#if (SYS_INFO)
    case IPC_GET_INFO:
        m051_uart_info(drv);
        need_post = true;
        break;
#endif
    case IPC_OPEN:
        m051_uart_open(drv, HAL_ITEM(ipc->param1), ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
//        M051_uart_close(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_UART_SET_BAUDRATE:
        m051_uart_set_baudrate(drv, HAL_ITEM(ipc->param1), ipc->process);
        need_post = true;
        break;
    case IPC_FLUSH:
//        M051_uart_flush(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_GET_TX_STREAM:
        ipc->param2 = m051_uart_get_tx_stream(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_GET_RX_STREAM:
        ipc->param2 = m051_uart_get_rx_stream(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_UART_GET_LAST_ERROR:
//        ipc->param2 = M051_uart_get_last_error(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_UART_CLEAR_ERROR:
//        M051_uart_clear_error(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_STREAM_WRITE:
    case IPC_UART_ISR_TX:
        m051_uart_write(drv, HAL_ITEM(ipc->param1), ipc->param3);
        //message from kernel (or ISR), no response
        break;
    case IPC_UART_ISR_RX:
        m051_uart_read(drv, HAL_ITEM(ipc->param1), ipc->param3);
        //message from ISR, no response
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

#if (UART_STDIO)
static inline void m051_uart_open_stdio(SHARED_UART_DRV* drv){
    UART_ENABLE ue;
//    ue.tx = UART_STDIO_TX;
//    ue.rx = UART_STDIO_RX;
    ue.baud.data_bits = UART_STDIO_DATA_BITS;
    ue.baud.parity = UART_STDIO_PARITY;
    ue.baud.stop_bits = UART_STDIO_STOP_BITS;
    ue.baud.baud = UART_STDIO_BAUD;
    ue.stream_size = STDIO_STREAM_SIZE;
    m051_uart_open_internal(drv, UART_STDIO_PORT, &ue);

    //setup kernel printk dbg
    setup_dbg(uart_write_kernel, (void*)UART_STDIO_PORT);
    //setup system stdio
    object_set(SYS_OBJ_STDOUT, drv->uart.uarts[UART_STDIO_PORT]->tx_stream);
#if (UART_STDIO_RX != PIN_UNUSED)
    object_set(SYS_OBJ_STDIN, drv->uart.uarts[UART_STDIO_PORT]->rx_stream);
#endif
#if (SYS_INFO) && !(MONOLITH_UART)
    //inform early process (core) about stdio.
    ack(object_get(SYS_OBJ_CORE), IPC_SET_STDIO, 0, 0, 0);
#endif //SYS_INFO && !MONOLITH_UART
}
#endif

void m051_uart_init(SHARED_UART_DRV* drv){
    int i;
    for (i = 0; i < UARTS_COUNT; ++i)
        drv->uart.uarts[i] = NULL;
//#if (UART_STDIO)
    m051_uart_open_stdio(drv);
//#endif //UART_STDIO
}

