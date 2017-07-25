#include "proxyclient.h"
#include "logger.h"
#include "types-app.h"

#include <stdlib.h>
#include <stdbool.h>
#include <message_port.h>

static int _proxy_client_set_remote_data(ProxyClient *proxy,
        const char *rem_app_name,
        const char *rem_port_name);
//static int _proxy_client_convert_msg_port_result(const int err);

static void _on_message_received_cb(int port_id,
        const char *rem_app_name,
        const char *rem_port_name,
        bool trusted_message,
        bundle *rec_msg,
        void *user_data);

ProxyClient::ProxyClient()
{
    mRemoteAppName = nullptr;
    mRemotePortName = nullptr;
    mCbData = nullptr;
    mCbFunc = nullptr;
    mLocalPortId = 0;

}

ProxyClient::~ProxyClient()
{
    message_port_unregister_local_port(mLocalPortId);
    delete(mRemotePortName);
    delete(mRemoteAppName);
}

int ProxyClient::RegisterPort(const char* portName)
{
    int result = SVC_RES_FAIL;

    RETVM_IF(!portName, result, "Message port name is NULL");

    int temp_id = message_port_register_local_port(portName, _on_message_received_cb, this);
    if (temp_id < 0) {
        ConvertMsgPortResult(temp_id);
        ERR("Failed to register local message port");
        mLocalPortId = 0;
        return result;
    }

    DBG("Message port %s registered with id: %d", portName, temp_id);
    mLocalPortId = temp_id;

    return SVC_RES_OK;
}

int ProxyClient::SendMessage(bundle *const msg)
{
    int result = SVC_RES_FAIL;

    RETVM_IF(!(mLocalPortId), result, "Message port is not registered");
    RETVM_IF(!(mRemoteAppName), result, "Remote application name is not registered");
    RETVM_IF(!(mRemotePortName), result, "Remote message port is not registered");

    result = ConvertMsgPortResult(message_port_send_message_with_local_port(
            mRemoteAppName,
            mRemotePortName,
            msg,
            mLocalPortId));

    RETVM_IF(result != SVC_RES_OK, result, "Failed to send bidirectional message to: %s:%s",
            mRemoteAppName,
            mRemotePortName);

    DBG("Message successfully send to: %s:%s", mRemoteAppName, mRemotePortName);
    return result;
}

int ProxyClient::RegisterMsgCallback(proxy_client_callback_func callback_func, void *data)
{
    mCbFunc = callback_func;
    mCbData = data;

    return SVC_RES_OK;
}

static int _proxy_client_set_remote_data(ProxyClient *proxy,
        const char *rem_app_name,
        const char *rem_port_name)
{
    //RETVM_IF(!proxy_cl, SVC_RES_FAIL, "Proxy pointer is NULL");

    char *temp_rem_app_name = NULL;
    char *temp_rem_port_name = NULL;

    if (!proxy->GetRemoteAppName() && rem_app_name) {
        temp_rem_app_name = strdup(rem_app_name);
        RETVM_IF(!temp_rem_app_name, SVC_RES_FAIL,
                "Failed to set remote application name. Strdup failed");
    }

    if (!proxy->GetRemotePortName() && rem_port_name) {
        temp_rem_port_name = strdup(rem_port_name);
        if (!temp_rem_port_name) {
            ERR("Failed to set remote port name. Strdup failed");
            free(temp_rem_app_name);
            return SVC_RES_FAIL;
        }
    }

    if (temp_rem_app_name) {
        //proxy_cl->remote_app_name = temp_rem_app_name;
        proxy->SetRemoteAppName(temp_rem_app_name);
    }
    if (temp_rem_port_name) {
        proxy->SetRemotePortName(temp_rem_port_name);
    }

    return SVC_RES_OK;
}

int ProxyClient::ConvertMsgPortResult(const int err)
{
    int result = SVC_RES_FAIL;

    switch (err) {
    case MESSAGE_PORT_ERROR_NONE:
        result = SVC_RES_OK;
        break;
    case MESSAGE_PORT_ERROR_IO_ERROR:
        ERR("MessagePort error: i/o error");
        break;
    case MESSAGE_PORT_ERROR_OUT_OF_MEMORY:
        ERR("MessagePort error: out of memory");
        break;
    case MESSAGE_PORT_ERROR_INVALID_PARAMETER:
        ERR("MessagePort error: invalid parameter");
        break;
    case MESSAGE_PORT_ERROR_PORT_NOT_FOUND:
        ERR("MessagePort error: message port not found");
        break;
    case MESSAGE_PORT_ERROR_CERTIFICATE_NOT_MATCH:
        ERR("MessagePort error: certificate not match");
        break;
    case MESSAGE_PORT_ERROR_MAX_EXCEEDED:
        ERR("MessagePort error: max exceeded");
        break;
    default:
        ERR("MessagePort error: unknown error");
        break;
    }
    return result;
}

static void _on_message_received_cb(int port_id,
        const char *rem_app_name,
        const char *rem_port_name,
        bool trusted_message,
        bundle *rec_msg,
        void *user_data)
{
    DBG("Received message from port %d", port_id);

    RETM_IF(!user_data, "user_data is NULL");

    ProxyClient *proxy = static_cast<ProxyClient*>(user_data);

    if (port_id != proxy->GetLocalPort()) {
        ERR("Receive message by unknown port id = %d", port_id);
        return;
    }

    int res = _proxy_client_set_remote_data(proxy, rem_app_name, rem_port_name);
    RETM_IF(res != SVC_RES_OK , "Failed to set remote data to message port");

    if (proxy->mCbFunc) {
        res = proxy->mCbFunc(proxy->mCbData, rec_msg);
        RETM_IF(res != SVC_RES_OK , "Message port callback function failed");
    } else {
        DBG("Message port callback function not set");
    }
}
