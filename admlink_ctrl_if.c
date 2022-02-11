#include <admlink.h>
#include <admlink_misc.h>
#include <admlink_ctrl_if.h>
static char ctrl_buf[128];
int admlink_cmd_stop(agent* pag, char* recvbuf);
int admlink_cmd_sendtest(agent* pag, char* recvbuf);
/*****************************************************************************/
struct _admlcmd_tb_ele 
{
	const char* cmd;
	int (*cmd_hdl)(agent* pag,char* recvbuf);
};
typedef struct _admlcmd_tb_ele  admlcmd_tb_ele;
/*****************************************************************************/
admlcmd_tb_ele adlcmd_tbl[]=
{
	{"stop", admlink_cmd_stop},
	{"sendtest", admlink_cmd_sendtest},
	{NULL},
};
/*****************************************************************************/
int admlink_cmd_stop(agent* pag, char* recvbuf)
{
	eloop_register_timeout(0,MQTT_SYN_T,eloop_set_exit,NULL,NULL);
	return 1;
}
/*****************************************************************************/
int admlink_cmd_sendtest(agent* pag, char* recvbuf)
{
	mqconn *p =&pag->mqupld;
	char resbuf[512];
	char tmpbuf[128];
	memset(resbuf,0,sizeof(resbuf));
	memset(tmpbuf,0,sizeof(tmpbuf));
	strcat(resbuf,"{");
	snprintf(tmpbuf,sizeof(tmpbuf),"\"type\":\"event\"");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"ver\":\"2.00\"");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"prdct\":\"AP\"");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"act_id\":9999");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"act_sts\":0");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"act_trg\":0");
	strcat(resbuf,tmpbuf);
	snprintf(tmpbuf,sizeof(tmpbuf),",\"act_para\":\"Test message\"");
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
	snprintf(tmpbuf,sizeof(tmpbuf),",\"msg\":\"テスト通知\"");
	strcat(resbuf,tmpbuf);
	strcat(resbuf,"}");
	set_mqtt_publish_nexsync(p, p->topicsend,
		resbuf, MQTT_PUBLISH_QOS_1);
	return 1;
}
/*****************************************************************************/
int x_get_time_of_day(struct timeval *tv)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	tv->tv_sec =ts.tv_sec;
	tv->tv_usec=ts.tv_nsec/1000;
	return 0; /* success*/
}
/*****************************************************************************/
int admlink_cmd_handle(agent* pag ,char *buf, char *reply)
{
	printf("%s-%d, receive msg [%s]\n", __FUNCTION__, __LINE__, buf);
	//Buf and reply must exist.
	if(!buf || !reply)
		return -1;
	int n=0;
	time_t timer;
    time(&timer);
	pag->tinfo = localtime(&timer);
	while(adlcmd_tbl[n].cmd!=NULL)
	{
		if(strncmp(buf,adlcmd_tbl[n].cmd,strlen(adlcmd_tbl[n].cmd))==0)
		{

			if(!adlcmd_tbl[n].cmd_hdl(pag,buf))
				strcpy(reply,"UNKNOWN_ERR");
			else
				strcpy(reply,"OK");
			goto end;
		}
		n++;
	}
	strcpy(reply,"UNKNOWNCMD");
	end:
	return strlen (reply);
}

/*****************************************************************************/
void admlink_debug_print_timestamp(void)
{
    struct timeval tv;
    char buf[16];

    x_get_time_of_day(&tv);

    if (strftime(buf, sizeof(buf), "%b %d %H:%M:%S",
             localtime((const time_t *) &tv.tv_sec)) <= 0) {
        snprintf(buf, sizeof(buf), "%u", (int) tv.tv_sec);
    }
    printf("%s.%06u: ", buf, (unsigned int) tv.tv_usec);
}

/*****************************************************************************/
static void _admlink_hexdump_ascii(int level, const char *title, const char *buf, size_t len, int show)
{
    int i, llen;
    const char *pos = buf;
    const int line_len = 16;

    admlink_debug_print_timestamp();

    if (!show) {
        printf("%s - hexdump_ascii(len=%lu): [REMOVED]\n",
               title, (unsigned long) len);
        return;
    }
    if (buf == NULL) {
        printf("%s - hexdump_ascii(len=%lu): [NULL]\n",
               title, (unsigned long) len);
        return;
    }
    printf("%s - hexdump_ascii(len=%lu):\n", title, (unsigned long) len);
    while (len) {
        llen = len > line_len ? line_len : len;
        printf("    ");
        for (i = 0; i < llen; i++)
            printf(" %02x", pos[i]);
        for (i = llen; i < line_len; i++)
            printf("   ");
        printf("   ");
        for (i = 0; i < llen; i++) {
            if (isprint(pos[i]))
                printf("%c", pos[i]);
            else
                printf("_");
        }
        for (i = llen; i < line_len; i++)
            printf(" ");
        printf("\n");
        pos += llen;
        len -= llen;
    }
}

/*****************************************************************************/
static void admlink_hexdump_ascii(int level, const char *title, const char *buf, size_t len)
{
    _admlink_hexdump_ascii(level, title, buf, len, 1);
}

/*****************************************************************************/
static char * admlink_ctrl_iface_path(admlinkdaemon *hapd)
{
    memset(ctrl_buf, 0, 128);
    sprintf(ctrl_buf, "%s/%s", CTRL_INTERFACE, CTRL_INTERFACE_IF);
    return ctrl_buf;
}

/*****************************************************************************/
void admlink_ctrl_iface_receive(int sock, void *eloop_ctx, void *sock_ctx)
{
    char buf[256];
    int res;
	agent* p=(agent*)eloop_ctx;
    struct sockaddr_un from;
    socklen_t fromlen = sizeof(from);
    char *reply, replybuf[REPLY_SIZE];
    int reply_len;

    res = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *) &from, &fromlen);
    if(res < 0)
    {
        perror("recvfrom(ctrl_iface)");
        return;
    }
    buf[res] = '\0';
	//     admlink_hexdump_ascii(DMSG_DEBUG, "admlink RX ctrl_iface", (char *) buf, res);
    reply = &replybuf[0];

    /** Process commands only if system is not going to reboot */
    if(p->agstatus!=isReboot 
		&&p->agstatus!=isFwup)
    {
        reply_len = admlink_cmd_handle(p,buf, reply);
    }
    else
    {
        /* Just return an "ok" to cheat the caller */
        memcpy(reply, STR_REPLY_OK, strlen(STR_REPLY_OK));
        reply_len = strlen(STR_REPLY_OK);
    }

    if(reply_len < 0)
    {
        memcpy(reply, STR_REPLY_ERROR, strlen(STR_REPLY_ERROR));
        reply_len = strlen(STR_REPLY_ERROR);
    }
    sendto(sock, reply, reply_len, 0, (struct sockaddr *) &from, fromlen);
}

/*****************************************************************************/
int admlink_ctrl_iface_init(admlinkdaemon *admlinkd)
{
    struct sockaddr_un addr;
    int s = -1;
    char *fname = NULL;

    admlinkd->ctrl_sock = -1;
     if(mkdir(CTRL_INTERFACE, S_IRWXU | S_IRWXG) < 0)
    {
        if(errno == EEXIST)
        {
            printf("Using existing control interface directory.\n");
        }
        else
        {
            perror("mkdir[ctrl_interface]");
            goto fail;
        }
    }

	s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if(s < 0)
    {
        perror("socket(PF_UNIX)");
        goto fail;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    fname = admlink_ctrl_iface_path(admlinkd);

    strncpy(addr.sun_path, fname, sizeof(addr.sun_path));
    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        perror("bind(PF_UNIX)");
        goto fail;
    }

    if(chmod(fname, S_IRWXU | S_IRWXG) < 0)
    {
        perror("chmod[ctrl_interface/ifname]");
        goto fail;
    }

    admlinkd->ctrl_sock = s;
    //eloop_register_read_sock(s, admlink_ctrl_iface_receive, admlinkd, NULL);//maomao regestier after init for using cust eloop_ctx
    return 0;

fail:
    if(s >= 0)
        close(s);

    if(fname)
    {
        unlink(fname);
    }

    return -1;
}

/*****************************************************************************/
void admlink_ctrl_iface_deinit(admlinkdaemon *admlinkd)
{
    struct admlink_ctrl_dst *dst, *prev;

    if (admlinkd->ctrl_sock > -1) {
        char *fname;
        eloop_unregister_read_sock(admlinkd->ctrl_sock);
        close(admlinkd->ctrl_sock);
        admlinkd->ctrl_sock = -1;
        fname = admlink_ctrl_iface_path(admlinkd);
        if (fname)
            unlink(fname);

        rmdir(CTRL_INTERFACE);
    }

    dst = admlinkd->ctrl_dst;
    while (dst) {
        prev = dst;
        dst = dst->next;
        free(prev);
    }
}
