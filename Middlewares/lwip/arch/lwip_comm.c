/**
 ****************************************************************************************************
 * @file        lwip_comm.c
 * @author      ԭŶ(ALIENTEK)
 * @version     V1.0
 * @date        2021-12-02
 * @brief       LWIP
 * @license     Copyright (c) 2020-2032, ӿƼ޹˾
 * @note        Modified for lwipandRTOS project - removed dependencies on old libraries
 ****************************************************************************************************
 */

#include "lwip_comm.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/init.h"
#include "ethernetif.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"
#include "tcp_echo_server.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"


__lwip_dev g_lwipdev;                   /* lwipƽṹ */
struct netif g_lwip_netif;              /* һȫֵӿ */

#if LWIP_DHCP
__IO uint8_t g_lwip_dhcp_state = LWIP_DHCP_OFF;         /* DHCP״̬ʼ */
#endif

/* LINK߳ */
#define LWIP_LINK_TASK_PRIO             3                   /* ȼ */
#define LWIP_LINK_STK_SIZE              128 * 2             /* ջС */
void lwip_link_thread( void * argument );                   /* ·߳ */

/* DHCP߳ */
#define LWIP_DHCP_TASK_PRIO             4                   /* ȼ */
#define LWIP_DHCP_STK_SIZE              128 * 2             /* ջС */
void lwip_periodic_handle(void *argument);                  /* DHCP߳ */
void lwip_link_status_updated(struct netif *netif);         /* DHCP״̬ص */

/**
 * @breif       lwip ĬIP
 * @param       lwipx  : lwipƽṹָ
 * @retval      
 */
void lwip_comm_default_ip_set(__lwip_dev *lwipx)
{
    /* ĬԶIPΪ:192.168.1.134 */
    lwipx->remoteip[0] = 192;
    lwipx->remoteip[1] = 168;
    lwipx->remoteip[2] = 1;
    lwipx->remoteip[3] = 27;
    
    /* MACַ */
    lwipx->mac[0] = 0xB8;
    lwipx->mac[1] = 0xAE;
    lwipx->mac[2] = 0x1D;
    lwipx->mac[3] = 0x00;
    lwipx->mac[4] = 0x01;
    lwipx->mac[5] = 0x00;
    
    /* ĬϱIPΪ:192.168.1.30 */
    lwipx->ip[0] = 192;
    lwipx->ip[1] = 168;
    lwipx->ip[2] = 1;
    lwipx->ip[3] = 30;
    /* Ĭ:255.255.255.0 */
    lwipx->netmask[0] = 255;
    lwipx->netmask[1] = 255;
    lwipx->netmask[2] = 255;
    lwipx->netmask[3] = 0;
    
    /* Ĭ:192.168.1.1 */
    lwipx->gateway[0] = 192;
    lwipx->gateway[1] = 168;
    lwipx->gateway[2] = 1;
    lwipx->gateway[3] = 1;
    lwipx->dhcpstatus = 0; /* ûDHCP */
}

/**
 * @breif       LWIPʼ(LWIPʱʹ)
 * @param       
 * @retval      0,ɹ
 *              1,ڴ
 *              2,ʧ
 */
uint8_t lwip_comm_init(void)
{
    struct netif *netif_init_flag;              /* netif_add()ʱķֵ,жʼǷɹ */
    ip_addr_t ipaddr;                           /* ipַ */
    ip_addr_t netmask;                          /*  */
    ip_addr_t gw;                               /* Ĭ */
    
    // 1. ʼTCP/IPЭջɹ
    tcpip_init(NULL, NULL);
    
    // 2. 分配内存
    if (ethernet_mem_malloc())
    {
        printf("Ethernet memory allocation FAILED\r\n");
        return 1;
    }

    // 3. ĬIP
    lwip_comm_default_ip_set(&g_lwipdev);         /* ĬIPϢ */

    // 4. 初始化太网芯片，失败后循环
    printf("[INIT] Initializing Ethernet PHY...\r\n");
    while (ethernet_init())                     /* 初始化太网芯片,失败的话延时 */
    {
        printf("[INIT] Ethernet init failed, retrying...\r\n");
        vTaskDelay(100);
    }
    printf("[INIT] Ethernet PHY initialized successfully\r\n");

    // 5. IP
#if LWIP_DHCP                                   /* ʹö̬IP */
    ip_addr_set_zero_ip4(&ipaddr);              /* IPַ뼰 */
    ip_addr_set_zero_ip4(&netmask);
    ip_addr_set_zero_ip4(&gw);
#else   /* ʹþ̬IP */
    IP4_ADDR(&ipaddr, g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
    IP4_ADDR(&netmask, g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
    IP4_ADDR(&gw, g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);
    printf("enMAC地址为:................%d.%d.%d.%d.%d.%d\r\n", g_lwipdev.mac[0], g_lwipdev.mac[1], g_lwipdev.mac[2], g_lwipdev.mac[3], g_lwipdev.mac[4], g_lwipdev.mac[5]);
    printf("静态IP地址........................%d.%d.%d.%d\r\n", g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);

    g_lwipdev.dhcpstatus = 0XFF;
    // g_lwipdev.lwip_display_fn(2);  /* 未知函数指针，注释掉 */
#endif

    // 6. 添加网络接口
    printf("[INIT] Adding network interface...\r\n");
    netif_init_flag = netif_add(&g_lwip_netif, (const ip_addr_t *)&ipaddr, (const ip_addr_t *)&netmask, (const ip_addr_t *)&gw, NULL, &ethernetif_init, &tcpip_input);

    if (netif_init_flag == NULL)
    {
        printf("[INIT] ERROR: netif_add FAILED!\r\n");
        return 2;                           /* 失败 */
    }

    printf("[INIT] Network interface added successfully\r\n");

    // 7. 设置网络接口
    /* 连接成功,netif为默认值,并使能netif */
    netif_set_default(&g_lwip_netif);       /* netif为默认 */

    if (netif_is_link_up(&g_lwip_netif))
    {
        netif_set_up(&g_lwip_netif);    /* netif */
    }
    else
    {
        netif_set_down(&g_lwip_netif);
    }

    // 8. 链路状态线程
#if LWIP_NETIF_LINK_CALLBACK
        printf("[INIT] Starting link monitoring thread...\r\n");
        lwip_link_status_updated(&g_lwip_netif);    /* DHCP状态回调 */
        netif_set_link_callback(&g_lwip_netif, lwip_link_status_updated);

        /* 查询PHY状态 */
        if (sys_thread_new("eth_link",
                       lwip_link_thread,            /* 线程入口 */
                       &g_lwip_netif,               /* 线程参数 */
                       LWIP_LINK_STK_SIZE,          /* 栈大小 */
                       LWIP_LINK_TASK_PRIO) == NULL)
        {
            printf("[INIT] ERROR: Failed to create link thread!\r\n");
        }
        else
        {
            printf("[INIT] Link thread created successfully\r\n");
        }
#endif
    
    g_lwipdev.link_status = LWIP_LINK_OFF;          /* ӱΪ0 */

#if LWIP_DHCP                                       /* ʹDHCPĻ */
    g_lwipdev.dhcpstatus = 0;                       /* DHCPΪ0 */
    /* DHCPѯ */
    sys_thread_new("eth_dhcp",
                   lwip_periodic_handle,            /* ں */
                   &g_lwip_netif,                   /* ں */
                   LWIP_DHCP_STK_SIZE,              /* ջС */
                   LWIP_DHCP_TASK_PRIO);            /* ȼ */
#endif

    return 0;                               /* ȫɹ */
}

/**
 * @brief       ֪ͨûӿ״̬
 * @param       netifƿ
 * @retval      
 */
void lwip_link_status_updated(struct netif *netif)
{
    if (netif_is_up(netif))
    {
#if LWIP_DHCP
        /* Update DHCP state machine */
        g_lwip_dhcp_state = LWIP_DHCP_START;
        printf ("The network cable is connected \r\n");
#endif /* LWIP_DHCP */
    }
    else
    {
#if LWIP_DHCP
        /* Update DHCP state machine */
        g_lwip_dhcp_state = LWIP_DHCP_LINK_DOWN;
        printf ("The network cable is not connected \r\n");
#endif /* LWIP_DHCP */
    }
}



extern xSemaphoreHandle g_rx_semaphore; /* һź */
/**
 * @breif       յݺ
 * @param       
 * @retval      
 */
void lwip_pkt_handle(void)
{
    BaseType_t xHigherPriorityTaskWoken;
    /* ȡź */
    xSemaphoreGiveFromISR(g_rx_semaphore,&xHigherPriorityTaskWoken);/* ͷŶֵź */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                   /* ҪĻһл  */
}



/* ʹDHCP */
#if LWIP_DHCP

/**
 * @breif       DHCP
 * @param       argument:β
 * @retval      
 */
void lwip_periodic_handle(void *argument)
{
     struct netif *netif = (struct netif *) argument;
    uint32_t ip = 0;
    uint32_t netmask = 0;
    uint32_t gw = 0;
    struct dhcp *dhcp;
    uint8_t iptxt[20];

    while (1)
    {
        switch (g_lwip_dhcp_state)
        {
            case LWIP_DHCP_START:
            {
                /* IPַصַҳ */
                ip_addr_set_zero_ip4(&netif->ip_addr);
                ip_addr_set_zero_ip4(&netif->netmask);
                ip_addr_set_zero_ip4(&netif->gw);
                ip_addr_set_zero_ip4(&netif->ip_addr);
                ip_addr_set_zero_ip4(&netif->netmask);
                ip_addr_set_zero_ip4(&netif->gw);
                
                g_lwip_dhcp_state = LWIP_DHCP_WAIT_ADDRESS;
                
                printf ("State: Looking for DHCP server ...\r\n");
                dhcp_start(netif);
            }
            break;
            case LWIP_DHCP_WAIT_ADDRESS:
            {
                if (dhcp_supplied_address(netif))
                {
                    g_lwip_dhcp_state = LWIP_DHCP_ADDRESS_ASSIGNED;
                    
                    ip = g_lwip_netif.ip_addr.addr;       /* ȡIPַ */
                    netmask = g_lwip_netif.netmask.addr;  /* ȡ */
                    gw = g_lwip_netif.gw.addr;            /* ȡĬ */
                    
                    sprintf((char *)iptxt, "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
                    printf ("IP address assigned by a DHCP server: %s\r\n", iptxt);
                    
                    if (ip != 0)
                    {
                        g_lwipdev.dhcpstatus = 2;         /* DHCPɹ */
                        printf("enMACַΪ:................%d.%d.%d.%d.%d.%d\r\n", g_lwipdev.mac[0], g_lwipdev.mac[1], g_lwipdev.mac[2], g_lwipdev.mac[3], g_lwipdev.mac[4], g_lwipdev.mac[5]);
                        /* ͨDHCPȡIPַ */
                        g_lwipdev.ip[3] = (uint8_t)(ip >> 24);
                        g_lwipdev.ip[2] = (uint8_t)(ip >> 16);
                        g_lwipdev.ip[1] = (uint8_t)(ip >> 8);
                        g_lwipdev.ip[0] = (uint8_t)(ip);
                        printf("ͨDHCPȡIPַ..............%d.%d.%d.%d\r\n", g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
                        /* ͨDHCPȡַ */
                        g_lwipdev.netmask[3] = (uint8_t)(netmask >> 24);
                        g_lwipdev.netmask[2] = (uint8_t)(netmask >> 16);
                        g_lwipdev.netmask[1] = (uint8_t)(netmask >> 8);
                        g_lwipdev.netmask[0] = (uint8_t)(netmask);
                        printf("ͨDHCPȡ............%d.%d.%d.%d\r\n", g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
                        /* ͨDHCPȡĬ */
                        g_lwipdev.gateway[3] = (uint8_t)(gw >> 24);
                        g_lwipdev.gateway[2] = (uint8_t)(gw >> 16);
                        g_lwipdev.gateway[1] = (uint8_t)(gw >> 8);
                        g_lwipdev.gateway[0] = (uint8_t)(gw);
                        printf("ͨDHCPȡĬ..........%d.%d.%d.%d\r\n", g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);
                        
                        // g_lwipdev.lwip_display_fn(2);  /* 未知函数指针，注释掉 */
                    }
                }
                else
                {
                    dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

                    /* DHCP timeout */
                    if (dhcp->tries > LWIP_MAX_DHCP_TRIES)
                    {
                        g_lwip_dhcp_state = LWIP_DHCP_TIMEOUT;
                        g_lwipdev.dhcpstatus = 0XFF;
                        /* ʹþ̬IPַ */
                        IP4_ADDR(&(g_lwip_netif.ip_addr), g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
                        IP4_ADDR(&(g_lwip_netif.netmask), g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
                        IP4_ADDR(&(g_lwip_netif.gw), g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);
                        netif_set_addr(netif, &g_lwip_netif.ip_addr, &g_lwip_netif.netmask, &g_lwip_netif.gw);

                        sprintf((char *)iptxt, "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
                        printf ("DHCP Timeout !! \r\n");
                        printf ("Static IP address: %s\r\n", iptxt);
                        // g_lwipdev.lwip_display_fn(2);  /* 未知函数指针，注释掉 */
                    }
                }
            }
            break;
            case LWIP_DHCP_LINK_DOWN:
            {
                g_lwip_dhcp_state = LWIP_DHCP_OFF;
            }
            break;
            default: break;
        }

        /* wait 1000 ms */
        vTaskDelay(1000);
    }
}
#endif

#if LWIP_NETIF_LINK_CALLBACK
/**
  * @brief       ETH路状态netif
  * @param       argument: netif
  * @retval
  */
void lwip_link_thread( void * argument )
{
    uint32_t regval = 0;
    struct netif *netif = (struct netif *) argument;
    int link_again_num = 0;
    uint32_t heartbeat_counter = 0;
    uint32_t phy_read_count = 0;

    printf("[LINK] Thread started, monitoring PHY link status...\r\n");

    while(1)
    {
        /* 获取PHY状态寄存器取信息 */
        if (HAL_ETH_ReadPHYRegister(&g_eth_handler, PHY_BSR, &regval) == HAL_OK)
        {
            phy_read_count++;
            if (phy_read_count % 25 == 0)  /* 每5秒打印一次PHY寄存器值 */
            {
                printf("[LINK] PHY BSR Register: 0x%04X\r\n", (unsigned int)regval);
            }
        }
        else
        {
            printf("[LINK] ERROR: Failed to read PHY register!\r\n");
        }

        /* 判断状态 */
        if((regval & PHY_LINKED_STATUS) == 0)
        {
            if (g_lwipdev.link_status != LWIP_LINK_OFF)
            {
                link_again_num++;

                if (link_again_num >= 5)                    /* Debounce: wait for 5 consecutive failures before disconnecting */
                {
                    g_lwipdev.link_status = LWIP_LINK_OFF;
#if LWIP_DHCP                                           /* 使用DHCP的话 */
                    g_lwip_dhcp_state = LWIP_DHCP_LINK_DOWN;
                    dhcp_stop(netif);
#endif
                    printf("[LINK] Cable DISCONNECTED\r\n");
                    /* 不要停止ETH，只是设置netif状态 */
                    /* HAL_ETH_Stop(&g_eth_handler);  // ETH一直在运行 */
                    netif_set_down(netif);
                    netif_set_link_down(netif);
                    link_again_num = 0;
                }
            }
        }
        else                                            /* 链路 */
        {
            link_again_num = 0;

            if (g_lwipdev.link_status == LWIP_LINK_OFF)/* 以太网 */
            {
                printf("[LINK] Cable CONNECTED\r\n");
                printf("[LINK] Setting netif UP...\r\n");
                g_lwipdev.link_status = LWIP_LINK_ON;
                /* ETH已经在low_level_init中启动了，这里不需要再调用HAL_ETH_Start */
                /* HAL_ETH_Start(&g_eth_handler);  // 不要重复启动 */
                netif_set_up(netif);
                netif_set_link_up(netif);
                printf("[LINK] Netif is now UP and RUNNING\r\n");
            }
        }

        /* Heartbeat every 10 seconds (50 ticks * 200ms) */
        heartbeat_counter++;
        if (heartbeat_counter >= 50)
        {
            heartbeat_counter = 0;
            printf("[HEARTBEAT] Link: %s | IP: %d.%d.%d.%d | Port: %d\r\n",
                   g_lwipdev.link_status ? "UP" : "DOWN",
                   ip4_addr1(ip_2_ip4(&netif->ip_addr)),
                   ip4_addr2(ip_2_ip4(&netif->ip_addr)),
                   ip4_addr3(ip_2_ip4(&netif->ip_addr)),
                   ip4_addr4(ip_2_ip4(&netif->ip_addr)),
                   ECHO_SERVER_PORT
            );
        }

        vTaskDelay(200);  /* Check every 200ms */
    }
}
#endif
