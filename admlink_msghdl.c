#include<admlink_misc.h>
#include<admlink.h>
/*****************************************************************************/
void admlink_process_rc(char *rcid, agent* pag)
{
	/*!since callback msg is handled in mqtt_syn, so we need to publish response with next sync! 
		dont just do mqtt_publish in callback msg handle!!*/
	char resbuf[512];
	char tmpbuf[256];
	time_t timer;
	time(&timer);
	struct tm* tm_info = localtime(&timer);
	dbg_printf("recv rcid %s\n",rcid);
	if(strcmp(rcid,"1234")==0)
		goto DO_REBOOT;
	else
	{
		mqconn *p=&pag->mqrecv;
		memset(resbuf,0,sizeof(resbuf));
		sprintf(resbuf,"{\"state\": {\"reported\": {\"rc_id\":\"%s\"}}}",rcid);
		set_mqtt_publish_nexsync(p, p->topicsend, resbuf, MQTT_PUBLISH_QOS_1);
	}
	return;
	DO_REBOOT:
		dbg_printf("do reboot\n");
		//sned recv response
		mqconn *p=&pag->mqrecv;
		memset(resbuf,0,sizeof(resbuf));
		sprintf(resbuf,"{\"state\": {\"reported\": {\"rc_id\":\"%s\"}}}",rcid);
		set_mqtt_publish_nexsync(p, p->topicsend, resbuf, MQTT_PUBLISH_QOS_1);
		//sned upld event
		p=&pag->mqupld;
		memset(resbuf,0,sizeof(resbuf));
		memset(tmpbuf,0,sizeof(tmpbuf));
		snprintf(tmpbuf,sizeof(tmpbuf),"%s",
			"{"
			"\"type\":\"event\""
			",\"ver\":\"2.00\""
			",\"prdct\":\"unknown\""
			",\"evt_id\":1"
			",\"act_id\":7777"//remote complete is 7777
			",\"act_trg\":1234"//set rcid
			",\"act_para\":\"1234\""
			",\"act_sts\":0"
			",\"msg\":\"ok reboot\""
		);
		strcat(resbuf,tmpbuf);
		
		snprintf(tmpbuf,sizeof(tmpbuf),",\"agt_name\":\"%s\"","maomaoag");
		strcat(resbuf,tmpbuf);
		
		snprintf(tmpbuf,sizeof(tmpbuf),",\"agt_ver\":\"%s\"","9.9.9");
		strcat(resbuf,tmpbuf);
		
		snprintf(tmpbuf,sizeof(tmpbuf),",\"ms_ver\":\"%s\"","9.9.9");
		strcat(resbuf,tmpbuf);
		
		snprintf(tmpbuf,sizeof(tmpbuf),",\"dev_id\":\"%s\"",pag->devid);
		strcat(resbuf,tmpbuf);
		
		strftime(tmpbuf,sizeof(tmpbuf),",\"date\":\"%Y/%m/%d %H:%M:%S\"",tm_info);
		strcat(resbuf,tmpbuf);
		
		strcat(resbuf,"}");
		
		set_mqtt_publish_nexsync(p, p->topicsend, resbuf, MQTT_PUBLISH_QOS_1);
		dbg_printf("do_reboot\n");
	return;
}
/*****************************************************************************/
int admlink_msg_handle(const char *buffer, agent* p)
{
	int ret=0;
	cJSON *pdata = cJSON_Parse(buffer);
	if(pdata)
    {
		cJSON *pstate = cJSON_GetObjectItem(pdata, "state");
        if(pstate)
        {
            cJSON *pdesired = cJSON_GetObjectItem(pstate, "desired");
            if(pdesired)
            {
				cJSON *prcid = cJSON_GetObjectItem(pdesired, "rc_id");
				if(prcid)
					admlink_process_rc(prcid->valuestring,p);
            }
        }
	}
	cJSON_Delete(pdata);
	return ret;
}
