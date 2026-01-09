/**
 ****************************************************************************************************
 * @file        lwip_demo
 * @author      CHENSI
 * @version     V1.1
 * @date        2025-10-30
 * @brief       lwIP Netconn TCPServer
 ****************************************************************************************************
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>
#include <stdio.h>
#include <lwip/sockets.h>
#include "./MALLOC/malloc.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip_demo.h"
#include "custom_protocol.h"
#include "adc_storage.h"
#include "debug_print.h"

/* Receive buffer */
uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE];

int g_sock_conn;                          /* Connection socket */
int g_lwip_connect_state = 0;

/**
 * @brief       lwip_demo implementation
 * @param       None
 * @retval      None
 */
void lwip_demo(void)
{
    struct sockaddr_in server_addr; /* Server address */
    struct sockaddr_in conn_addr;   /* Client connection address */
    socklen_t addr_len;             /* Address length */
    int length;
    int sock_fd;

    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* Create a new socket */
    if (sock_fd < 0)
    {
        my_printf("Socket creation failed, errno: %d\r\n", errno);
        error_log_append("Socket creation failed, errno: %d\r\n", errno);
        error_code_add(ERROR_CODE_COMM_FAIL);
        return;
    }
    my_printf("Socket created successfully, fd: %d\r\n", sock_fd);

    memset(&server_addr, 0, sizeof(server_addr));        /* Clear server address structure */
    server_addr.sin_family = AF_INET;                    /* IPv4 address family */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);     /* Listen on all interfaces */
    server_addr.sin_port = htons(LWIP_DEMO_PORT);        /* Set port number */

    /* Bind socket to port */
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        my_printf("Port bind failed, errno: %d\r\n", errno);
        error_log_append("Port bind failed, errno: %d\r\n", errno);
        closesocket(sock_fd);
        return;
    }
    my_printf("===========================================\r\n");
    my_printf("TCP Server started successfully!\r\n");
    my_printf("Listening on port: %d\r\n", LWIP_DEMO_PORT);
    my_printf("===========================================\r\n");

    /* Start listening for connections */
    if (listen(sock_fd, 5) < 0) {
        my_printf("Listen failed, errno: %d\r\n", errno);
        error_log_append("Listen failed, errno: %d\r\n", errno);
        closesocket(sock_fd);
        return;
    }
    my_printf("TCP Server listening, waiting for client connections...\r\n");

    while(1)
    {
        g_lwip_connect_state = 0;
        addr_len = sizeof(struct sockaddr_in); /* Initialize address length */

        g_sock_conn = accept(sock_fd, (struct sockaddr *)&conn_addr, &addr_len); /* Accept client connection */
        if (g_sock_conn < 0) /* Connection failed */
        {
            my_printf("Accept client failed, errno: %d\r\n", errno);
            error_log_append("Accept client failed, errno: %d\r\n", errno);
            error_code_add(ERROR_CODE_COMM_FAIL);
            vTaskDelay(1000);
            continue;
        }
        g_lwip_connect_state = 1;
        my_printf("\r\n>>> Client connected! <<<\r\n");
        my_printf("    Client IP: %s\r\n", inet_ntoa(conn_addr.sin_addr));
        my_printf("    Client Port: %d\r\n", ntohs(conn_addr.sin_port));
        my_printf("    Connection socket: %d\r\n", g_sock_conn);
        error_log_append("Client connected: %s:%d (socket: %d)\r\n",
                        inet_ntoa(conn_addr.sin_addr), ntohs(conn_addr.sin_port), g_sock_conn);

        /* Handle client data */
        while (1)
        {
            memset(g_lwip_demo_recvbuf,0,LWIP_DEMO_RX_BUFSIZE);
            length = recv(g_sock_conn, (unsigned int *)g_lwip_demo_recvbuf, sizeof(g_lwip_demo_recvbuf), 0); /* Receive data */

            if (length <= 0)
            {
                if (length == 0) {
                    my_printf("Client closed connection gracefully\r\n");
                } else {
                    my_printf("Receive error, errno: %d\r\n", errno);
                }
                goto atk_exit;
            }

            my_printf("\r\n[RX] Received %d bytes from client:\r\n", length);
            my_printf("     Data: %s\r\n", g_lwip_demo_recvbuf);
            error_log_append("Received data (%d bytes): %s\r\n", length, g_lwip_demo_recvbuf);

            // Parse received data using custom protocol
            custom_protocol_t *protocol = NULL;
            int unpack_ret = protocol_unpack_data(g_lwip_demo_recvbuf, length, &protocol);
            if (unpack_ret == 0)
            {
                // Successfully parsed protocol
                my_printf("     Protocol parsed successfully\r\n");
                protocol_process_command(protocol);
            }
            else
            {
                my_printf("Protocol parse failed, error code: %d, raw data: %s\r\n", unpack_ret, g_lwip_demo_recvbuf);
                error_log_append("Protocol parse failed, error code: %d, raw data: %s\r\n", unpack_ret, g_lwip_demo_recvbuf);
                error_code_add(ERROR_CODE_COMM_FAIL);
            }
        }
atk_exit:
        if (g_sock_conn >= 0)
        {
            closesocket(g_sock_conn);
            g_sock_conn = -1;
            g_lwip_connect_state = 0;
            my_printf("\r\n<<< Client disconnected >>>\r\n");
            my_printf("Connection closed, waiting for new client...\r\n\r\n");
            error_log_append("Client disconnected, connection closed\r\n");
        }

    }
}
