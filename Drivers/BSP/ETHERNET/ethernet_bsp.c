/**
 ****************************************************************************************************
 * @file        ethernet.c
 * @author      ԭŶ(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-14
 * @brief       ETHERNET
 * @license     Copyright (c) 2020-2032, ӿƼ޹˾
 * @note        Modified for lwipandRTOS project - removed dependencies on old libraries
 ****************************************************************************************************
 */

#include "ethernet_bsp.h"
#include "lwip_comm.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <string.h>


ETH_HandleTypeDef g_eth_handler;            /* ̫ */
ETH_DMADescTypeDef *g_eth_dma_rx_dscr_tab;  /* ̫DMAݽṹָ */
ETH_DMADescTypeDef *g_eth_dma_tx_dscr_tab;  /* ̫DMAݽṹָ */
uint8_t *g_eth_rx_buf;                      /* ̫ײbuffersָ */
uint8_t *g_eth_tx_buf;                      /* ̫ײbuffersָ */


/**
 * @brief       ̫оƬʼ
 * @param       
 * @retval      0,ɹ
 *              1,ʧ
 */
uint8_t ethernet_init(void)
{
    uint8_t macaddress[6];

    macaddress[0] = g_lwipdev.mac[0];
    macaddress[1] = g_lwipdev.mac[1];
    macaddress[2] = g_lwipdev.mac[2];
    macaddress[3] = g_lwipdev.mac[3];
    macaddress[4] = g_lwipdev.mac[4];
    macaddress[5] = g_lwipdev.mac[5];

    g_eth_handler.Instance = ETH;
    g_eth_handler.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;    /* ʹЭģʽ */
    g_eth_handler.Init.Speed = ETH_SPEED_100M;                          /* ٶ100M,ЭģʽþЧ */
    g_eth_handler.Init.DuplexMode = ETH_MODE_FULLDUPLEX;                /* ȫ˫ģʽЭģʽþЧ */
    g_eth_handler.Init.PhyAddress = ETHERNET_PHY_ADDRESS;               /* ̫оƬĵַ */
    g_eth_handler.Init.MACAddr = macaddress;                            /* MACַ */
    g_eth_handler.Init.RxMode = ETH_RXINTERRUPT_MODE;                   /* жϽģʽ */
    g_eth_handler.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;         /* Ӳ֡У */
    g_eth_handler.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;       /* RMIIӿ */

    if (HAL_ETH_Init(&g_eth_handler) == HAL_OK)
    {
        return 0;   /* ɹ */
    }
    else
    {
        return 1;  /* ʧ */
    }
}

/**
 * @brief       ETHײʱʹܣ
 *    @note     ˺ᱻHAL_ETH_Init()
 * @param       heth:̫
 * @retval      
 */
void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef gpio_init_struct;

    ETH_CLK_GPIO_CLK_ENABLE();          /* ETH_CLKʱ */
    ETH_MDIO_GPIO_CLK_ENABLE();         /* ETH_MDIOʱ */
    ETH_CRS_GPIO_CLK_ENABLE();          /* ETH_CRSʱ */
    ETH_MDC_GPIO_CLK_ENABLE();          /* ETH_MDCʱ */
    ETH_RXD0_GPIO_CLK_ENABLE();         /* ETH_RXD0ʱ */
    ETH_RXD1_GPIO_CLK_ENABLE();         /* ETH_RXD1ʱ */
    ETH_TX_EN_GPIO_CLK_ENABLE();        /* ETH_TX_ENʱ */
    ETH_TXD0_GPIO_CLK_ENABLE();         /* ETH_TXD0ʱ */
    ETH_TXD1_GPIO_CLK_ENABLE();         /* ETH_TXD1ʱ */
    ETH_RESET_GPIO_CLK_ENABLE();        /* ETH_RESETʱ */
    __HAL_RCC_ETH_CLK_ENABLE();         /* ETHʱ */


    /*  RMIIӿ
     * ETH_MDIO -------------------------> PA2
     * ETH_MDC --------------------------> PC1
     * ETH_RMII_REF_CLK------------------> PA1
     * ETH_RMII_CRS_DV ------------------> PA7
     * ETH_RMII_RXD0 --------------------> PC4
     * ETH_RMII_RXD1 --------------------> PC5
     * ETH_RMII_TX_EN -------------------> PG11
     * ETH_RMII_TXD0 --------------------> PG13
     * ETH_RMII_TXD1 --------------------> PG14
     * ETH_RESET-------------------------> PD3
     */

    /* PA1,2,7 */
    gpio_init_struct.Pin = ETH_CLK_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 츴 */
    gpio_init_struct.Pull = GPIO_NOPULL;                    /*  */
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;               /*  */
    gpio_init_struct.Alternate = GPIO_AF11_ETH;             /* ΪETH */
    HAL_GPIO_Init(ETH_CLK_GPIO_PORT, &gpio_init_struct);    /* ETH_CLKģʽ */
    
    gpio_init_struct.Pin = ETH_MDIO_GPIO_PIN;
    HAL_GPIO_Init(ETH_MDIO_GPIO_PORT, &gpio_init_struct);   /* ETH_MDIOģʽ */
    
    gpio_init_struct.Pin = ETH_CRS_GPIO_PIN;    
    HAL_GPIO_Init(ETH_CRS_GPIO_PORT, &gpio_init_struct);    /* ETH_CRSģʽ */

    /* PC1 */
    gpio_init_struct.Pin = ETH_MDC_GPIO_PIN;
    HAL_GPIO_Init(ETH_MDC_GPIO_PORT, &gpio_init_struct);    /* ETH_MDCʼ */

    /* PC4 */
    gpio_init_struct.Pin = ETH_RXD0_GPIO_PIN;
    HAL_GPIO_Init(ETH_RXD0_GPIO_PORT, &gpio_init_struct);   /* ETH_RXD0ʼ */
    
    /* PC5 */
    gpio_init_struct.Pin = ETH_RXD1_GPIO_PIN;
    HAL_GPIO_Init(ETH_RXD1_GPIO_PORT, &gpio_init_struct);   /* ETH_RXD1ʼ */
    
    
    /* PG11,13,14 */
    gpio_init_struct.Pin = ETH_TX_EN_GPIO_PIN; 
    HAL_GPIO_Init(ETH_TX_EN_GPIO_PORT, &gpio_init_struct);  /* ETH_TX_ENʼ */

    gpio_init_struct.Pin = ETH_TXD0_GPIO_PIN; 
    HAL_GPIO_Init(ETH_TXD0_GPIO_PORT, &gpio_init_struct);   /* ETH_TXD0ʼ */
    
    gpio_init_struct.Pin = ETH_TXD1_GPIO_PIN; 
    HAL_GPIO_Init(ETH_TXD1_GPIO_PORT, &gpio_init_struct);   /* ETH_TXD1ʼ */
    
    
    /* λ */
    gpio_init_struct.Pin = ETH_RESET_GPIO_PIN;      /* ETH_RESETʼ */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*  */
    gpio_init_struct.Pull = GPIO_NOPULL;            /*  */
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;       /*  */
    HAL_GPIO_Init(ETH_RESET_GPIO_PORT, &gpio_init_struct);

    ETHERNET_RST(0);     /* Ӳλ */
    HAL_Delay(50);
    ETHERNET_RST(1);     /* λ */

    HAL_NVIC_SetPriority(ETH_IRQn, 6, 0);           /* жȼӦøһ */
    HAL_NVIC_EnableIRQ(ETH_IRQn);
}

/**
 * @breif       ȡ̫оƬĴֵ
 * @param       regȡļĴַ
 * @retval      
 */
uint32_t ethernet_read_phy(uint16_t reg)
{
    uint32_t regval;

    HAL_ETH_ReadPHYRegister(&g_eth_handler, reg, &regval);
    return regval;
}

/**
 * @breif       ̫оƬַָдĴֵ
 * @param       reg   : ҪдļĴ
 * @param       value : ҪдļĴ
 * @retval      
 */
void ethernet_write_phy(uint16_t reg, uint16_t value)
{
    uint32_t temp = value;
    
    HAL_ETH_WritePHYRegister(&g_eth_handler, reg, temp);
}

/**
 * @breif       оƬٶģʽ
 * @param       
 * @retval      1:ȡ100Mɹ
                0:ʧ
 */
uint8_t ethernet_chip_get_speed(void)
{
    uint8_t speed;
    #if(PHY_TYPE == LAN8720) 
    speed = ~((ethernet_read_phy(PHY_SR) & PHY_SPEED_STATUS));         /* LAN872031żĴжȡٶȺ˫ģʽ */
    #elif(PHY_TYPE == SR8201F)
    speed = ((ethernet_read_phy(PHY_SR) & PHY_SPEED_STATUS) >> 13);    /* SR8201F0żĴжȡٶȺ˫ģʽ */
    #elif(PHY_TYPE == YT8512C)
    speed = ((ethernet_read_phy(PHY_SR) & PHY_SPEED_STATUS) >> 14);    /* YT8512C17żĴжȡٶȺ˫ģʽ */
    #elif(PHY_TYPE == RTL8201)
    speed = ((ethernet_read_phy(PHY_SR) & PHY_SPEED_STATUS) >> 1);     /* RTL820116żĴжȡٶȺ˫ģʽ */
    #endif

    return speed;
}

extern void lwip_pkt_handle(void);                  /* lwip_comm.c涨 */

/**
 * @breif       жϷ
 * @param       
 * @retval      
 */
void ETH_IRQHandler(void)
{
    static uint32_t irq_count = 0;

    irq_count++;

    if (ethernet_get_eth_rx_size(g_eth_handler.RxDesc))
    {
        lwip_pkt_handle();      /* 以太网数据，提交LWIP */
    }
    else
    {
        /* Interrupt but no data - might be TX completion */
        if (irq_count % 100 == 0)
        {
            printf("[ETH] IRQ count: %lu (TX or spurious)\r\n", irq_count);
        }
    }

    __HAL_ETH_DMA_CLEAR_IT(&g_eth_handler, ETH_DMA_IT_NIS);   /* DMA中断标志位 */
    __HAL_ETH_DMA_CLEAR_IT(&g_eth_handler, ETH_DMA_IT_R);     /* DMA中断标志位 */
}

/**
 * @breif       ȡյ֡
 * @param       dma_rx_desc : DMA
 * @retval      frameLength : յ֡
 */
uint32_t  ethernet_get_eth_rx_size(ETH_DMADescTypeDef *dma_rx_desc)
{
    uint32_t frameLength = 0;

    if (((dma_rx_desc->Status & ETH_DMARXDESC_OWN) == (uint32_t)RESET) &&
        ((dma_rx_desc->Status & ETH_DMARXDESC_ES)  == (uint32_t)RESET) &&
        ((dma_rx_desc->Status & ETH_DMARXDESC_LS)  != (uint32_t)RESET))
    {
        frameLength = ((dma_rx_desc->Status & ETH_DMARXDESC_FL) >> ETH_DMARXDESC_FRAME_LENGTHSHIFT);
    }

    return frameLength;
}

/**
 * @breif       ΪETHײڴ
 * @param       
 * @retval      0,
 *              1,ʧ
 */
uint8_t ethernet_mem_malloc(void)
{
    /* Check if any of the buffers are already allocated */
    if (g_eth_dma_rx_dscr_tab == NULL && g_eth_dma_tx_dscr_tab == NULL &&
        g_eth_rx_buf == NULL && g_eth_tx_buf == NULL)
    {
        printf("Allocating ETH DMA buffers...\r\n");

        g_eth_dma_rx_dscr_tab = (ETH_DMADescTypeDef *)malloc(ETH_RXBUFNB * sizeof(ETH_DMADescTypeDef));
        printf("RX desc: %p\r\n", (void*)g_eth_dma_rx_dscr_tab);

        g_eth_dma_tx_dscr_tab = (ETH_DMADescTypeDef *)malloc(ETH_TXBUFNB * sizeof(ETH_DMADescTypeDef));
        printf("TX desc: %p\r\n", (void*)g_eth_dma_tx_dscr_tab);

        g_eth_rx_buf = (uint8_t *)malloc(ETH_RX_BUF_SIZE * ETH_RXBUFNB);
        printf("RX buf: %p (size=%d)\r\n", (void*)g_eth_rx_buf, ETH_RX_BUF_SIZE * ETH_RXBUFNB);

        g_eth_tx_buf = (uint8_t *)malloc(ETH_TX_BUF_SIZE * ETH_TXBUFNB);
        printf("TX buf: %p (size=%d)\r\n", (void*)g_eth_tx_buf, ETH_TX_BUF_SIZE * ETH_TXBUFNB);

        if (g_eth_dma_rx_dscr_tab == NULL || g_eth_dma_tx_dscr_tab == NULL ||
            g_eth_rx_buf == NULL || g_eth_tx_buf == NULL)
        {
            printf("ERROR: Memory allocation failed!\r\n");
            printf("  RX desc: %s\r\n", g_eth_dma_rx_dscr_tab ? "OK" : "FAIL");
            printf("  TX desc: %s\r\n", g_eth_dma_tx_dscr_tab ? "OK" : "FAIL");
            printf("  RX buf: %s\r\n", g_eth_rx_buf ? "OK" : "FAIL");
            printf("  TX buf: %s\r\n", g_eth_tx_buf ? "OK" : "FAIL");
            ethernet_mem_free();
            return 1;
        }

        memset(g_eth_dma_rx_dscr_tab, 0, ETH_RXBUFNB * sizeof(ETH_DMADescTypeDef));
        memset(g_eth_dma_tx_dscr_tab, 0, ETH_TXBUFNB * sizeof(ETH_DMADescTypeDef));
        memset(g_eth_rx_buf, 0, ETH_RX_BUF_SIZE * ETH_RXBUFNB);
        memset(g_eth_tx_buf, 0, ETH_TX_BUF_SIZE * ETH_TXBUFNB);

        printf("All ETH buffers allocated successfully\r\n");
    }

    return 0;
}

/**
 * @breif       ͷETH ײڴ
 * @param       
 * @retval      
 */
void ethernet_mem_free(void)
{
    if (g_eth_dma_rx_dscr_tab) {
        free(g_eth_dma_rx_dscr_tab);
        g_eth_dma_rx_dscr_tab = NULL;
    }
    if (g_eth_dma_tx_dscr_tab) {
        free(g_eth_dma_tx_dscr_tab);
        g_eth_dma_tx_dscr_tab = NULL;
    }
    if (g_eth_rx_buf) {
        free(g_eth_rx_buf);
        g_eth_rx_buf = NULL;
    }
    if (g_eth_tx_buf) {
        free(g_eth_tx_buf);
        g_eth_tx_buf = NULL;
    }
}
