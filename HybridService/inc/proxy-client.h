#ifdef MIRADHAM
#ifndef __PROXY_CLIENT_H__
#define __PROXY_CLIENT_H__

#include <bundle.h>

typedef struct _proxy_client proxy_client;

/**
 * @brief Proxy client related message receive callback definition
 */
typedef int (*proxy_client_callback_func)(void *data, bundle *const msg);

/**
 * @brief Create proxy client instance
 * @return Proxy client instance on success, otherwise NULL
 */
proxy_client *proxy_client_create();

/**
 * @brief Destroy proxy client instance
 * @param[in]   proxy_cl    Proxy client instance
 */
void proxy_client_destroy(proxy_client *proxy_cl);

/**
 * @brief Register port for proxy client to receive/send messages
 * @param[in]   proxy_cl    Proxy client instance
 * @param[in]   port_name   Port name to set
 * @return Error code.  SVC_RES_OK if success.
 *                      SVC_RES_FAIL if fails
 */
int proxy_client_register_port(proxy_client *proxy_cl, const char *port_name);

/**
 * @brief Register callback function on proxy client message receive
 * @param[in]   proxy_cl        Proxy client instance
 * @param[in]   callback_func   Callback function
 * @param[in]   data            Data set to callback function
 * @return Error code.  SVC_RES_OK if success.
 *                      SVC_RES_FAIL if fails
 */
int proxy_client_register_msg_receive_callback (proxy_client *proxy_cl,
        proxy_client_callback_func callback_func,
        void *data);

/**
 * @brief Send message by registered port
 * @param[in]   proxy_cl    Proxy client instance
 * @param[in]   msg         Message to send
 * @return Error code.  SVC_RES_OK if success.
 *                      SVC_RES_FAIL if fails
 */
int proxy_client_send_message(proxy_client *proxy_cl, bundle *const msg);

#endif /* __PROXY_CLIENT_H__ */
#endif
