#include "hybridservice.h"
#include "proxyclient.h"
#include "logger.h"
#include "defines-app.h"

int HybridService::Init(app_data* appData)
{
    int result = SVC_RES_FAIL;
        RETVM_IF(!appData, result, "Application data is NULL");

        //app->proxy_client = proxy_client_create();
        mProxy = new ProxyClient();
        mAppData = appData;
        //RETVM_IF(!app->proxy_client, result, "Failed to create proxy client");

        result = mProxy->RegisterPort(LOCAL_MESSAGE_PORT_NAME);
        if (result != SVC_RES_OK) {
            ERR("Failed to register proxy client port");
            //proxy_client_destroy(app->proxy_client);
            //app->proxy_client = NULL;
            return result;
        }
        DBG("done with client port");

        result = mProxy->RegisterMsgCallback(OnMsgReceived, this);
        if (result != SVC_RES_OK) {
            ERR("Failed to register proxy client on message receive callback");
            //proxy_client_destroy(app->proxy_client);
            //app->proxy_client = NULL;
            return result;
        }
        DBG("done with cb register");

        /*result = pthread_cond_init(&appData->tmr_cond, NULL);
        if (result != 0) {
            ERR("Failed to init timer condition ");
            //proxy_client_destroy(app->proxy_client);
            //app->proxy_client = NULL;
            return SVC_RES_FAIL;
        }
        DBG("done with cond init");

        result = pthread_mutex_init(&appData->tmr_mutex, NULL);
        if (result != 0) {
            ERR("Failed to init timer mutex");
            pthread_cond_destroy(&appData->tmr_cond);
            //proxy_client_destroy(app->proxy_client);
            //app->proxy_client = NULL;
            return SVC_RES_FAIL;
        }
        DBG("done with mutex init for tmr");*/

        result = pthread_mutex_init(&appData->proxy_locker, NULL);
        if (result != 0) {
            ERR("Failed to init message response mutex ");
            //pthread_cond_destroy(&appData->tmr_cond);
            //pthread_mutex_destroy(&appData->tmr_mutex);
            //proxy_client_destroy(app->proxy_client);
            //app->proxy_client = NULL;
            return SVC_RES_FAIL;
        }
        DBG("done with mutex init for locker");

        appData->run_svc = true;
        appData->run_timer = false;

        /*result = pthread_create(&appData->tmr_thread, NULL, _app_timer_thread_func, this);
        if (result != 0) {
            ERR("Failed to create timer thread");
            pthread_cond_destroy(&appData->tmr_cond);
            pthread_mutex_destroy(&appData->tmr_mutex);
            pthread_mutex_destroy(&appData->proxy_locker);
            //proxy_client_destroy(app->proxy_client);
            //app->proxy_client = NULL;
            return SVC_RES_FAIL;
        }*/
        DBG("successfully completed app init");
        return SVC_RES_OK;
}

void HybridService::Deinit()
{
    if (mAppData->tmr_thread) {
        //pthread_mutex_lock(&mAppData->tmr_mutex);
        mAppData->run_timer = false;
        mAppData->run_svc = false;
        //pthread_mutex_unlock(&mAppData->tmr_mutex);
        //pthread_cond_signal(&mAppData->tmr_cond);

        //pthread_join(mAppData->tmr_thread, NULL);
        mAppData->tmr_thread = 0;
    }
    //pthread_cond_destroy(&mAppData->tmr_cond);
    //pthread_mutex_destroy(&mAppData->tmr_mutex);
    pthread_mutex_destroy(&mAppData->proxy_locker);
    delete mProxy;
    //proxy_client_destroy(app->proxy_client);

}


int HybridService::OnMsgReceived(void *data, bundle *const rec_msg)
{
    DBG("_on_proxy_client_msg_received_cb got cb");
    int result = SVC_RES_FAIL;
    HybridService* service = static_cast<HybridService*>(data);

    req_operation req_operation = REQ_OPER_NONE;
    bundle *resp_msg = bundle_create();
    RETVM_IF(!resp_msg, result, "Failed to create bundle");

    result = service->HandleMessage(rec_msg, resp_msg, &req_operation);
    if (result != SVC_RES_OK) {
        ERR("Failed to generate response bundle");
        bundle_free(resp_msg);
        return result;
    }

    result = service->ExecuteOperation(req_operation);
    if (result == SVC_RES_OK) {
        result = service->SendResponse(resp_msg);
        if (result != SVC_RES_OK) {
            ERR("Failed to send message to remote application");
        }
    } else {
        ERR("Failed to execute operation");
    }
    bundle_free(resp_msg);

    return result;
}

int HybridService::HandleMessage(bundle *rec_msg, bundle *resp_msg, req_operation *req_oper)
{
    RETVM_IF(!rec_msg, SVC_RES_FAIL, "Received message is NULL");
    RETVM_IF(!resp_msg, SVC_RES_FAIL, "Response message is NULL");

    const char *resp_key_val = NULL;
    char *rec_key_val = NULL;
    int res = bundle_get_str(rec_msg, "command", &rec_key_val);
    RETVM_IF(res != BUNDLE_ERROR_NONE, SVC_RES_FAIL, "Failed to get string from bundle");

    if (strcmp(rec_key_val, "connect") == 0) {
        resp_key_val = "WELCOME";
        *req_oper = REQ_OPER_NONE;
    } else if (strcmp(rec_key_val, "start") == 0) {
        resp_key_val = "started";
        *req_oper = REQ_OPER_START_TIMER;
    } else if (strcmp(rec_key_val, "stop") == 0) {
        resp_key_val = "stopped";
        *req_oper = REQ_OPER_STOP_TIMER;
    } else if (strcmp(rec_key_val, "exit") == 0) {
        resp_key_val = "exit";
        *req_oper = REQ_OPER_EXIT_APP;
    } else {
        resp_key_val = "unsupported";
        *req_oper = REQ_OPER_NONE;
    }

    RETVM_IF(bundle_add_str(resp_msg, "server", resp_key_val) != 0, SVC_RES_FAIL, "Failed to add data by key to bundle");

    return SVC_RES_OK;
}

int HybridService::ExecuteOperation(req_operation operation_type)
{
    switch (operation_type) {
    case REQ_OPER_NONE:
        break;
    case REQ_OPER_START_TIMER:
        DBG("Request to start timer");
        break;
    case REQ_OPER_STOP_TIMER:
        DBG("Request to stop timer");
        break;
    case REQ_OPER_SEND_TIMER_EXP_MSG:
    {
        DBG("Request to send message on timer expired");
        bundle *msg = bundle_create();
        RETVM_IF(!msg, SVC_RES_FAIL, "Failed to create bundle");

        if (bundle_add_str(msg, "server", "timer expired") != 0) {
            ERR("Failed to add data by key to bundle");
            bundle_free(msg);
            return SVC_RES_FAIL;
        }

        int result = SendResponse(msg);
        if (result != SVC_RES_OK) {
            ERR("Failed to send message");
            bundle_free(msg);
            return result;
        }
        bundle_free(msg);
    }
    break;
    case REQ_OPER_EXIT_APP:
        DBG("Request to exit");
        break;
    default:
        DBG("Unknown request id");
        return SVC_RES_FAIL;
        break;
    }

    return SVC_RES_OK;
}

int HybridService::SendResponse(bundle *const msg)
{
    int res = SVC_RES_FAIL;

    pthread_mutex_lock(&mAppData->proxy_locker);
    res = mProxy->SendMessage(msg);
    pthread_mutex_unlock(&mAppData->proxy_locker);

    return res;
}
