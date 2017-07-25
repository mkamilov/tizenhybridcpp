#ifndef __HYBRID_SERVICE_H__
#define __HYBRID_SERVICE_H__

#include <pthread.h>
#include <stdbool.h>
#include <bundle.h>

#include "types-app.h"

/* Forward declaration: */
//struct _proxy_client;

/**
 * @brief Application data
 */
typedef struct _app_data {
    //struct _proxy_client *proxy_client;     /**< proxy client structure pointer */
    pthread_t tmr_thread;                   /**< timer thread handler*/
    pthread_cond_t tmr_cond;                /**< timer thread condition */
    pthread_mutex_t tmr_mutex;              /**< timer thread mutex */
    pthread_mutex_t proxy_locker;           /**< mutex for locking sending messages process by proxy client*/
    bool run_timer;                         /**< timer run state condition */
    bool run_svc;                           /**< service run state condition */
} app_data;

class ProxyClient;

class HybridService
{
    private:
        ProxyClient* mProxy;
        app_data* mAppData;

    private:
       int SendResponse(bundle *const msg);
       int ExecuteOperation(req_operation operation_type);
       int HandleMessage(bundle *rec_msg, bundle *resp_msg, req_operation *req_oper);

    public:
        int Init(app_data* appData);
        void Deinit();
        static int OnMsgReceived(void *data, bundle *const rec_msg);
};

#endif /* __HYBRID_SERVICE_H__ */
