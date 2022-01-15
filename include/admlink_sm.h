#ifndef ADMLINK_SM_H
#define ADMLINK_SM_H
#include<admlink.h>
void publish_callback(void** ppstate, struct mqtt_response_publish *published);
void ag_handle_clttraffic(void *eloop_ctx, void *timeout_ctx);
void chk_agstat(void *eloop_ctx, void *timeout_ctx);
#endif