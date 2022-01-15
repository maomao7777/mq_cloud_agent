/** -------------------------------------------------------------------------
                          INCLUDE HEADER FILES
  -------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>
#include <admlink.h>
#include <admlink_ctrl_if.h>
#include <admlink_ctrl.h>

/** -------------------------------------------------------------------------
                          DEFINITIONS
  -------------------------------------------------------------------------*/
/** -------------------------------------------------------------------------
                          VARIABLES
  -------------------------------------------------------------------------*/

static admlink_ctrl *ctrl_conn;
static const char *ctrl_iface_dir = CTRL_INTERFACE;
static char *ctrl_ifname = NULL;

/** -------------------------------------------------------------------------
                          FUNCTIONS
  -------------------------------------------------------------------------*/

/*****************************************************************************/
static admlink_ctrl * admlink_cli_open_connection(const char *ifname)
{
    char *cfile;
    int flen;

    if(ifname == NULL)
        return NULL;

    flen = strlen(ctrl_iface_dir) + strlen(ifname) + 2;
    cfile = malloc(flen);
    if(cfile == NULL)
        return NULL;
    snprintf(cfile, flen, "%s/%s", ctrl_iface_dir, ifname);

    ctrl_conn = admlink_ctrl_open(cfile);
    free(cfile);
    return ctrl_conn;
}

/*****************************************************************************/
static void admlink_cli_close_connection(void)
{
    if(ctrl_conn == NULL)
        return;

    admlink_ctrl_close(ctrl_conn);
    ctrl_conn = NULL;
}

/*****************************************************************************/
static void admlink_cli_msg_cb(char *msg, size_t len)
{
    printf("%s\n", msg);
}

/*****************************************************************************/
static int _admlink_ctrl_command(admlink_ctrl *ctrl, char *cmd, int print)
{
    char buf[1024];
    size_t len;
    int ret;

    if(ctrl_conn == NULL)
    {
        printf("Not connected to %s - command dropped.\n", ADMLINK_NAME);
        return -1;
    }
    len = sizeof(buf) - 1;
    ret = admlink_ctrl_request(ctrl, cmd, strlen(cmd), buf, &len,
	admlink_cli_msg_cb);
    if(ret == -2)
    {
        printf("'%s' command timed out.\n", cmd);
        return -2;
    }
    else if(ret < 0)
    {
        printf("'%s' command failed.\n", cmd);
        return -1;
    }
    if(print)
    {
        buf[len] = '\0';
    }

    if (!memcmp(buf, STR_REPLY_ERROR, strlen(STR_REPLY_ERROR)))
        ret = -10;
	printf("[%s]\n",buf);
    return ret;
}
/*****************************************************************************/
static inline int admlink_ctrl_command(admlink_ctrl *ctrl, char *cmd)
{
    return _admlink_ctrl_command(ctrl, cmd, 1);
}
/*****************************************************************************/
static int sysconf_request(admlink_ctrl *ctrl, int argc, char *argv[])
{
    int i, val;
    char buf[1024];

    val = sprintf(buf, "%s", argv[0]);

    for(i=1; i<argc; ++i)
    {
        val += sprintf(buf+val, " %s", argv[i]);
    }

    return admlink_ctrl_command(ctrl, buf);
}


/*****************************************************************************/
int main(int argc, char *argv[])
{
    int interactive;
    int warning_displayed = 0;
    int c, ret = -999;

    for(;;)
    {
        c = getopt(argc, argv, "i:p:v");
        if(c < 0)
            break;
        switch(c)
        {
        case 'v':
            return 0;
        case 'i':
            ctrl_ifname = strdup(optarg);
            break;
        case 'p':
            ctrl_iface_dir = optarg;
            break;
        default:
            return -1;
        }
    }

    interactive = argc == optind;

    for(;;)
    {
        if(ctrl_ifname == NULL)
        {
            struct dirent *dent;
            DIR *dir = opendir(ctrl_iface_dir);
            if(dir)
            {
                while((dent = readdir(dir)))
                {
                    if(strcmp(dent->d_name, ".") == 0 ||
                       strcmp(dent->d_name, "..") == 0)
                        continue;
                    //printf("Selected interface '%s'\n",
                    //     dent->d_name);
                    ctrl_ifname = strdup(dent->d_name);
                    break;
                }
                closedir(dir);
            }
        }
        ctrl_conn = admlink_cli_open_connection(ctrl_ifname);
        if(ctrl_conn)
        {
            if(warning_displayed)
                printf("Connection established.\n");
            break;
        }

        if(!interactive)
        {
            perror("Failed to connect to " ADMLINK_NAME "- "
                   "admlink_ctrl_open");
            return -1;
        }

        if(!warning_displayed)
        {
            printf("Could not connect to %s - re-trying\n",ADMLINK_NAME);
            warning_displayed = 1;
        }
        sleep(1);
        continue;
    }

    if(!ctrl_conn)
    {
        perror("Failed to connect to "ADMLINK_NAME "- admlink_ctrl_open");
        free(ctrl_ifname);
        return -1;
    }

    if(!interactive)
    {
        ret = sysconf_request(ctrl_conn, argc - optind, &argv[optind]);
    }

    free(ctrl_ifname);
    admlink_cli_close_connection();
    return ret;
}
