#include "hybridservice.h"
#include <service_app.h>

static bool ServiceEventCreate(void *data);
static void ServiceEventTerminate(void *data);
static void ServiceEventControl(app_control_h svc, void *data);

static HybridService *mService = nullptr;

int main(int argc, char **argv)
{
    int result = 0;
    mService = new HybridService();
    app_data appData = {0, };

    service_app_lifecycle_callback_s cbs = {
            .create = ServiceEventCreate,
            .terminate = ServiceEventTerminate,
            .app_control = ServiceEventControl
    };

    result = service_app_main(argc, argv, &cbs, &appData);

    delete mService;
    return result;
}

static bool ServiceEventCreate(void *data)
{
    app_data *appData = (app_data *)data;

    if (mService) {
        mService->Init(appData);
    }

    return true;
}

static void ServiceEventTerminate(void *data)
{
    if (mService) {
        mService->Deinit();
    }
}

static void ServiceEventControl(app_control_h svc, void *data)
{
}
