/**
 ****************************************************************************************************
 * @file        lwip_demo.h
 * @author      CHENSI
 * @version     V1.1
 * @date        2025-10-30
 * @brief       lwIP Netconn TCPServer 框架头文件
 ****************************************************************************************************
 */
 
#ifndef _LWIP_DEMO_H
#define _LWIP_DEMO_H
#include "./SYSTEM/sys/sys.h"

#define LWIP_DEMO_RX_BUFSIZE         200                    /* 最大接收数据长度 */
#define LWIP_DEMO_PORT               8080                   /* 连接的本地端口号 */

extern int g_sock_conn;                                     /* 客户端连接套接字 */
extern int g_lwip_connect_state;                            /* 连接状态（1=已连接） */
extern uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE];   /* 接收缓冲区声明 */

void lwip_demo(void);

#endif /* _CLIENT_H */
