/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_eth.h"
#include "stm32_config.h"
#include "../../userspace/stdio.h"
#include "../../userspace/ipc.h"
#include "../../userspace/irq.h"
#include "../../userspace/sys.h"
#include "../../userspace/direct.h"
#include "../../userspace/timer.h"
#include "../../userspace/stm32_driver.h"
#include "../../userspace/file.h"
#include "../eth_phy.h"
#include "stm32_gpio.h"
#include "stm32_power.h"
#include <stdbool.h>
#include <string.h>

#define _printd                 printf
#define get_clock               stm32_power_get_clock_outside
#define ack_gpio                stm32_core_request_outside
#define ack_power               stm32_core_request_outside

#define STM32_ETH_MDC           C1
#define STM32_ETH_MDIO          A2
#define STM32_ETH_TX_CLK        C3
#define STM32_ETH_TX_EN         B11
#define STM32_ETH_TX_D0         B12
#define STM32_ETH_TX_D1         B13
#define STM32_ETH_TX_D2         C2
#define STM32_ETH_TX_D3         B8

#define STM32_ETH_RX_CLK        A1
#define STM32_ETH_RX_ER         B10

#if (STM32_ETH_REMAP)
#define STM32_ETH_RX_DV         D8
#define STM32_ETH_RX_D0         D9
#define STM32_ETH_RX_D1         D10
#define STM32_ETH_RX_D2         D11
#define STM32_ETH_RX_D3         D12
#else
#define STM32_ETH_RX_DV         A7
#define STM32_ETH_RX_D0         C4
#define STM32_ETH_RX_D1         C5
#define STM32_ETH_RX_D2         B0
#define STM32_ETH_RX_D3         B1
#endif

#define STM32_ETH_PPS_OUT       B5
#define STM32_ETH_COL           A3
#define STM32_ETH_CRS_WKUP      A0

void stm32_eth();

const REX __STM32_ETH = {
    //name
    "STM32 ETH",
    //size
    STM32_ETH_PROCESS_SIZE,
    //priority - driver priority
    91,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    STM32_ETH_IPC_COUNT,
    //function
    stm32_eth
};

void eth_phy_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
{
    while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
    ETH->MACMIIAR &= ~(ETH_MACMIIAR_PA | ETH_MACMIIAR_MR);
    ETH->MACMIIAR |= ((phy_addr & 0x1f) << 11) | ((reg_addr & 0x1f) << 6) | ETH_MACMIIAR_MW;
    ETH->MACMIIDR = data & ETH_MACMIIDR_MD;
    ETH->MACMIIAR |= ETH_MACMIIAR_MB;
    while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
}

uint16_t eth_phy_read(uint8_t phy_addr, uint8_t reg_addr)
{
    while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
    ETH->MACMIIAR &= ~(ETH_MACMIIAR_PA | ETH_MACMIIAR_MR | ETH_MACMIIAR_MW);
    ETH->MACMIIAR |= ((phy_addr & 0x1f) << 11) | ((reg_addr & 0x1f) << 6);
    ETH->MACMIIAR |= ETH_MACMIIAR_MB;
    while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
    return ETH->MACMIIDR & ETH_MACMIIDR_MD;
}

static void stm32_eth_flush(ETH_DRV* drv)
{
    HANDLE block;

    //flush TxFIFO controller
    ETH->DMAOMR |= ETH_DMAOMR_FTF;
    while(ETH->DMAOMR & ETH_DMAOMR_FTF) {}
#if (ETH_DOUBLE_BUFFERING)
    int i;
    for (i = 0; i < 2; ++i)
    {
        __disable_irq();
        drv->rx_des[i].ctl = 0;
        block = drv->rx_block[i];
        drv->rx_block[i] = INVALID_HANDLE;
        __enable_irq();
        if (block != INVALID_HANDLE)
            fread_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_FILE_IO_CANCELLED);

        __disable_irq();
        drv->tx_des[i].ctl = 0;
        block = drv->tx_block[i];
        drv->tx_block[i] = INVALID_HANDLE;
        __enable_irq();
        if (block != INVALID_HANDLE)
            fwrite_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_FILE_IO_CANCELLED);
    }
    drv->cur_rx = (ETH->DMACHRDR == (unsigned int)(&drv->rx_des[0]) ? 0 : 1);
    drv->cur_tx = (ETH->DMACHTDR == (unsigned int)(&drv->tx_des[0]) ? 0 : 1);
#else
    __disable_irq();
    block = drv->rx_block;
    drv->rx_block = INVALID_HANDLE;
    __enable_irq();
    if (block != INVALID_HANDLE)
        fread_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_FILE_IO_CANCELLED);

    __disable_irq();
    block = drv->tx_block;
    drv->tx_block = INVALID_HANDLE;
    __enable_irq();
    if (block != INVALID_HANDLE)
        fwrite_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_FILE_IO_CANCELLED);
#endif
}

static void stm32_eth_conn_check(ETH_DRV* drv)
{
    ETH_CONN_TYPE new_conn;
    new_conn = eth_phy_get_conn_status(ETH_PHY_ADDRESS);
    if (new_conn != drv->conn)
    {
        drv->conn = new_conn;
        drv->connected = ((drv->conn != ETH_NO_LINK) && (drv->conn != ETH_REMOTE_FAULT));
        ipc_post_inline(drv->tcpip, ETH_NOTIFY_LINK_CHANGED, HAL_HANDLE(HAL_ETH, 0), drv->conn, 0);
        if (drv->connected)
        {
            //set speed and duplex
            switch (drv->conn)
            {
            case ETH_10_HALF:
                ETH->MACCR &= ~(ETH_MACCR_FES | ETH_MACCR_DM);
                break;
            case ETH_10_FULL:
                ETH->MACCR &= ~ETH_MACCR_FES;
                ETH->MACCR |= ETH_MACCR_DM;
                break;
            case ETH_100_HALF:
                ETH->MACCR &= ~(ETH_MACCR_DM);
                ETH->MACCR |= ETH_MACCR_FES;
                break;
            case ETH_100_FULL:
                ETH->MACCR |= ETH_MACCR_FES | ETH_MACCR_DM;
                break;
            default:
                break;
            }
            //enable RX/TX and DMA operations
            ETH->MACCR |= ETH_MACCR_TE | ETH_MACCR_RE;
            ETH->DMAOMR |= ETH_DMAOMR_SR | ETH_DMAOMR_ST;
        }
        else
        {
            stm32_eth_flush(drv);
            ETH->MACCR &= ~(ETH_MACCR_TE | ETH_MACCR_RE);
            ETH->DMAOMR &= ~(ETH_DMAOMR_SR | ETH_DMAOMR_ST);
        }
    }
    timer_start_ms(drv->timer, 1000, 0);
}

void stm32_eth_isr(int vector, void* param)
{
    int i;
    uint32_t sta;
    ETH_DRV* drv = (ETH_DRV*)param;
    sta = ETH->DMASR;
    if (sta & ETH_DMASR_RS)
    {
#if (ETH_DOUBLE_BUFFERING)
        for (i = 0; i < 2; ++i)
        {
            if ((drv->rx_block[drv->cur_rx] != INVALID_HANDLE) && ((drv->rx_des[drv->cur_rx].ctl & ETH_RDES_OWN) == 0))
            {
                firead_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), drv->rx_block[drv->cur_rx], (drv->rx_des[drv->cur_rx].ctl & ETH_RDES_FL_MASK) >> ETH_RDES_FL_POS);
                drv->rx_block[drv->cur_rx] = INVALID_HANDLE;
                drv->cur_rx = (drv->cur_rx + 1) & 1;
            }
            else
                break;
        }
#else
        if (drv->rx_block != INVALID_HANDLE)
        {
            firead_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), drv->rx_block, (drv->rx_des.ctl & ETH_RDES_FL_MASK) >> ETH_RDES_FL_POS);
            drv->rx_block = INVALID_HANDLE;
        }
#endif
        ETH->DMASR = ETH_DMASR_RS;
    }
    if (sta & ETH_DMASR_TS)
    {
#if (ETH_DOUBLE_BUFFERING)
        for (i = 0; i < 2; ++i)
        {
            if ((drv->tx_block[drv->cur_tx] != INVALID_HANDLE) && ((drv->tx_des[drv->cur_tx].ctl & ETH_TDES_OWN) == 0))
            {
                fiwrite_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), drv->tx_block[drv->cur_tx], (drv->tx_des[drv->cur_tx].size & ETH_TDES_TBS1_MASK) >> ETH_TDES_TBS1_POS);
                drv->tx_block[drv->cur_tx] = INVALID_HANDLE;
                drv->cur_tx = (drv->cur_tx + 1) & 1;
            }
            else
                break;
        }
#else
        if (drv->tx_block != INVALID_HANDLE)
        {
            fiwrite_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), drv->tx_block, (drv->tx_des.size & ETH_TDES_TBS1_MASK) >> ETH_TDES_TBS1_POS);
            drv->tx_block = INVALID_HANDLE;
        }
#endif
        ETH->DMASR = ETH_DMASR_TS;
    }
    ETH->DMASR = ETH_DMASR_NIS;
}

static void stm32_eth_close(ETH_DRV* drv)
{
    //disable interrupts
    NVIC_DisableIRQ(ETH_IRQn);
    irq_unregister(ETH_IRQn);

    //flush
    stm32_eth_flush(drv);

    //turn phy off
    eth_phy_power_off(ETH_PHY_ADDRESS);

    //disable clocks
    RCC->AHBENR &= ~(RCC_AHBENR_ETHMACEN | RCC_AHBENR_ETHMACTXEN | RCC_AHBENR_ETHMACRXEN);

    //disable pins
#if (STM32_ETH_REMAP)
    AFIO->MAPR &= ~AFIO_MAPR_ETH_REMAP;
#endif
#if (STM32_ETH_PPS_OUT_ENABLE)
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_PPS_OUT, 0, 0);
#endif

    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_MDC, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_MDIO, 0, 0);

    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_TX_CLK, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_TX_EN, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_TX_D0, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_TX_D1, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_TX_D2, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_TX_D3, 0, 0);

    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_RX_CLK, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_RX_DV, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_RX_ER, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_RX_D0, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_RX_D1, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_RX_D2, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, STM32_ETH_RX_D3, 0, 0);

    //destroy timer
    timer_destroy(drv->timer);
    drv->timer = INVALID_HANDLE;

    //switch to unconfigured state
    drv->tcpip = INVALID_HANDLE;
    drv->connected = false;
    drv->conn = ETH_NO_LINK;
}

static inline void stm32_eth_open(ETH_DRV* drv, ETH_CONN_TYPE conn, HANDLE tcpip)
{
    unsigned int clock;

    drv->timer = timer_create(HAL_HANDLE(HAL_ETH, 0));
    if (drv->timer == INVALID_HANDLE)
        return;
    drv->tcpip = tcpip;
    //enable pins
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_MDC, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_MDIO, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);

    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_CLK, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_EN, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_D0, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_D1, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_D2, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_D3, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);

    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_CLK, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_DV, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_ER, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_D0, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_D1, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_D2, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_D3, STM32_GPIO_MODE_INPUT_FLOAT, false);

#if (STM32_ETH_REMAP)
    AFIO->MAPR |= AFIO_MAPR_ETH_REMAP;
#endif
#if (STM32_ETH_PPS_OUT_ENABLE)
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_PPS_OUT, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
#endif
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_COL, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_CRS_WKUP, STM32_GPIO_MODE_INPUT_FLOAT, false);

    //enable clocks
    RCC->AHBENR |= RCC_AHBENR_ETHMACEN | RCC_AHBENR_ETHMACTXEN | RCC_AHBENR_ETHMACRXEN;

    //reset DMA
    ETH->DMABMR |= ETH_DMABMR_SR;
    while(ETH->DMABMR & ETH_DMABMR_SR) {}

    //setup DMA
#if (ETH_DOUBLE_BUFFERING)
    drv->rx_des[0].ctl = 0;
    drv->rx_des[0].size = ETH_RDES_RCH;
    drv->rx_des[0].buf2_ndes = &drv->rx_des[1];
    drv->rx_des[1].ctl = 0;
    drv->rx_des[1].size = ETH_RDES_RCH;
    drv->rx_des[1].buf2_ndes = &drv->rx_des[0];

    drv->tx_des[0].ctl = ETH_TDES_TCH | ETH_TDES_IC;
    drv->tx_des[0].buf2_ndes = &drv->tx_des[1];
    drv->tx_des[1].ctl = ETH_TDES_TCH | ETH_TDES_IC;
    drv->tx_des[1].buf2_ndes = &drv->tx_des[0];

    drv->cur_rx = drv->cur_tx = 0;
#else
    drv->rx_des.ctl = 0;
    drv->rx_des.size = ETH_RDES_RCH;
    drv->rx_des.buf2_ndes = &drv->rx_des;
    drv->tx_des.ctl = ETH_TDES_TCH;
    drv->tx_des.buf2_ndes = &drv->tx_des;
#endif
    ETH->DMATDLAR = (unsigned int)&drv->tx_des;
    ETH->DMARDLAR = (unsigned int)&drv->rx_des;

    //disable receiver/transmitter before link established
    ETH->MACCR = 0x8000;
    //setup MAC
    ETH->MACA0HR = (drv->mac.u8[5] << 8) | (drv->mac.u8[4] << 0) |  (1 << 31);
    ETH->MACA0LR = (drv->mac.u8[3] << 24) | (drv->mac.u8[2] << 16) | (drv->mac.u8[1] << 8) | (drv->mac.u8[0] << 0);
    //apply MAC unicast filter
#if (TCPIP_MAC_FILTER)
    ETH->MACFFR = ETH_MACFFR_RA;
#else
    ETH->MACFFR = 0;
#endif

    //configure clock for SMI
    clock = get_clock(drv, STM32_CLOCK_AHB);
    if (clock <= 35000000)
        ETH->MACMIIAR = ETH_MACMIIAR_CR_Div16;
    else if (clock <= 60000000)
        ETH->MACMIIAR = ETH_MACMIIAR_CR_Div26;
    else
        ETH->MACMIIAR = ETH_MACMIIAR_CR_Div42;

    //enable dma interrupts
    irq_register(ETH_IRQn, stm32_eth_isr, (void*)drv);
    NVIC_EnableIRQ(ETH_IRQn);
    NVIC_SetPriority(ETH_IRQn, 13);
    ETH->DMAIER = ETH_DMAIER_NISE | ETH_DMAIER_RIE | ETH_DMAIER_TIE;

    //turn phy on
    if (!eth_phy_power_on(ETH_PHY_ADDRESS, conn))
    {
        error(ERROR_NOT_FOUND);
        stm32_eth_close(drv);
        return;
    }

    stm32_eth_conn_check(drv);
}

static inline void stm32_eth_read(ETH_DRV* drv, HANDLE block, unsigned int size)
{
    if (!drv->connected)
    {
        fread_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_NOT_ACTIVE);
        return;
    }
#if (ETH_DOUBLE_BUFFERING)
    int i = -1;
    uint8_t cur_rx = drv->cur_rx;

    if (drv->rx_block[cur_rx] == INVALID_HANDLE)
        i = cur_rx;
    else if (drv->rx_block[(cur_rx + 1) & 1] == INVALID_HANDLE)
        i = (cur_rx + 1) & 1;
    if (i < 0)
    {
        fread_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_IN_PROGRESS);
        return;
    }
    drv->rx_des[i].buf1 = block_open(block);
    if (drv->rx_des[i].buf1 == NULL)
    {
        fread_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, get_last_error());
        return;
    }
    drv->rx_des[i].size &= ~ETH_RDES_RBS1_MASK;
    drv->rx_des[i].size |= ((size << ETH_RDES_RBS1_POS) & ETH_RDES_RBS1_MASK);
    __disable_irq();
    drv->rx_block[i] = block;
    //give descriptor to DMA
    drv->rx_des[i].ctl = ETH_RDES_OWN;
    __enable_irq();
#else
    if (drv->rx_block != INVALID_HANDLE)
    {
        fread_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_IN_PROGRESS);
        return;
    }
    drv->rx_des.buf1 = block_open(block);
    if (drv->rx_des.buf1 == NULL)
    {
        fread_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, get_last_error());
        return;
    }
    drv->rx_block = block;
    drv->rx_des.size &= ~ETH_RDES_RBS1_MASK;
    drv->rx_des.size |= ((size << ETH_RDES_RBS1_POS) & ETH_RDES_RBS1_MASK);
    //give descriptor to DMA
    drv->rx_des.ctl = ETH_RDES_OWN;
#endif
    //enable and poll DMA. Value is doesn't matter
    ETH->DMARPDR = 0;
}

static inline void stm32_eth_write(ETH_DRV* drv, HANDLE block, unsigned int size)
{
    if (!drv->connected)
    {
        fwrite_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_NOT_ACTIVE);
        return;
    }
#if (ETH_DOUBLE_BUFFERING)
    int i = -1;
    uint8_t cur_tx = drv->cur_tx;

    if (drv->tx_block[cur_tx] == INVALID_HANDLE)
        i = cur_tx;
    else if (drv->tx_block[(cur_tx + 1) & 1] == INVALID_HANDLE)
        i = (cur_tx + 1) & 1;
    if (i < 0)
    {
        fwrite_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_IN_PROGRESS);
        return;
    }
    drv->tx_des[i].buf1 = block_open(block);
    if (drv->tx_des[i].buf1 == NULL)
    {
        fwrite_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, get_last_error());
        return;
    }
    drv->tx_des[i].size = ((size << ETH_TDES_TBS1_POS) & ETH_TDES_TBS1_MASK);
    drv->tx_des[i].ctl = ETH_TDES_TCH | ETH_TDES_FS | ETH_TDES_LS | ETH_TDES_IC;
    __disable_irq();
    drv->tx_block[i] = block;
    //give descriptor to DMA
    drv->tx_des[i].ctl |= ETH_TDES_OWN;
    __enable_irq();
#else
    if (drv->tx_block != INVALID_HANDLE)
    {
        fwrite_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, ERROR_IN_PROGRESS);
        return;
    }
    drv->tx_des.buf1 = block_open(block);
    if (drv->tx_des.buf1 == NULL)
    {
        fwrite_complete(drv->tcpip, HAL_HANDLE(HAL_ETH, 0), block, get_last_error());
        return;
    }
    drv->tx_block = block;
    drv->tx_des.size = ((size << ETH_TDES_TBS1_POS) & ETH_TDES_TBS1_MASK);
    //give descriptor to DMA
    drv->tx_des.ctl = ETH_TDES_TCH | ETH_TDES_FS | ETH_TDES_LS | ETH_TDES_IC;
    drv->tx_des.ctl |= ETH_TDES_OWN;
#endif
    //enable and poll DMA. Value is doesn't matter
    ETH->DMATPDR = 0;
}

static inline void stm32_eth_set_mac(ETH_DRV* drv, unsigned int param1, unsigned int param2)
{
    drv->mac.u32.hi = param1;
    drv->mac.u32.lo = (uint16_t)param2;
}

static inline void stm32_eth_get_mac(ETH_DRV* drv, IPC* ipc)
{
    ipc->param1 = drv->mac.u32.hi;
    ipc->param2 = drv->mac.u32.lo;
}

void stm32_eth_init(ETH_DRV* drv)
{
    drv->tcpip = INVALID_HANDLE;
    drv->conn = ETH_NO_LINK;
    drv->connected = false;
    drv->mac.u32.hi = drv->mac.u32.lo = 0;
#if (ETH_DOUBLE_BUFFERING)
    drv->rx_block[0] = drv->tx_block[0] = INVALID_HANDLE;
    drv->rx_block[1] = drv->tx_block[1] = INVALID_HANDLE;
#else
    drv->rx_block = drv->tx_block = INVALID_HANDLE;
#endif
}

#if (SYS_INFO)
static inline void stm32_eth_info(ETH_DRV* drv)
{
    int i;
    printf("STM32 ETH driver info\n\r");
    printf("Link status: %s\n\r", drv->connected ? "Active" : "No cable");
    printf("MAC: ");
    for (i = 0; i < MAC_SIZE; ++i)
    {
        printf("%02X", drv->mac.u8[i]);
        if (i < MAC_SIZE - 1)
            printf(":");
    }
    printf("\n\r");
}
#endif

bool stm32_eth_request(ETH_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        stm32_eth_info(drv);
        need_post = true;
        break;
#endif
    case IPC_OPEN:
        stm32_eth_open(drv, ipc->param2, ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        stm32_eth_close(drv);
        need_post = true;
        break;
    case IPC_FLUSH:
        stm32_eth_flush(drv);
        need_post = true;
        break;
    case IPC_READ:
        if ((int)ipc->param3 < 0)
            break;
        stm32_eth_read(drv, ipc->param2, ipc->param3);
        //generally posted with block, no return IPC
        break;
    case IPC_WRITE:
        if ((int)ipc->param3 < 0)
            break;
        stm32_eth_write(drv, ipc->param2, ipc->param3);
        //generally posted with block, no return IPC
        break;
    case IPC_TIMEOUT:
        stm32_eth_conn_check(drv);
        break;
    case ETH_SET_MAC:
        stm32_eth_set_mac(drv, ipc->param1, ipc->param2);
        need_post = true;
        break;
    case ETH_GET_MAC:
        stm32_eth_get_mac(drv, ipc);
        need_post = true;
        break;
    default:
        break;
    }
    return need_post;
}

void stm32_eth()
{
    IPC ipc;
    ETH_DRV drv;
    bool need_post;
    stm32_eth_init(&drv);
#if (SYS_INFO)
    open_stdout();
#endif
    object_set_self(SYS_OBJ_ETH);
    for (;;)
    {
        error(ERROR_OK);

        need_post = false;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        default:
            need_post = stm32_eth_request(&drv, &ipc);
            break;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
