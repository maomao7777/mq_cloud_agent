#include<admlink_misc.h>
#include<admlink_socket.h>
#include<admlink_sm.h>
#include<admlink_msghdl.h>
#include<admlink.h>
/*****************************************************************************/
void publish_callback(void** ppstate, struct mqtt_response_publish *published) 
{
    /* note that published->topic_name is NOT null-terminated (here we'll change it to a c-string) */
	agent* pag=(agent*)*ppstate;
	dbg_printf( "cbak_state agent addr is %p \n",pag);
    char* topic_name = (char*) calloc(1,published->topic_name_size + 1);
	char* app_msg = (char*) calloc(1,published->application_message_size + 1);
    memcpy(topic_name, published->topic_name, published->topic_name_size);
    memcpy(app_msg, published->application_message, published->application_message_size);
    dbg_printf("recv_pub('%s'):%s\n", topic_name, app_msg);
#if 1
	admlink_msg_handle((const char*)app_msg, pag);
#endif
	free(topic_name);
	free(app_msg);
}
/*****************************************************************************/
void ag_handle_clttraffic(void *eloop_ctx, void *timeout_ctx)
{
	mqconn* p=(mqconn*)eloop_ctx;
	int ret;
	if((ret=mqtt_sync(&p->mqclt))!=MQTT_OK)
	{
		fprintf(stderr, "syncerror: %s\n", mqtt_error_str(ret));
		close_conn(p);
		return;
	}
	eloop_register_timeout(0,MQTT_SYN_T,ag_handle_clttraffic,p,NULL);
}
/*****************************************************************************/
void check_agstatus(agent* pag)
{
	static unsigned long int chkcnt=0;
	time_t timer;
	if(pag->agstatus==isErr)
		exit_link(1);//it is tmp method, we can chek endpoit/cert valid in the furture;
	time(&timer);
	pag->tinfo = localtime(&timer);
	pag->agstatus=isOk;
	chkcnt++;
	return;
}
/*****************************************************************************/
int check_mqrecv(mqconn* p,agent* pag)
{
  	switch(p->status)
	{
		
		case 0: //state unconnected : do connect and subscribe 
#if 1
		{
			dbg_printf("chkrecvconn:state is unconnected\n");
			open_nb_socket(&p->biofd, &p->ssl_ctx, p->daddr, p->dport, p->ca_file, NULL,p->clt_key,p->clt_cert);
			if (p->biofd == NULL) {
				return 0;
			}
			BIO_get_fd(p->biofd,&p->connfd);
			dbg_printf("conn is %d\n",p->connfd);
			/* setup a client */
			mqtt_init(&p->mqclt, p->biofd, p->sendbuf, sizeof(p->sendbuf), p->recvbuf, sizeof(p->recvbuf), publish_callback);
			p->mqclt.publish_response_callback_state=pag;
			/* Create an anonymous session */
			/* Ensure we have a clean session */
			uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
			/* Send connection request to the broker. */
			dbg_printf("chkrecvconn:set mqtt_connect pak ret is %d\n",
			mqtt_connect(&p->mqclt, p->cltid, NULL, NULL, 0, NULL, NULL, connect_flags, 60)
			);
			/* check that we don't have any errors */
			if (p->mqclt.error != MQTT_OK) {
				fprintf(stderr, "error: %s\n", mqtt_error_str(p->mqclt.error));
				return 0;
			}
			if(mqtt_sync(&p->mqclt)!= MQTT_OK)
			{
				fprintf(stderr,"mqtt connect sync error\n");
				return 0;
			}
			sleep(1); //waiting broker for 1s this is important!!
			/* subscribe */
			dbg_printf("set mqtt_subscribe to %s pak ret is %d\n",
				p->topicrecv,
				mqtt_subscribe(&p->mqclt,p->topicrecv, 0)
			);
			p->status=1;
			eloop_register_timeout(0,MQTT_SYN_T, ag_handle_clttraffic,p,NULL);
		}
#endif
		break;
		case 1:  //state connected : do check wut we need to do
		{
			dbg_printf("chkrecvconn:state is connected and subscribed to topic\n");
		}
		break;
		default:
		break;
	}
	return 1;
}
/*****************************************************************************/
int check_mqupld(mqconn* p,agent* pag)
{
  	switch(p->status)
	{
		
		case 0: //state unconnected : do connect and subscribe 
#if 1
		{
			dbg_printf("chkupldconn:state is unconnected\n");
			open_nb_socket(&p->biofd, &p->ssl_ctx, p->daddr, p->dport, p->ca_file, NULL,p->clt_key,p->clt_cert);
			if (p->biofd == NULL) {
				return 0;
			}
			BIO_get_fd(p->biofd,&p->connfd);
			dbg_printf("conn is %d\n",p->connfd);
			/* setup a client */
			mqtt_init(&p->mqclt, p->biofd, p->sendbuf, sizeof(p->sendbuf), p->recvbuf, sizeof(p->recvbuf), publish_callback);
			p->mqclt.publish_response_callback_state=pag;
			/* Create an anonymous session */
			/* Ensure we have a clean session */
			uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
			/* Send connection request to the broker. */
			dbg_printf("chkupldconn:set mqtt_connect pak ret is %d\n",
			mqtt_connect(&p->mqclt, p->cltid, NULL, NULL, 0, NULL, NULL, connect_flags, 60)
			);
			/* check that we don't have any errors */
			if (p->mqclt.error != MQTT_OK) {
				fprintf(stderr, "error: %s\n", mqtt_error_str(p->mqclt.error));
				return 0;
			}
			if(mqtt_sync(&p->mqclt)!= MQTT_OK)
			{
				fprintf(stderr,"mqtt connect sync error\n");
				return 0;
			}
			p->status=1;
			eloop_register_timeout(0,MQTT_SYN_T, ag_handle_clttraffic,p,NULL);
		}
#endif
		break;
		case 1:  //state connected : do check wut we need to do
		{
			dbg_printf("chkupldconn:state is connected\n");
		}
		break;
		default:
		break;
	}
	return 1;
}
/*****************************************************************************/
/*ag state_machine*/
void chk_agstatm(void *eloop_ctx, void *timeout_ctx)
{
	agent* p=(agent*)eloop_ctx;
	if(check_mqrecv(&p->mqrecv,p)
		&&check_mqupld(&p->mqupld,p)
	)
		check_agstatus(p);
	eloop_register_timeout(SM_CHECK_T,0,chk_agstatm,p,NULL);
	return;
}