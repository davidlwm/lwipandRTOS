/**
 ****************************************************************************************************
 * @file        ethernet.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2021-12-02
 * @brief       ETHERNET ��������
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� ̽���� F407������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20211202
 * ��һ�η���
 *
 ****************************************************************************************************
 */
 
#ifndef __ETHERNET_H
#define __ETHERNET_H
#include "stm32f4xx_hal.h"

/******************************************************************************************/
/* PHY配置 */

// 以太网PHY芯片地址 (LAN8720/LAN8742默认地址)
#define ETHERNET_PHY_ADDRESS            0x00

// PHY类型选择
#define PHY_TYPE                        LAN8720

// PHY状态寄存器定义 (LAN8720)
#define PHY_BSR                         31  // Basic Status Register
#define PHY_LINKED_STATUS               0x0004  // Link Status

// 以太网缓冲区大小
#define ETH_RX_BUF_SIZE                 1536  // 接收缓冲区大小
#define ETH_TX_BUF_SIZE                 1536  // 发送缓冲区大小

// 以太网描述符数量
#define ETH_RXBUFNB                     4     // 接收描述符数量
#define ETH_TXBUFNB                     4     // 发送描述符数量

/******************************************************************************************/
/* ���� ���� */

#define ETH_CLK_GPIO_PORT               GPIOA
#define ETH_CLK_GPIO_PIN                GPIO_PIN_1
#define ETH_CLK_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)                  /* ����IO��ʱ��ʹ�� */

#define ETH_MDIO_GPIO_PORT              GPIOA
#define ETH_MDIO_GPIO_PIN               GPIO_PIN_2
#define ETH_MDIO_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)                 /* ����IO��ʱ��ʹ�� */

#define ETH_CRS_GPIO_PORT               GPIOA
#define ETH_CRS_GPIO_PIN                GPIO_PIN_7
#define ETH_CRS_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)                  /* ����IO��ʱ��ʹ�� */

#define ETH_MDC_GPIO_PORT               GPIOC
#define ETH_MDC_GPIO_PIN                GPIO_PIN_1
#define ETH_MDC_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOC_CLK_ENABLE();}while(0)                  /* ����IO��ʱ��ʹ�� */

#define ETH_RXD0_GPIO_PORT              GPIOC
#define ETH_RXD0_GPIO_PIN               GPIO_PIN_4
#define ETH_RXD0_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE();}while(0)                  /* ����IO��ʱ��ʹ�� */

#define ETH_RXD1_GPIO_PORT              GPIOC
#define ETH_RXD1_GPIO_PIN               GPIO_PIN_5
#define ETH_RXD1_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE();}while(0)                  /* ����IO��ʱ��ʹ�� */

#define ETH_TX_EN_GPIO_PORT             GPIOG
#define ETH_TX_EN_GPIO_PIN              GPIO_PIN_11
#define ETH_TX_EN_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOG_CLK_ENABLE();}while(0)                  /* ����IO��ʱ��ʹ�� */

#define ETH_TXD0_GPIO_PORT              GPIOG
#define ETH_TXD0_GPIO_PIN               GPIO_PIN_13
#define ETH_TXD0_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOG_CLK_ENABLE();}while(0)                  /* ����IO��ʱ��ʹ�� */

#define ETH_TXD1_GPIO_PORT              GPIOG
#define ETH_TXD1_GPIO_PIN               GPIO_PIN_14
#define ETH_TXD1_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOG_CLK_ENABLE();}while(0)                  /* ����IO��ʱ��ʹ�� */

#define ETH_RESET_GPIO_PORT             GPIOD
#define ETH_RESET_GPIO_PIN              GPIO_PIN_3
#define ETH_RESET_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOD_CLK_ENABLE();}while(0)                  /* ����IO��ʱ��ʹ�� */


/******************************************************************************************/

/* ETH�˿ڶ��� */
#define ETHERNET_RST(x)  do{ x ? \
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET) : \
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET); \
                        }while(0)



extern ETH_HandleTypeDef g_eth_handler;                                 /* ��̫����� */
extern ETH_DMADescTypeDef *g_eth_dma_rx_dscr_tab;                       /* ��̫��DMA�������������ݽṹ��ָ�� */
extern ETH_DMADescTypeDef *g_eth_dma_tx_dscr_tab;                       /* ��̫��DMA�������������ݽṹ��ָ�� */
extern uint8_t *g_eth_rx_buf;                                           /* ��̫���ײ���������buffersָ�� */
extern uint8_t *g_eth_tx_buf;                                           /* ��̫���ײ���������buffersָ�� */

uint8_t ethernet_init(void);                                            /* ��̫��оƬ��ʼ�� */
uint32_t ethernet_read_phy(uint16_t reg);                               /* ��ȡ��̫��оƬ�Ĵ���ֵ */
void ethernet_write_phy(uint16_t reg, uint16_t value);                  /* ����̫��оƬָ����ַд��Ĵ���ֵ */
uint8_t ethernet_chip_get_speed(void);                                  /* �����̫��оƬ���ٶ�ģʽ */
uint32_t  ethernet_get_eth_rx_size(ETH_DMADescTypeDef *dma_rx_desc);    /* ��ȡ���յ���֡���� */

uint8_t ethernet_mem_malloc(void);                                           /* ΪETH�ײ����������ڴ� */
void ethernet_mem_free(void);                                                /* �ͷ�ETH �ײ�����������ڴ� */
#endif

