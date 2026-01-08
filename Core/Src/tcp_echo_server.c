/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : tcp_echo_server.c
  * @brief          : TCP Echo Server implementation
  ******************************************************************************
  * @attention
  *
  * TCP Echo Server with detailed logging
  * - Logs connection events
  * - Logs received data
  * - Logs disconnection events
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "tcp_echo_server.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
struct echo_state {
    struct tcp_pcb *pcb;
    struct pbuf *p;
};

/* Private function prototypes -----------------------------------------------*/
static err_t echo_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t echo_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void echo_error(void *arg, err_t err);
static err_t echo_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void echo_close(struct tcp_pcb *tpcb, struct echo_state *es);

/* Private variables ---------------------------------------------------------*/
static struct tcp_pcb *echo_pcb;

/**
  * @brief  Initialize the TCP echo server
  * @retval None
  */
void tcp_echo_server_init(void)
{
    err_t err;

    printf("[TCP Echo Server] Starting initialization...\r\n");

    /* Create new TCP PCB */
    echo_pcb = tcp_new();

    printf("[TCP Echo Server] TCP PCB created: %p\r\n", echo_pcb);

    if (echo_pcb != NULL)
    {
        /* Bind to port ECHO_SERVER_PORT (7) */
        printf("[TCP Echo Server] Binding to port %d...\r\n", ECHO_SERVER_PORT);
        err = tcp_bind(echo_pcb, IP_ADDR_ANY, ECHO_SERVER_PORT);

        printf("[TCP Echo Server] Bind result: %d\r\n", err);

        if (err == ERR_OK)
        {
            /* Start listening for incoming connections */
            echo_pcb = tcp_listen(echo_pcb);

            printf("[TCP Echo Server] Listen PCB: %p\r\n", echo_pcb);

            /* Set accept callback */
            tcp_accept(echo_pcb, echo_accept);

            printf("\r\n[TCP Echo Server] ========================================\r\n");
            printf("[TCP Echo Server] Initialization successful!\r\n");
            printf("[TCP Echo Server] Listening on port %d\r\n", ECHO_SERVER_PORT);
            printf("[TCP Echo Server] Static IP: 192.168.1.30\r\n");
            printf("[TCP Echo Server] Waiting for client connections...\r\n");
            printf("[TCP Echo Server] ========================================\r\n\r\n");
        }
        else
        {
            printf("[TCP Echo Server] ERROR: Bind port failed, error: %d\r\n", err);
            memp_free(MEMP_TCP_PCB, echo_pcb);
        }
    }
    else
    {
        printf("[TCP Echo Server] ERROR: Create PCB failed\r\n");
    }
}

/**
  * @brief  Accept callback for new incoming connections
  * @param  arg: user supplied argument
  * @param  newpcb: the new connection pcb
  * @param  err: error value
  * @retval err_t: error code
  */
static err_t echo_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    err_t ret_err;
    struct echo_state *es;

    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(err);

    /* Set priority for the new pcb */
    tcp_setprio(newpcb, TCP_PRIO_MIN);

    /* Allocate structure es to maintain tcp connection information */
    es = (struct echo_state *)mem_malloc(sizeof(struct echo_state));

    if (es != NULL)
    {
        es->pcb = newpcb;
        es->p = NULL;

        /* Pass newly allocated es structure as argument to newpcb */
        tcp_arg(newpcb, es);

        /* Initialize lwip tcp_recv callback function for newpcb */
        tcp_recv(newpcb, echo_recv);

        /* Initialize lwip tcp_err callback function for newpcb */
        tcp_err(newpcb, echo_error);

        /* Initialize lwip tcp_sent callback function for newpcb */
        tcp_sent(newpcb, echo_sent);

        ret_err = ERR_OK;

        /* Log connection */
        printf("\r\n>>> [TCP Echo Server] NEW CONNECTION <<<\r\n");
        printf("[TCP Echo Server] Client IP: %d.%d.%d.%d:%d\r\n",
               ip4_addr1(&newpcb->remote_ip),
               ip4_addr2(&newpcb->remote_ip),
               ip4_addr3(&newpcb->remote_ip),
               ip4_addr4(&newpcb->remote_ip),
               newpcb->remote_port);
        printf("[TCP Echo Server] Connection established\r\n\r\n");
    }
    else
    {
        printf("[TCP Echo Server] Memory alloc failed, reject connection\r\n");
        ret_err = ERR_MEM;
    }

    return ret_err;
}

/**
  * @brief  Receive callback for incoming data
  * @param  arg: pointer to echo_state structure
  * @param  tpcb: pointer to tcp_pcb structure
  * @param  p: pointer to pbuf structure
  * @param  err: error value
  * @retval err_t: error code
  */
static err_t echo_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    struct echo_state *es;
    err_t ret_err;

    LWIP_ASSERT("arg != NULL", arg != NULL);

    es = (struct echo_state *)arg;

    /* If we receive an empty tcp frame from client => close connection */
    if (p == NULL)
    {
        /* Remote host closed connection */
        printf("\r\n<<< [TCP Echo Server] CONNECTION CLOSED <<<\r\n");
        printf("[TCP Echo Server] Client IP: %d.%d.%d.%d:%d\r\n",
               ip4_addr1(&tpcb->remote_ip),
               ip4_addr2(&tpcb->remote_ip),
               ip4_addr3(&tpcb->remote_ip),
               ip4_addr4(&tpcb->remote_ip),
               tpcb->remote_port);
        printf("[TCP Echo Server] Connection terminated by client\r\n\r\n");

        es->p = NULL;
        echo_close(tpcb, es);
        ret_err = ERR_OK;
    }
    /* Else: a non empty frame was received from client but for some reason err != ERR_OK */
    else if (err != ERR_OK)
    {
        printf("[TCP Echo Server] Receive error, code: %d\r\n", err);

        /* Free received pbuf */
        if (p != NULL)
        {
            es->p = NULL;
            pbuf_free(p);
        }
        ret_err = err;
    }
    else if (es->p == NULL)
    {
        /* Store reference to incoming pbuf (chain) */
        es->p = p;

        /* Log received data */
        printf("[TCP Echo Server] <<< DATA RECEIVED <<<\r\n");
        printf("[TCP Echo Server] From: %d.%d.%d.%d:%d\r\n",
               ip4_addr1(&tpcb->remote_ip),
               ip4_addr2(&tpcb->remote_ip),
               ip4_addr3(&tpcb->remote_ip),
               ip4_addr4(&tpcb->remote_ip),
               tpcb->remote_port);
        printf("[TCP Echo Server] Length: %d bytes\r\n", p->tot_len);

        /* Print received data (first 64 bytes max) */
        if (p->tot_len > 0)
        {
            char *data = (char *)p->payload;
            int print_len = (p->len < 64) ? p->len : 64;
            printf("[TCP Echo Server] Content: \"");
            for (int i = 0; i < print_len; i++)
            {
                if (data[i] >= 32 && data[i] <= 126)
                    printf("%c", data[i]);
                else if (data[i] == '\r')
                    printf("\\r");
                else if (data[i] == '\n')
                    printf("\\n");
                else
                    printf(".");
            }
            if (p->tot_len > 64)
                printf("...");
            printf("\"\r\n");
        }

        /* Echo back the received data */
        ret_err = tcp_write(tpcb, p->payload, p->len, TCP_WRITE_FLAG_COPY);

        if (ret_err == ERR_OK)
        {
            /* Inform TCP that we have taken the data */
            tcp_recved(tpcb, p->tot_len);

            /* Free the pbuf */
            pbuf_free(p);
            es->p = NULL;

            printf("[TCP Echo Server] >>> DATA ECHOED >>> (%d bytes sent back)\r\n\r\n", p->len);
        }
        else if (ret_err == ERR_MEM)
        {
            /* We are low on memory, try again later */
            printf("[TCP Echo Server] Out of memory, retry later\r\n");
            es->p = p;
        }
        else
        {
            printf("[TCP Echo Server] Send failed, error: %d\r\n", ret_err);
            /* Free the pbuf */
            pbuf_free(p);
            es->p = NULL;
        }
    }
    else
    {
        /* Already processing a pbuf, queue this one */
        struct pbuf *ptr;

        /* Chain pbufs to the end of what we received previously */
        ptr = es->p;
        pbuf_chain(ptr, p);
    }

    return ret_err;
}

/**
  * @brief  Error callback
  * @param  arg: pointer to echo_state structure
  * @param  err: error value
  * @retval None
  */
static void echo_error(void *arg, err_t err)
{
    struct echo_state *es;

    LWIP_UNUSED_ARG(err);

    es = (struct echo_state *)arg;

    if (es != NULL)
    {
        printf("[TCP Echo Server] Connection error, code: %d\r\n", err);

        /* Free es structure */
        if (es->p != NULL)
        {
            pbuf_free(es->p);
        }
        mem_free(es);
    }
}

/**
  * @brief  Sent callback
  * @param  arg: pointer to echo_state structure
  * @param  tpcb: pointer to tcp_pcb structure
  * @param  len: length of data sent
  * @retval err_t: error code
  */
static err_t echo_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    struct echo_state *es;

    LWIP_UNUSED_ARG(len);

    es = (struct echo_state *)arg;

    if (es->p != NULL)
    {
        /* Still got pbufs to send */
        tcp_write(tpcb, es->p->payload, es->p->len, TCP_WRITE_FLAG_COPY);

        /* Inform TCP that we have taken the data */
        tcp_recved(tpcb, es->p->tot_len);

        /* Free the pbuf */
        pbuf_free(es->p);
        es->p = NULL;
    }

    return ERR_OK;
}

/**
  * @brief  Close the connection
  * @param  tpcb: pointer to tcp_pcb structure
  * @param  es: pointer to echo_state structure
  * @retval None
  */
static void echo_close(struct tcp_pcb *tpcb, struct echo_state *es)
{
    /* Remove all callbacks */
    tcp_arg(tpcb, NULL);
    tcp_sent(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);

    /* Free es structure */
    if (es != NULL)
    {
        if (es->p != NULL)
        {
            pbuf_free(es->p);
        }
        mem_free(es);
    }

    /* Close tcp connection */
    tcp_close(tpcb);

    printf("[TCP Echo Server] Connection closed\r\n");
}
