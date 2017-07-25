#ifdef MIRADHAM
#include "main-app.h"
#include "defines-app.h"
#include "types-app.h"
#include "logger.h"
#include "proxy-client.h"

#include <service_app.h>
#include <stdlib.h>

/* app event callbacks */
static bool _on_create_cb(void *user_data);
static void _on_terminate_cb(void *user_data);
static void _on_app_control_callback(app_control_h app_control, void *user_data);

static int _app_init(app_data *app);
static int _app_send_response(app_data *app, bundle *const msg);
static int _app_execute_operation(app_data *appdata, req_operation operation_type);
static int _app_process_received_message(bundle *rec_msg, bundle *resp_msg, req_operation *req_oper);
static void *_app_timer_thread_func(void *data);
static int _on_proxy_client_msg_received_cb(void *data, bundle *const rec_msg);

app_data *app_create()
{
    DBG("creating app");
    app_data *app = calloc(1, sizeof(app_data));
    return app;
}

void app_destroy(app_data *app)
{
    free(app);
}

int app_run(app_data *app, int argc, char **argv)
{
    RETVM_IF(!app, -1, "Application data is NULL");

    service_app_lifecycle_callback_s cbs = {
            .create = _on_create_cb,
            .terminate = _on_terminate_cb,
            .app_control = _on_app_control_callback
    };

    return service_app_main(argc, argv, &cbs, app);
}

static bool _on_create_cb(void *user_data)
{
    RETVM_IF(!user_data, false, "User_data is NULL");
    RETVM_IF(_app_init(user_data) != SVC_RES_OK, false, "Failed to initialise application data");
    return true;
}

static void _on_terminate_cb(void *user_data)
{
    if (user_data) {
        app_data *app = user_data;
        proxy_client_destroy(app->proxy_client);

        if (app->tmr_thread) {
            pthread_mutex_lock(&app->tmr_mutex);
            app->run_timer = false;
            app->run_svc = false;
            pthread_mutex_unlock(&app->tmr_mutex);
            pthread_cond_signal(&app->tmr_cond);

            pthread_join(app->tmr_thread, NULL);
            app->tmr_thread = 0;
        }
        pthread_cond_destroy(&app->tmr_cond);
        pthread_mutex_destroy(&app->tmr_mutex);
        pthread_mutex_destroy(&app->proxy_locker);
        proxy_client_destroy(app->proxy_client);
    }
}

void _on_app_control_callback(app_control_h app_control, void *user_data)
{
    DBG("got control");
}

static int _on_proxy_client_msg_received_cb(void *data, bundle *const rec_msg)
{
    DBG("_on_proxy_client_msg_received_cb got cb");
    int result = SVC_RES_FAIL;
    RETVM_IF(!data, result, "Data is NULL");

    app_data *app = data;
    req_operation req_operation = REQ_OPER_NONE;

    bundle *resp_msg = bundle_create();
    RETVM_IF(!resp_msg, result, "Failed to create bundle");

    result = _app_process_received_message(rec_msg, resp_msg, &req_operation);
    if (result != SVC_RES_OK) {
        ERR("Failed to generate response bundle");
        bundle_free(resp_msg);
        return result;
    }

    result = _app_execute_operation(app, req_operation);
    if (result == SVC_RES_OK) {
        result = _app_send_response(app, resp_msg);
        if (result != SVC_RES_OK) {
            ERR("Failed to send message to remote application");
        }
    } else {
        ERR("Failed to execute operation");
    }
    bundle_free(resp_msg);

    return result;
}

static int _app_init(app_data *app)
{
    int result = SVC_RES_FAIL;
    RETVM_IF(!app, result, "Application data is NULL");

    app->proxy_client = proxy_client_create();
    RETVM_IF(!app->proxy_client, result, "Failed to create proxy client");

    result = proxy_client_register_port(app->proxy_client, LOCAL_MESSAGE_PORT_NAME);
    if (result != SVC_RES_OK) {
        ERR("Failed to register proxy client port");
        proxy_client_destroy(app->proxy_client);
        app->proxy_client = NULL;
        return result;
    }
    DBG("done with client port");

    result = proxy_client_register_msg_receive_callback(app->proxy_client, _on_proxy_client_msg_received_cb, app);
    if (result != SVC_RES_OK) {
        ERR("Failed to register proxy client on message receive callback");
        proxy_client_destroy(app->proxy_client);
        app->proxy_client = NULL;
        return result;
    }
    DBG("done with cb register");

    result = pthread_cond_init(&app->tmr_cond, NULL);
    if (result != 0) {
        ERR("Failed to init timer condition ");
        proxy_client_destroy(app->proxy_client);
        app->proxy_client = NULL;
        return SVC_RES_FAIL;
    }
    DBG("done with cond init");

    result = pthread_mutex_init(&app->tmr_mutex, NULL);
    if (result != 0) {
        ERR("Failed to init timer mutex");
        pthread_cond_destroy(&app->tmr_cond);
        proxy_client_destroy(app->proxy_client);
        app->proxy_client = NULL;
        return SVC_RES_FAIL;
    }
    DBG("done with mutex init for tmr");

    result = pthread_mutex_init(&app->proxy_locker, NULL);
    if (result != 0) {
        ERR("Failed to init message response mutex ");
        pthread_cond_destroy(&app->tmr_cond);
        pthread_mutex_destroy(&app->tmr_mutex);
        proxy_client_destroy(app->proxy_client);
        app->proxy_client = NULL;
        return SVC_RES_FAIL;
    }
    DBG("done with mutex init for locker");

    app->run_svc = true;
    app->run_timer = false;

    result = pthread_create(&app->tmr_thread, NULL, _app_timer_thread_func, app);
    if (result != 0) {
        ERR("Failed to create timer thread");
        pthread_cond_destroy(&app->tmr_cond);
        pthread_mutex_destroy(&app->tmr_mutex);
        pthread_mutex_destroy(&app->proxy_locker);
        proxy_client_destroy(app->proxy_client);
        app->proxy_client = NULL;
        return SVC_RES_FAIL;
    }
    DBG("successfully completed app init");
    return SVC_RES_OK;
}

static int _app_process_received_message(bundle *rec_msg,
        bundle *resp_msg,
        req_operation *req_oper)
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

static void _app_run_timer(app_data *appdata, bool state)
{
    pthread_mutex_lock(&appdata->tmr_mutex);
    appdata->run_timer = state;
    pthread_mutex_unlock(&appdata->tmr_mutex);
    pthread_cond_signal(&appdata->tmr_cond);
}

static int _app_execute_operation(app_data *appdata, req_operation operation_type)
{
    RETVM_IF(!appdata, SVC_RES_FAIL, "Application data is NULL");

    switch (operation_type) {
    case REQ_OPER_NONE:
        break;
    case REQ_OPER_START_TIMER:
        DBG("Request to start timer");
        _app_run_timer(appdata, true);
        break;
    case REQ_OPER_STOP_TIMER:
        DBG("Request to stop timer");
        _app_run_timer(appdata, false);
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

        int result = _app_send_response(appdata, msg);
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

static void *_app_timer_thread_func(void *data)
{
    RETVM_IF(!data, NULL, "Data is NULL");
    app_data *app = data;

    pthread_mutex_lock(&app->tmr_mutex);
    bool was_timedwait = false;

    while (app->run_svc) {
        if (app->run_timer) {
            if (was_timedwait) {
                if (_app_execute_operation(app, REQ_OPER_SEND_TIMER_EXP_MSG) != SVC_RES_OK) {
                    ERR("Execute operation failed");
                    app->run_timer = false;
                    continue;
                }
            } else {
                was_timedwait = true;
            }
            struct timespec nowTs;
            clock_gettime(CLOCK_REALTIME, &nowTs);
            nowTs.tv_sec += TIMER_INTERVAL_RUN;
            pthread_cond_timedwait(&app->tmr_cond, &app->tmr_mutex, &nowTs);
        } else {
            was_timedwait = false;
            pthread_cond_wait(&app->tmr_cond, &app->tmr_mutex);
        }
    }
    pthread_mutex_unlock(&app->tmr_mutex);

    return NULL;
}

static int _app_send_response(app_data *app, bundle *const msg)
{
    int res = SVC_RES_FAIL;

    pthread_mutex_lock(&app->proxy_locker);
    res = proxy_client_send_message(app->proxy_client, msg);
    pthread_mutex_unlock(&app->proxy_locker);

    return res;
}
#endif
