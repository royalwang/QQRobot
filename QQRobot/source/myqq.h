#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __WIN32__
#include <conio.h>
#include <winsock.h>
#include <wininet.h>
#include <windows.h>
#else
#include <termios.h> //read password
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <mysql.h>

#include "qqclient.h"
#include "buddy.h"
#include "qun.h"
#include "group.h"
#include "memory.h"
#include "utf8.h"
#include "config.h"
#include "qqsocket.h"
#include "qq_robot.h"
#include "debug.h"
#include "db.h"

void IncreaseLog(unsigned long nFrom,unsigned long nTo,int bSend,MYSQL * pDBConn) ;

void UpdateOnlineStatus(qqclient * qq,MYSQL * pDBConn) ;

