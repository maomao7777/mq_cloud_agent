#include<admlink_misc.h>
#include<admlink.h>
/*****************************************************************************/
int admlink_reboot_rc(agent* pag, char *rcid);
int admlink_imhere_rc(agent* pag, char *rcid);

/*****************************************************************************/
struct _rc_hdl_tb_ele 
{
	const char* rcid;
	int (*rc_hdl)(agent* pag, char *rcid);
};
typedef struct _rc_hdl_tb_ele  rc_hdl_tb_ele;
/*****************************************************************************/
enum 
{
	rc_failed=0,
	rc_ok,
	rc_reboot,
	rc_sysAply,
};
/*****************************************************************************/
rc_hdl_tb_ele rc_hdl_tbl[]=
{
	{"1000",admlink_reboot_rc},
	{"1000,2",admlink_reboot_rc},
	{"2000",admlink_imhere_rc},
	{"2000,2",admlink_imhere_rc},
	{NULL},
};
/*****************************************************************************/
const char* rcid2name(char* rcid)
{
	if(strncmp(rcid,"1000",4)==0)
		return "Reboot";
	if(strncmp(rcid,"2000",4)==0)
		return "I'm here";
	return "UNKNOWN";
}
/*****************************************************************************/
void get_rcfi_msg(agent* pag, char *rcid, char *resbuf, int success)
{
	char tmpbuf[128]={0};
	*resbuf='\0';
	rcid[4]='\0';
	strcat(resbuf,"{");
	snprintf(tmpbuf,sizeof(tmpbuf),"\"type\":\"event\"");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"ver\":\"2.00\"");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"prdct\":\"AP\"");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"act_id\":%d",success==1?7777:7788);//remote complete is 8010 /8020
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"act_sts\":0");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"act_trg\":%s",rcid);
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"act_para\":\"%s\"",rcid);
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"evt_id\":1");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"agt_name\":\"%s\"",AG_NAME);
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"agt_ver\":\"%s\"",AG_VER);
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"ms_ver\":\"%s\"",MSG_VER);
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"dev_id\":\"%s\"",pag->devid);
	strcat(resbuf,tmpbuf);
	strftime(tmpbuf,sizeof(tmpbuf),",\"date\":\"%Y/%m/%d %H:%M:%S\"",pag->tinfo);
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"msg\":\"%s%s\"",rcid2name(rcid),success==1?"is ok":"is failed");
	strcat(resbuf,tmpbuf);
	strcat(resbuf,"}");
}
/*****************************************************************************/
int admlink_reboot_rc(agent* pag, char *rcid)
{
	dbg_printf("do reboot\n");
	pag->agstatus=isReboot;
	//system(reboot);
	return rc_reboot;
}
/*****************************************************************************/
int admlink_imhere_rc(agent* pag, char *rcid)
{
	dbg_printf("do imhere\n");
	//system(im here);
	return rc_ok;
}
/*****************************************************************************/
void admlink_process_rc(char *rcid, agent* pag)
{
	/*!since callback msg is handled in mqtt_syn, so we need to publish response with next sync! 
		dont just do mqtt_publish in callback msg handle!!*/
	char resbuf[1024];
	int n=0;
	int rc_ret=rc_failed;
	dbg_printf("recv rcid %s\n",rcid);
	//sned recv response
	mqconn *p=&pag->mqrecv;
	memset(resbuf,0,sizeof(resbuf));
	sprintf(resbuf,"{\"state\": {\"reported\": {\"rc_id\":\"%s\"}}}",rcid);
	set_mqtt_publish_nexsync(p, p->topicsend, resbuf, MQTT_PUBLISH_QOS_1);
	//send upload evnt
	memset(resbuf,0,sizeof(resbuf));
	p=&pag->mqupld;
	while(rc_hdl_tbl[n].rcid!=NULL)
	{
		if(strcmp(rcid,rc_hdl_tbl[n].rcid)==0)
		{
			rc_ret=rc_hdl_tbl[n].rc_hdl(pag,rcid);
			break;
		}
		n++;
	}
	if(rc_ret==rc_failed){
		get_rcfi_msg(pag,rcid,resbuf,0);
		set_mqtt_publish_nexsync(p, p->topicsend, resbuf, MQTT_PUBLISH_QOS_1);
	}
	else if(rc_ret==rc_ok){
		get_rcfi_msg(pag,rcid,resbuf,1);
		set_mqtt_publish_nexsync(p, p->topicsend, resbuf, MQTT_PUBLISH_QOS_1);
	}
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
