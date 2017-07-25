#ifndef __TYPES_APP_H__
#define __TYPES_APP_H__

typedef enum {
    SVC_RES_OK = 0,
    SVC_RES_FAIL = -1,
} svc_result;

typedef enum {
    REQ_OPER_NONE = 0,
    REQ_OPER_START_TIMER,
    REQ_OPER_STOP_TIMER,
    REQ_OPER_SEND_TIMER_EXP_MSG,
    REQ_OPER_EXIT_APP
} req_operation;

#endif /* __TYPES_APP_H__ */
