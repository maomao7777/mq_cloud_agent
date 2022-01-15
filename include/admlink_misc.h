#ifndef ADMLINK_MISC_H
#define ADMLINK_MISC_H
#include<admlink.h>
struct MemoryStruct {
  char *memory;
  size_t size;
};
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
void eloop_set_exit(void *eloop_ctx, void *timeout_ctx);
void eloop_set_publish(void *eloop_ctx, void *timeout_ctx);
void set_mqtt_publish_nexsync(mqconn* conn,
                     char* topic_name,
                     void* application_message,
                     uint8_t publish_flags);
#endif
