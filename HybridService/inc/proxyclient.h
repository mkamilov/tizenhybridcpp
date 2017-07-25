#ifndef __PROXY_CLIENT_H__
#define __PROXY_CLIENT_H__

#include <bundle.h>

typedef int (*proxy_client_callback_func)(void *data, bundle *const msg);

typedef struct _proxy_client {
    int local_port_id;
    char *remote_port_name;
    char *remote_app_name;

    proxy_client_callback_func cb_func;
    void *cb_data;
} proxy_client;

class ProxyClient
{
    private:
        int mLocalPortId;
        char* mRemotePortName;
        char* mRemoteAppName;

    public:
        proxy_client_callback_func mCbFunc;
        void* mCbData;

    private:
        int ConvertMsgPortResult(const int err);

    public:
        /**
         * @brief Create proxy client instance
         * @return Proxy client instance on success, otherwise NULL
         */
        ProxyClient();
        ~ProxyClient();

        int GetLocalPort() { return mLocalPortId; }
        char* GetRemotePortName() { return mRemotePortName; }
        char* GetRemoteAppName() { return mRemoteAppName; }
        void SetRemotePortName(char* remotePort) { mRemotePortName = remotePort; }
        void SetRemoteAppName(char* remoteApp) { mRemoteAppName = remoteApp; }

        /**
         * @brief Register port for proxy client to receive/send messages
         * @param[in]   port_name   Port name to set
         * @return Error code.  SVC_RES_OK if success.
         *                      SVC_RES_FAIL if fails
         */
        int RegisterPort(const char *port_name);

        /**
         * @brief Register callback function on proxy client message receive
         * @param[in]   callback_func   Callback function
         * @param[in]   data            Data set to callback function
         * @return Error code.  SVC_RES_OK if success.
         *                      SVC_RES_FAIL if fails
         */
        int RegisterMsgCallback(proxy_client_callback_func callback_func, void *data);

        /**
         * @brief Send message by registered port
         * @param[in]   msg         Message to send
         * @return Error code.  SVC_RES_OK if success.
         *                      SVC_RES_FAIL if fails
         */
        int SendMessage(bundle *const msg);
};

#endif /* __PROXY_CLIENT_H__ */
