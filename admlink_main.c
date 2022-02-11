#include <admlink.h>
#include <admlink_socket.h>
#include <admlink_misc.h>
#include <admlink_sm.h>
#include <admlink_ctrl_if.h>
/* crtl_if initial callback function */
typedef int (*admlink_iface_init_callback_f)(admlinkdaemon *admcd);
/* crtl_if de-initial callback function */
typedef void (*admlink_iface_deinit_callback_f)(admlinkdaemon *admcd);
struct admlinkdaemon_interface
{
    admlink_iface_init_callback_f init;
    admlink_iface_deinit_callback_f deinit;
};
/* interface list */
struct admlinkdaemon_interface admlinkdaemon_intf_list[] =
{
    {admlink_ctrl_iface_init,     admlink_ctrl_iface_deinit},    /* admlink_ctrl */
};
admlink_interfaces interfaces;
admlinkdaemon admcd;
agent g_agent;
uint8_t gdbg;
/*****************************************************************************/
void handle_debug(int sig)
{
    printf("%s[%d]: signal = %d\n", __FUNCTION__, __LINE__, sig);
    printf("dbg change to %u\n",gdbg=gdbg?0:1);
}
/*****************************************************************************/
void handle_pipe(int sig)
{
    printf("handle signal_pipe ign\n");
}
/*****************************************************************************/
void dbg_printf_to_file(char *format, ...)
{	
	FILE *fp;
	char fname[128]="", write_type[2]="w";
	char buf[BUF_SEND_SIZE];
    va_list ap;
	/*is debug is disable, don't printf anything*/
    if(!gdbg)
        return;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
	fp = fopen("/tmp/admlink_dbg", "r");
	if(fp) {
		fgets(fname,sizeof(fname),fp);
		//Remove the last \n
		if(strlen(fname)>0 && fname[strlen(fname)-1]==10)
			fname[strlen(fname)-1]=0;
		fclose(fp);
	}
	if(!strlen(fname))
		strcpy(fname,"/dev/console");
	//Check file exist or not.
	if( access( fname, F_OK ) != -1 ) {
		// file exists
		struct stat sb;
		if (stat(fname, &sb) != -1) {
			switch (sb.st_mode & S_IFMT) {
			case S_IFREG:
				strcpy(write_type,"a");
				break;
			default:
				strcpy(write_type,"w");
				break;
			}
		}
		fp = fopen(fname,write_type);
	} else {
		// file doesn't exist, then create one and see.
		fp = fopen(fname,"a");
		//The file still cannot be created, then we output to /dev/console
		if(!fp)
			fp = fopen("/dev/console","w");
	}
	if(fp) {
		fprintf(fp, "[adml]%s", buf);
		fclose(fp);
	}
	return;
}

/*****************************************************************************/
int dbg_printf(char *format, ...)
{
    char buf[BUF_SEND_SIZE];
    va_list ap;
    /*is debug is disable, don't printf anything*/
    if(!gdbg)
        return 0;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
#if 0
    return printf(buf);
#else
	dbg_printf_to_file("%s", buf);
#endif
    return 0;
}
/*****************************************************************************/
void close_conn(mqconn* conn)
{
	int trys=0;
    if (conn->biofd != NULL){
		do
		{
			dbg_printf("try to close conn[%d]\n",conn->connfd);
			if(mqtt_disconnect(&conn->mqclt)==MQTT_OK)
				trys=(mqtt_sync(&conn->mqclt)==MQTT_OK)?-1:trys;
			trys++;
			usleep(100000U);
		}while(trys>0&&trys<3);
		BIO_free_all(conn->biofd);
		conn->biofd=NULL;
		conn->ssl_ctx=NULL;
	}
	dbg_printf("conn[%d] closed\n",conn->connfd);
	conn->connfd=0;
	conn->status=0;
}
/*****************************************************************************/
void exit_link(int exitval)
{
	if(interfaces.admcd[0])
		admlinkdaemon_intf_list[0].deinit(interfaces.admcd[0]);
    eloop_destroy();
	close_conn(&g_agent.mqrecv);
	close_conn(&g_agent.mqupld);
    exit(exitval);
}
/*****************************************************************************/
void handle_stop(int sig)
{
	printf("%s[%d]: signal = %d\n", __FUNCTION__, __LINE__, sig);
	exit_link(EXIT_SUCCESS);
}
/*****************************************************************************/
int setup_reg()
{
    return 0;
}
/*****************************************************************************/
int setup_endpoint()
{
// 	CURL *curl;
// 	CURLcode res;
//     char user_agent[64] ="";
// 	char req_data[512] = "";
// 	struct MemoryStruct chunk;
// 	cJSON *json = NULL;
//     sprintf(user_agent, "ELECOM-%s-Default","WABS1775");
//    	curl_global_init(CURL_GLOBAL_DEFAULT);
// 	curl = curl_easy_init();
// 	chunk.size = 0;
// 	if(curl) 
// 	{
// 		/* always cleanup */
// 		curl_easy_cleanup(curl);
// 	}
    return 0;
}
#define fuckuele 1
/*****************************************************************************/
int cfg_agInit(agent *p,int argc,char **argv)
{
    int ret=1;
#if AG_DBG
	gdbg=1;
#endif
#if 0
	char addr[128]="ag123456789ab-ats.iot.ap-northeast-1.amazonaws.com";
	char port[128]="443";
	char ca_file[128]="/etc/iotcoreca.crt";
	char clt_key[128]="/etc/iotcoreclient.key";
	char clt_cert[128]="/etc/iotcoreclient.crt";
#else
 	char addr[128]="test.mosquitto.org";
 	char port[128]="8883";
	char ca_file[128]="/etc/mosquitto.org.crt";
	char clt_key[128]="/etc/client.key";
	char clt_cert[128]="/etc/client.crt";
#endif
	char devid[128]="maomaoag";
	memset(p,0,sizeof(agent));
	if (argc>1&&strlen(argv[1]))
		strcpy(devid,argv[1]);
	dbg_printf("set devid [%s]\n",devid);
	strcpy(p->devid,devid);
	strcpy(p->mqrecv.daddr,addr);
	strcpy(p->mqupld.daddr,addr);
	strcpy(p->mqrecv.dport,port);
	strcpy(p->mqupld.dport,port);
#if fuckuele
	sprintf(p->mqrecv.cltid,"%s",devid);
	sprintf(p->mqrecv.topicsend,"$aws/things/%s/shadow/update",p->mqrecv.cltid);
	sprintf(p->mqrecv.topicrecv,"$aws/things/%s/shadow/update/accepted",p->mqrecv.cltid);
	sprintf(p->mqupld.cltid,"%s_uploader",devid);
	sprintf(p->mqupld.topicsend,"devices/%s/message",p->mqupld.cltid);
#endif
	strcpy(p->mqrecv.ca_file,ca_file);
	strcpy(p->mqrecv.clt_key,clt_key);
	strcpy(p->mqrecv.clt_cert,clt_cert);
	strcpy(p->mqupld.ca_file,ca_file);
	strcpy(p->mqupld.clt_key,clt_key);
	strcpy(p->mqupld.clt_cert,clt_cert);
	
    return ret;
}
/*****************************************************************************/
int main(int argc, char** argv)
{
	int ret=EXIT_FAILURE;
    agent* p=&g_agent;
	signal(SIGINT, handle_stop);
	signal(SIGTERM, handle_stop);
	signal(SIGUSR2, handle_debug);
	signal(SIGPIPE, handle_pipe);
    /* Load OpenSSL */
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    SSL_library_init();
    if(!cfg_agInit(p,argc,argv))
    {
	  fprintf(stderr,"set agint error!\n");
      goto out;
    }
#if 1// init ctrl if
	interfaces.count=1;//just one interface
	memset(&admcd, 0, sizeof(admlinkdaemon));
    interfaces.admcd = (admlinkdaemon **)&admcd;
	printf("CTRL_INTERFACE:[%s]  CTRL_INTERFACE_IF:[%s]\n", CTRL_INTERFACE, CTRL_INTERFACE_IF);
	interfaces.admcd[0] = &admcd;
	if(!interfaces.admcd[0])
		goto out;
	if(admlinkdaemon_intf_list[0].init(interfaces.admcd[0]) < 0)//cread  sock for read ctrl
		goto out;
#endif
	eloop_init(NULL);
	eloop_register_read_sock(admcd.ctrl_sock, admlink_ctrl_iface_receive, p, NULL);
    eloop_register_timeout(1,0,chk_agstatm,p,NULL);//start state machine(sm) after 1 s
    eloop_run();

    out:
		if(interfaces.admcd[0])
			admlinkdaemon_intf_list[0].deinit(interfaces.admcd[0]);
		eloop_destroy();
		close_conn(&p->mqrecv);
		close_conn(&p->mqupld);
    return ret;
}

