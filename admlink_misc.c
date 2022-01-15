#include<admlink_misc.h>
/*****************************************************************************/

/* WriteMemoryCallback
 * use this callback will malloc new memory and replace data.(different with HeaderWriteMemoryCallback)
 */
/*****************************************************************************/
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char *ptr = (char *)malloc(realsize + 1);
  if(!ptr) {
    /* out of memory! */ 
    // fprintf(stderr, "not enough memory (malloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(mem->memory, contents, realsize + 1);
  mem->size = realsize;
  // mem->memory[mem->size] = 0;

  return realsize;
}
/*****************************************************************************/
void eloop_set_exit(void *eloop_ctx, void *timeout_ctx)
{
	exit_link(EXIT_SUCCESS);
}
/*****************************************************************************/
struct _setpublish  
{
	mqconn *conn;
	char topic_name[128];
	void* application_message;
	size_t application_message_size;
	uint8_t publish_flags;
};
typedef struct _setpublish set_publish;
/*****************************************************************************/
void eloop_set_publish(void *eloop_ctx, void *timeout_ctx)
{
	set_publish *p=(set_publish*) eloop_ctx;
	int ret;
	ret=mqtt_publish(&p->conn->mqclt,(const char *)p->topic_name,(const void *)p->application_message,p->application_message_size,p->publish_flags);
	dbg_printf("set pak with msg[%s] to topic [%s] wiith ret %d\n",(char *)p->application_message,p->topic_name,ret);
	/* check for errors */
	if (p->conn->mqclt.error != MQTT_OK) {
		fprintf(stderr, "error: %s\n", mqtt_error_str(p->conn->mqclt.error));
		close_conn(p->conn);
	}
	free(p->application_message);
	free(p);
}
/*****************************************************************************/
void set_mqtt_publish_nexsync(mqconn* conn,
                     char* topic_name,
                     void* application_message,
                     uint8_t publish_flags)
{
	set_publish *p=(set_publish*) calloc(1,sizeof(set_publish));
	p->conn=conn;
	p->application_message_size=strlen((char*)application_message)+1;
	p->publish_flags=publish_flags;
	p->application_message=calloc(1,p->application_message_size);
	strcpy(p->topic_name,topic_name);
	strcpy(p->application_message,application_message);
	eloop_register_timeout(0,MQTT_SYN_T,eloop_set_publish,p,NULL);
}
