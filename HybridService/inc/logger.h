#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "miradham"

#define MODULE_INFO (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define INF(fmt, arg...) dlog_print(DLOG_INFO, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define DBG(fmt, arg...) dlog_print(DLOG_DEBUG, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define WARN(fmt, arg...) dlog_print(DLOG_WARN, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define ERR(fmt, arg...) dlog_print(DLOG_ERROR, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define RETM_IF(expr, fmt, arg...) \
{ \
    if (expr) { \
        ERR(fmt, ##arg); \
        return; \
    } \
}

#define RETVM_IF(expr, val, fmt, arg...) \
{ \
    if (expr) { \
        ERR(fmt, ##arg); \
        return (val); \
    } \
}

#endif /* __LOGGER_H__ */
