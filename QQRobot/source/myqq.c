/*
 *  myqq.c
 *
 *  main() A small console for MyQQ.
 *
 *  Copyright (C) 2008  Huang Guan (gdxxhg@gmail.com)
 *
 *  2008-01-31 Created.
 *  2008-2-5   Add more commands.
 *  2008-7-15  Mr. Wugui said "There's no accident in the world.", 
 *		   but I always see accident in my code :)
 *  2008-10-1  Character color support and nonecho password.
 *  2009-1-25  Use UTF-8 as a default type.
 *
 *  Description: This file mainly includes the functions about 
 *			   Parsing Input Commands.
 *		 Warning: this file should be in UTF-8.
 *			   
 */


#include "myqq.h"


static MYSQL * pDBConnForRecvTread ;


#define MSG need_reset = 1; setcolor( color_index ); printf

#define QUN_BUF_SIZE	80*100
#define BUDDY_BUF_SIZE	80*500
#define PRINT_BUF_SIZE	80*500*3
#define QUN_CMD_STRLEN	31

static char psRobotVersion[30] = "0.1.3" ;

static char psRobotVersionLog[50][1024] = {
	"0.1.1 在 robots_status 中记录 robot 版本"
	"0.1.2 在 inbox 中保存 sSId 信息"
	"0.1.3 在 robots_quns 中保存 robot 所加入的群"
	"\0"
} ;


static char* qun_buf, *buddy_buf, *print_buf;
static uint to_uid = 0;		//send message to that guy.
static uint qun_int_uid;		//internal qun id if entered.
static char input[1024];
static int enter = 0;
static qqclient* qq;
static int need_reset, no_color = 0;

static char psQunCmds[30][31] ;
static int nMaxQunCmdIndx = 30 ;


static char psSId[33];


#ifdef __WIN32__
#define CCOL_GREEN	FOREGROUND_GREEN
#define CCOL_LIGHTBLUE	FOREGROUND_BLUE | FOREGROUND_GREEN
#define CCOL_REDGREEN	FOREGROUND_RED | FOREGROUND_GREEN
#define CCOL_NONE		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
static int color_index = CCOL_NONE; 
static void charcolor( int col )
{
	color_index = col;
}
static void setcolor( int col )
{
	if(!no_color)
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_INTENSITY | col);
}
static void clear_screen()
{
	system("cls");
}
#else

#define CCOL_NONE		"\033[0m"   
#define CCOL_BLACK	"\033[0;30m"   
#define CCOL_DARKGRAY	"\033[1;30m"   
#define CCOL_BLUE		"\033[0;34m"   
#define CCOL_LIGHTBLUE	"\033[1;34m"   
#define CCOL_GREEN	"\033[0;32m"   
#define CCOL_LIGHTGREEN	"\033[1;32m"   
#define CCOL_CYAN		"\033[0;36m"   
#define CCOL_LIGHTCYAN	"\033[1;36m"   
#define CCOL_RED		"\033[0;31m"   
#define CCOL_LIGHTRED	"\033[1;31m"   
#define CCOL_PURPLE	"\033[0;35m"   
#define CCOL_LIGHTPURPLE	"\033[1;35m"   
#define CCOL_LIGHTBROWN	"\033[0;33m"   
#define CCOL_YELLOW	"\033[1;33m"   
#define CCOL_LIGHTGRAY	"\033[0;37m"   
#define CCOL_WHITE	"\033[1;37m"
#define CCOL_REDGREEN	CCOL_YELLOW
static char* color_index = CCOL_NONE;
static void charcolor( const char* col )
{
	color_index = (char*)col;
}
static void setcolor( const char* col )
{
	if(!no_color)
		printf( col );
}
static void clear_screen()
{
	//system("clear");
	printf("\033[2J   \033[0;0f");
}
#endif

//含颜色控制 
#define RESET_INPUT charcolor( CCOL_NONE ); \
	fflush( stdout ); 






#ifdef __WIN32__
#define _TEXT to_gb_force 
static char* to_gb_force( char* s )
{
//	memset( print_buf, 0, PRINT_BUF_SIZE );
	utf8_to_gb( s, print_buf, PRINT_BUF_SIZE-1 );
	return print_buf;
}

static char* to_utf8( char* s )
{
//	memset( print_buf, 0, PRINT_BUF_SIZE );
	gb_to_utf8( s, print_buf, PRINT_BUF_SIZE-1 );
	return print_buf;
}
#else
#define _TEXT
#endif

static char* skip_line( char* p, int ln )
{
	while( *p && ln-- )
	{
		p ++;
		while( *p && *p!='\n' ) p++;
	}
	return p;
}
static char* myqq_get_buddy_name( qqclient* qq, uint uid )
{
	static char tmp[16];
	qqbuddy* b = buddy_get( qq, uid, 0 );
	if( b )
		return b->nickname;
	if( uid == 10000 )
		return "System";
	if( uid != 0 ){
		sprintf( tmp, "%u" , uid );
		return tmp;
	}
	return "Nobody";
}
static char* myqq_get_qun_name( qqclient* qq, uint uid )
{
	static char tmp[16];
	qqqun* q = qun_get( qq, uid, 1 );
	if( q )
		return q->name;
	if( uid != 0 ){
		sprintf( tmp, "%u" , uid );
		return tmp;
	}
	return "[q==NULL]";
}
static char* myqq_get_qun_member_name( qqclient* qq, uint int_uid, uint uid )
{
	static char tmp[16];
	qqqun* q = qun_get( qq, int_uid, 0 );
	if( q ){
		qunmember* m = qun_member_get( qq, q, uid, 0 );
		if( m )
			return m->nickname;
		if( uid != 0 ){
			sprintf( tmp, "%u" , uid );
			return tmp;
		}
		return "[m==NULL]";
	}
	return "[q==NULL]";
}
static int myqq_send_im_to_qun( qqclient* qq, uint int_uid, char* msg, int wait )
{
	qun_send_message( qq, int_uid, msg );
	if( wait )
	{
		if( qqclient_wait( qq, 15 ) < 0 )
			return -1;
	}
	return 0;
}
static int myqq_send_im_to_buddy( qqclient* qq, uint int_uid, char* msg, int wait )
{
	buddy_send_message( qq, int_uid, msg );
	if( wait )
	{
		if( qqclient_wait( qq, 15 ) < 0 )
			return -1;
	}
	return 0;
}



//Note: this function change the source string directly.
static char* util_escape( char* str )
{
	unsigned char* ch;
	ch = (unsigned char*)str;
	while( *ch )
	{
		if( *ch < 0x80 ) //ASCII??
		{
			if( !isprint(*ch) ) //if not printable??
				*ch = ' ';	//use space instead..
			ch ++;	//skip one
		}else{
			ch +=2; //skip two
		}
	}
	return str;
}


// 将 qq 所加入的群保存到数据库
void SaveQuns(qqclient * qq,MYSQL * pDBConn)
{
	if(qq->process!=P_LOGIN)
	{
		printf("账号不在登陆状态。\r\n") ;
		return ;
	}

	printf("记录已经加入的群。\r\n") ;

	// 清除db中已有的记录
	char psSQL[128] ;
	sprintf(psSQL,"delete from robots_quns where sIM='qq' and sRId='%d' ;",qq->number) ;

	db_query(psSQL,pDBConn) ;

	
	int i, len = 0, ln=1;
	//avoid editing the array
	// buf[0] = 0;
	pthread_mutex_lock( &qq->qun_list.mutex ) ;

	for( i=0; i<qq->qun_list.count; i++ )
	{
		qqqun * q = (qqqun *)qq->qun_list.items[i] ;
		
		printf("%d群[%d]：%s\r\n",i,q->ext_number,q->name) ;
		
		char * psQunNameEscaped = NULL ; // 4K * 2 + 1
		NEW(psQunNameEscaped,strlen(q->name)*2+1) ;
		mysql_escape_string(psQunNameEscaped,q->name,strlen(q->name));
		
		sprintf(psSQL,"insert into robots_quns (sIM,sRId,sQunId,sQunName) values ('qq','%d','%d','%s');",qq->number,q->ext_number,psQunNameEscaped) ;
		db_query(psSQL,pDBConn) ;

		DEL(psQunNameEscaped) ;
	}

	pthread_mutex_unlock( &qq->qun_list.mutex ) ;
}

void buddy_msg_callback ( qqclient* qq, uint uid, time_t t, char* msg, qqmessage* pMsg )
{
	char* nick = myqq_get_buddy_name( qq, uid );

	IncreaseLog(qq->number,uid,0,pDBConnForRecvTread) ;

	// 保存到数据库
	char * psMsgEscaped = NULL ; // 4K * 2 + 1
	NEW(psMsgEscaped,strlen(msg)*2+1) ;
	mysql_escape_string(psMsgEscaped,msg,strlen(msg));

	char * psFromNickname = NULL ;
	NEW(psFromNickname,strlen(nick)*2+1) ;
	mysql_escape_string(psFromNickname,nick,strlen(nick)) ;

	int nProtType = 0 ;
	int nClientVer = 0 ;
	if(pMsg)
	{
		nProtType = pMsg->auto_reply ;
		nClientVer = pMsg->im_type ;
	}

	char psSQL[5120] ;
	sprintf(psSQL,"insert into inbox (sIM,sFrom,sFromNickname,sTo,sMsg,bQun,nProtType,nCreateTime,sClientVersion,sSId) values ('qq',%d,'%s',%d,'%s','0',%d,%d,'%d','%s') ;",uid,psFromNickname,qq->number,psMsgEscaped,nProtType,t,nClientVer,psSId) ;
	// printf(psSQL) ;	
	if(db_query(psSQL,pDBConnForRecvTread))
	{
		printf("保存消息到 inbox 时遇到错误：%s\r\n",mysql_error(pDBConnForRecvTread)) ;
	}

	DEL(psMsgEscaped) ;
	DEL(psFromNickname) ;

	// active 记录
	time_t nNow = 0 ; 
	time(&nNow); 
	sprintf(psSQL,"update robot_status set nActiveTime=%d where sIM='qq' and sRId=%d",nNow,qq->number) ;
	// printf(psSQL) ;
	db_query(psSQL,pDBConnForRecvTread) ;


	// output
	char timestr[12];
	struct tm * timeinfo;
  	timeinfo = localtime ( &t );
	strftime( timestr, 12, "%H:%M:%S", timeinfo );

	printf("[%s][%s][ClientVersion:%d]收到用户消息：\r\n%s\r\n",timestr,nick,nClientVer,msg) ;
}


void qun_msg_callback ( qqclient* qq, uint uid, uint int_uid, time_t t, char* msg )
{
	// 检查 群命令
	char psCmdTemp[QUN_CMD_STRLEN] ;
	psCmdTemp[0] = '\0' ;

	int bIgnore = 1 ;
	int nCmdIdx = 0 ;
	for(nCmdIdx=0;nCmdIdx<nMaxQunCmdIndx;nCmdIdx++)
	{
		int nCmdLen = strlen(psQunCmds[nCmdIdx]) ;
		if( nCmdLen<=0 || nCmdLen>QUN_CMD_STRLEN-1 )
		{
			continue ;
		}
		
		// printf("\r\n%d:%d:%s\n",nCmdIdx,nCmdLen,psQunCmds[nCmdIdx]) ;

		psCmdTemp[0] = '\0' ;
		strncpy(psCmdTemp,msg,nCmdLen) ;
		psCmdTemp[nCmdLen] = '\0' ;
		// printf("msg header: %s\n",psCmdTemp) ;

		int nCmpRes = stricmp(psCmdTemp,psQunCmds[nCmdIdx]) ;
		// printf("%d=>%s(%d):%s(%d)\n",nCmpRes,psCmdTemp,strlen(psCmdTemp),psQunCmds[nCmdIdx],strlen(psQunCmds[nCmdIdx])) ;

		if(nCmpRes==0)
		{
			printf("bingo!\n") ;
			bIgnore = 0 ;
			break ;
		}
	}

	// 忽略消息
	if(bIgnore>0)
	{
		printf("x忽略消息x") ;
		return ;
	}




	char* qun_name = myqq_get_qun_name( qq, int_uid );
	char* nick = myqq_get_qun_member_name( qq, int_uid, uid );

	// 找到群的群号
	qqqun * pQun = qun_get(qq,int_uid,0) ;
	if( !pQun )
	{
		printf("收到来自群(内部号码：%u)的消息，但是找不到群的信息。\r\n") ;
	}
	else
	{
		IncreaseLog(qq->number,pQun->ext_number,0,pDBConnForRecvTread) ;

		// 保存到数据库
		char * psMsgEscaped = NULL ; // 4K * 2 + 1
		NEW(psMsgEscaped,strlen(msg)*2+1) ;
		mysql_escape_string(psMsgEscaped,msg,strlen(msg));

		char * psFromNickname = NULL ;
		NEW(psFromNickname,strlen(nick)*2+1) ;
		mysql_escape_string(psFromNickname,nick,strlen(nick)) ;

		char psSQL[5120] ;
		sprintf(psSQL,"insert into inbox (sIM,sFrom,sFromNickname,sFromQunUser,sTo,sMsg,bQun,nCreateTime,sSId) values ('qq',%u,'%s',%u,%u,'%s','1',%d,'%s') ;",pQun->ext_number,psFromNickname,uid,qq->number,psMsgEscaped,t,psSId) ;
		// printf(psSQL) ;	
		if(db_query(psSQL,pDBConnForRecvTread))
		{
			printf("保存消息到 inbox 时遇到错误：%s\r\n",mysql_error(pDBConnForRecvTread)) ;
		}

		DEL(psMsgEscaped) ;
		DEL(psFromNickname) ;

		// active 记录
		time_t nNow = 0 ; 
		time(&nNow); 
		sprintf(psSQL,"update robot_status set nActiveTime=%u where sIM='qq' and sRId=%u",nNow,qq->number) ;
		// printf(psSQL) ;
		db_query(psSQL,pDBConnForRecvTread) ;
	}


	
	// output
	char timestr[12];
	struct tm * timeinfo;
  	timeinfo = localtime ( &t );
	strftime( timestr, 12, "%H:%M:%S", timeinfo );

	printf("[%s][%s][%s]收到群消息：\r\n%s\r\n",timestr,qun_name,nick,msg) ;
}

void buddy_add_callback ( struct qqclient* qq, uint uid, int nType )
{
	// 检查是否为订阅用户
	if(nType==40)
	{
		char psSQL[5120] ;
		sprintf(psSQL,"select * from service_subscriptions where sRId='%u' and sIMId='%u' and sIM='qq' ;",qq->number,uid) ;
		if(db_query(psSQL,pDBConnForRecvTread))
		{
			printf("SQL:%s\r\nError:%s",psSQL,mysql_error(pDBConnForRecvTread)) ;
		}
		else
		{
			MYSQL_RES * res = mysql_store_result(pDBConnForRecvTread) ;
			int nRow = mysql_num_rows(res) ;
			mysql_free_result(res) ;
			if(nRow)
			{
				printf("自动加对方[%u]为好友\r\n",uid) ;
				char * sAddMsg = "" ;
				qqclient_add(qq,uid,sAddMsg) ;
			}
			else
			{
				printf("忽略未知用户[%u] 的好友添加\r\n",uid) ;
			}
		}
	}
}

void qun_approve_apply_to_qun ( struct qqclient* qq )
{
	SaveQuns(qq,pDBConnForRecvTread) ;
}
void qun_delete_from_qun ( struct qqclient* qq )
{
	SaveQuns(qq,pDBConnForRecvTread) ;
}

#ifndef __WIN32__
int read_password(char   *lineptr )   
{   
	struct   termios   old,   new;   
	int   nread;	
	/*   Turn   echoing   off   and   fail   if   we   can't.   */   
	if   (tcgetattr   (fileno   (stdout),   &old)   !=   0)   
		return   -1;   
	new   =   old;   
	new.c_lflag   &=   ~ECHO;   
	if   (tcsetattr   (fileno   (stdout),   TCSAFLUSH,   &new)   !=   0)   
		return   -1;
	/*   Read   the   password.   */   
	nread   =   scanf   ("%31s", lineptr);	
	/*   Restore   terminal.   */   
	(void)   tcsetattr   (fileno   (stdout),   TCSAFLUSH,   &old); 	
	return   nread;   
}
#endif


void IncreaseLog(unsigned long nFrom,unsigned long nTo,int bSend,MYSQL * pDBConn)
{
	time_t nNow = 0 ;
	time(&nNow) ;
	struct tm * t = localtime(&nNow) ;
	char psToday[12] ;
	sprintf(psToday,"%d-%d-%d",t->tm_year+1900 ,t->tm_mon+1,t->tm_mday) ;


	char psSQL[5120] ;
	sprintf(psSQL,"select * from log_daily_summary where sFrom='%d' and sTo='%d' and sIM='qq' and Date='%s'",nFrom,nTo,psToday) ;
	// fprintf(stderr, "%s\n", psSQL);
	if (db_query(psSQL,pDBConn))
	{
		fprintf(stderr, "%s:%s\n", mysql_error(pDBConn),psSQL);
		exit(1);
	}

	MYSQL_RES *res;
	res = mysql_store_result(pDBConn) ;


	char psFieldName[10] ;
	strcpy(psFieldName, (bSend?"nSend":"nReceive")) ;
	//char psFieldNameLastTime[20] ;
	//strcpy(psFieldNameLastTime, (bSend?"nSendLastTime":"nReceiveLastTime")) ;

	int nNumRows = mysql_num_rows(res) ;
	mysql_free_result(res) ;


	// 新建记录
	if(nNumRows==0)
	{
		sprintf(psSQL,"insert into log_daily_summary (sIM,sFrom,sTo,Date,%s) values ('qq','%d','%d','%s',1)",psFieldName,nFrom,nTo,psToday) ;
		fprintf(stderr, "%s\n", psSQL);
		if (db_query(psSQL,pDBConn))
		{
			fprintf(stderr, "%s:%s\n", mysql_error(pDBConn),psSQL);
			exit(1);
		}
	}

	// 更新记录
	else
	{
		sprintf(psSQL,"update log_daily_summary set %s=%s+1 where sFrom='%d' and sTo='%d' and sIM='qq' and Date='%s'",psFieldName,psFieldName,nFrom,nTo,psToday) ;
		//fprintf(stderr, "%s\n", psSQL);
		if (db_query(psSQL,pDBConn))
		{
			fprintf(stderr, "%s:%s\n", mysql_error(pDBConn),psSQL);
			exit(1);
		}
	}
}


// 从数据库中取得 人工输入的验证码
int GetVerifyCodeFromDB(unsigned long nQQNum, char * psVerifyCode,MYSQL * pDBConn)
{
	char psSQL[5120] ;
	sprintf(psSQL,"SELECT `sInputedVerifyCode` FROM `robot_status` WHERE `sRId`='%ld' AND `sIM`='qq'", nQQNum);

	if (db_query(psSQL, pDBConn))
	{
		fprintf(stderr, "%s:%s\n", mysql_error(pDBConn), psSQL);
		exit(1);
	}

	MYSQL_RES *res = mysql_store_result(pDBConn);
	MYSQL_ROW row = mysql_fetch_row(res);

	strcpy(psVerifyCode, row[0]);

	mysql_free_result(res);

	// 用户未输入
	if( strlen(psVerifyCode)==0 )
	{
		printf(".");
		sleep(2);
		RESET_INPUT;

		return 0;
	}
	
	// 清空字段
	sprintf(psSQL, "UPDATE `robot_status` SET `sInputedVerifyCode`='' WHERE `sRId`='%ld' AND `sIM`='qq'", nQQNum);
	db_query(psSQL,pDBConn);

	return 1;
}

/* 更新QQ在线状态 */
void UpdateOnlineStatus(qqclient * qq, MYSQL * pDBConn)
{
	time_t nNow = 0 ;
	time(&nNow);

	char psSQL[5120] ;
	sprintf(psSQL, "UPDATE `robot_status` SET `nOnlineTime`=%ld, `nProtStatus`=%d, `sVersion`='%s' WHERE `sIM`='qq' AND `sRId`='%d'", nNow, qq->process, psRobotVersion, qq->number);
	db_query(psSQL,pDBConn);
    
	printf("o") ;
}

/* 保存QQ状态 */
void SaveStatus(qqclient * qq, MYSQL * pDBConn)
{
	char psSQL[512] ;
    
    if (qq->number > 0) {
        sprintf(psSQL, "SELECT count(`sRId`) FROM `robot_status` WHERE `sIM`='qq' AND `sRId`='%d'", qq->number);
        if (db_query(psSQL, pDBConn)) {
            sprintf(psSQL, "INSERT `robot_status` (`sIM`, `sRId`, `nProtStatus`) VALUES ('qq','%d','%d');", qq->number, qq->process);
            
            //printf("%s\n",psSQL) ;
            if(db_query(psSQL, pDBConn))
            {
                sprintf(psSQL,"UPDATE `robot_status` SET `nProtStatus`=%d WHERE `sIM`='qq' AND `sRId`='%d'", qq->process, qq->number);
                //printf("%s\n",psSQL) ;
                db_query(psSQL,pDBConn) ;
            }
        }
        
        
    }
}

void InitQunCmds(uint nRId, MYSQL * pDBConn)
{
	// 从 outbox 取得消息发送
	char psSQL[128] ;
	sprintf(psSQL,"SELECT sSId FROM robots where sRId='%d' and sIM='qq' limit 0,1",nRId) ;
	if (db_query(psSQL,pDBConn))
	{
		fprintf(stderr, "%s\n", mysql_error(pDBConn));
		exit(1);
	}

	MYSQL_RES *res;
	MYSQL_ROW rowRobot;

	res = mysql_store_result(pDBConn);
	if ((rowRobot=mysql_fetch_row(res)) == NULL)
	{
		mysql_free_result(res) ;
		return ;
	}
	mysql_free_result(res) ;

	
	char psTemp[128] ;
	char psSQLCmd[512] = "SELECT " ;

	int nCmdIdx = 0 ;
	for(nCmdIdx=0;nCmdIdx<nMaxQunCmdIndx;nCmdIdx++)
	{
		if(nCmdIdx>0)
		{
			strcat(psSQLCmd,",") ;
		}
		sprintf(psTemp,"sCmd%d",nCmdIdx+1) ;
		strcat(psSQLCmd,psTemp) ;
	}

	sprintf(psTemp," FROM qun_cmds where sSId='%s' limit 0,1",rowRobot[0]) ;
	strcat(psSQLCmd,psTemp) ;
	if (db_query(psSQLCmd,pDBConn))
	{
		fprintf(stderr, "%s\n", mysql_error(pDBConn));
		exit(1);
	}

	MYSQL_RES *resCmds;
	MYSQL_ROW rowCmds ;
	resCmds = mysql_store_result(pDBConn);
	if ((rowCmds=mysql_fetch_row(resCmds)) == NULL)
	{
		mysql_free_result(resCmds) ;
		return ;
	}
	mysql_free_result(resCmds) ;

	for(nCmdIdx=0;nCmdIdx<nMaxQunCmdIndx;nCmdIdx++)
	{
		strcpy(psQunCmds[nCmdIdx],rowCmds[nCmdIdx]) ;
		if(strlen(psQunCmds[nCmdIdx]))
		{
			printf("Qun Cmd: %s\r\n",psQunCmds[nCmdIdx]) ;
		}
	}
}


int main(int argc, char** argv)
{
	pDBConnForRecvTread = db_init();

	MYSQL * pDBConn = db_init() ;
	MYSQL_RES *res;
	MYSQL_ROW row;



	srand((unsigned)time(NULL));
	//init data
	config_init();
	qqsocket_init();
	//no color?
	if( config_readint( g_conf, "NoColor" ) )
		no_color = 1;
    
	NEW( qun_buf, QUN_BUF_SIZE );
	NEW( buddy_buf, BUDDY_BUF_SIZE );
	NEW( print_buf, PRINT_BUF_SIZE );
	NEW( qq, sizeof(qqclient) );
    
	if( !qun_buf || !buddy_buf || !print_buf || !qq ){
		MSG("no enough memory.");
		return -1;
	}

	//login
	// clear_screen();

	uint uid;
	char password[32];

	if( argc < 3 )
	{
		printf("缺少QQ账号或密码。");
		exit(0) ;
	}
    else
    {
		//check if failed here...
//		uid = atoi(argv[1]);
        uid = atoi("119185121");

		// 初始化 log 
		init_file_path(uid);

//		strcpy(password, argv[2]) ;
        strcpy(password, "oywenjun99269853");
		qqclient_create( qq, uid, password );
        
		if( argc > 3 )
		{
			strcpy(psSId,argv[3]) ;
			printf("所属服务ID：%s\r\n", psSId) ;
		}
        
		if( argc > 4 )
		{
			qq->mode = atoi(argv[4])!=0 ? QQ_HIDDEN : QQ_ONLINE;
		}
        
		qqclient_login( qq );
		argc = 1;
	}


	// 初始化群命令
	InitQunCmds(uid,pDBConn) ;


	int bAlertedInputVerifyCode = 0 ;
	char psInputedVerifyCode[15] ;
	psInputedVerifyCode[0] = '\0' ;

	printf("登陆中[server: %d.%d.%d.%d:%d]...\n"
			, (qq->server_ip&0xFF000000)>>24
			, (qq->server_ip&0xFF0000)>>16
			, (qq->server_ip&0xFF00)>>8
			, (qq->server_ip&0xFF)
			, qq->server_port ) ;

	
	SaveStatus(qq,pDBConn) ;

	for( ;qq->process!= P_LOGIN; qqclient_wait(qq,1)/*wait one second*/ )
	{
		switch( qq->process )
		{
		case P_LOGGING:
			continue;
		case P_VERIFYING:

			if(bAlertedInputVerifyCode==0)
			{
				printf("请输入验证码（验证码目录下）: ");
				bAlertedInputVerifyCode = 1 ;
			}

			if( GetVerifyCodeFromDB(uid,psInputedVerifyCode,pDBConn)>0 )
			{
				printf("取得了验证码：%s\n",psInputedVerifyCode) ;

				qqclient_verify( qq, *(uint*)psInputedVerifyCode ) ;
				bAlertedInputVerifyCode = 0 ;
			}
			break ;

		case P_ERROR:

			MSG(_TEXT("网络错误.\n"));
			//qqclient_logout( qq );
			//qqclient_cleanup( qq );
			goto DO_EXIT ;

		case P_DENIED:
			MSG(_TEXT("您的QQ需要激活(http://jihuo.qq.com).\n"));
			//qqclient_logout( qq );
			//qqclient_cleanup( qq );
			goto DO_EXIT;

		case P_WRONGPASS:
			MSG(_TEXT("您的密码错误.\n"));
			//qqclient_logout( qq );
			//qqclient_cleanup( qq );
			goto DO_EXIT;

		default:
			MSG(_TEXT("未知错误.\n"));
			goto DO_EXIT;
		}
		
	}

	// 清楚验证码PNG，如果存在
	char psVerifyPngPath[256] ;
	sprintf(psVerifyPngPath,"verify/%d.png",uid) ;
	unlink(psVerifyPngPath) ;


	printf("[%d]已经登录\r\n",uid) ;


	char psSQL[5120] ;

	// 清除下线期间 队列中堆积的消息
	sprintf(psSQL,"delete from outbox where sFrom='%d' and sIM='qq'",qq->number) ;
	db_query(psSQL,pDBConn) ;


	// 创建对应的 robot_status 记录，如果不存在	
	sprintf(psSQL,"insert robot_status (sIM,sRId) values ('qq','%d')",qq->number) ;
	db_query(psSQL,pDBConn) ;
	printf("清空了消息队列里积累的 %llu 条消息\n", mysql_affected_rows(pDBConn));


	// 初次更新
	UpdateOnlineStatus(qq,pDBConn) ;





	// 更新自己的 nickname 和 签名
	buddy_update_info(qq,buddy_get(qq,qq->number,0));
	if( qqclient_wait(qq,10) < 0 )
	{
		MSG( _TEXT("获取自己的详细资料失败。\n") );
	}
	else
	{
		qqbuddy *b = buddy_get( qq, qq->number, 0 );

		if(!b)
		{
			MSG( _TEXT("获取自己的信息失败\n") );
		}else{

			sprintf(psSQL,"update robots set sNickname='%s', sSignature='%s' where sRId='%d' and sIM='qq'",b->nickname,b->signature,uid) ;
			if (db_query(psSQL,pDBConn))
			{
				printf("更新 robot 的信息失败\n%s\n",psSQL);
			}
			
			printf("%s：%s\r\n",b->nickname,b->signature) ;
		}
	}

	
	time_t nNow = 0 ; 
	time_t nStartTime = 0 ; 
	//time(nStartTime) ;
    time(&nNow);
    
	int bSavedQuns = 0 ;


	// 主循环
	while( qq->process != P_INIT )
	{
		if(qq->process==P_BUSY)
		{
			MSG(_TEXT("在另一地点登陆。\n"));
			goto DO_EXIT;
		}
		if(qq->process==P_ERROR)
		{
			MSG(_TEXT("网络错误。\n"));
			goto DO_EXIT;
		}


		time(&nNow); 
		

		// 启动 1 分钟后，记录该账号加入的群
		if( !bSavedQuns && nNow-nStartTime>60 )
		{
			SaveQuns(qq,pDBConn) ;
			bSavedQuns = 1 ;
		}

		// 从 outbox 取得消息发送
		char psSQL[5120] ;
		sprintf(psSQL,"SELECT nOBId,sTo,sMsg,bQun FROM outbox where sFrom='%d' and sIM='qq' and nSendTime<=%ld order by nSendTime limit 0,30", uid, nNow) ;
		// printf((uchar*)psSQL) ;
		if (db_query(psSQL,pDBConn))
		{
			fprintf(stderr, "%s\n", mysql_error(pDBConn));
			exit(1);
		}

		int nRow = 0 ;
		res = mysql_store_result(pDBConn);
		while ((row=mysql_fetch_row(res)) != NULL)
		{
			nRow ++ ;
			uint nTo = atoi(row[1]) ;

			// 在群中发消息
			if( strcmp(row[3],"1")==0 )
			{
				printf("向群[%d]发送消息>> %s \n",nTo,row[2]);

				qqqun * pQun = qun_get_by_ext(qq,nTo) ;
				if( !pQun )
				{
					printf("指派的qq服务账号没有加入该群。\r\n") ;
				}

				else
				{
					if( myqq_send_im_to_qun(qq,pQun->number,row[2],1)<0 )
					{
						printf("发送消息失败！\r\n") ;
					}
					else
					{
						if(nRow>1)
						{
							sleep(1) ;
						}

						IncreaseLog(qq->number,pQun->ext_number,1,pDBConn) ;
					}
				}
			}

			// 对用户发消息
			else
			{
				printf("向用户[%d]发送消息>> %s \n",nTo,row[2]);
				if( myqq_send_im_to_buddy(qq,nTo,row[2],1)<0 )
				{
					printf("发送消息失败！\r\n") ;
				}

				else
				{
					if(nRow>1)
					{
						sleep(1) ;
					}

					IncreaseLog(qq->number,nTo,1,pDBConn) ;
				}
			}
			
			// 删除消息
			sprintf(psSQL,"delete FROM outbox where nOBId=%s",row[0]) ;
			// printf(psSQL) ;
			if(db_query(psSQL,pDBConn))
			{
				printf("消息没有从队列里删除：%s\r\n",mysql_error(pDBConn)) ;
			}
		}


		mysql_free_result(res) ;
		if(!nRow)
		{
			printf(".") ;
			sleep(1) ;
			RESET_INPUT ;
		}

	}



DO_EXIT:
	SaveStatus(qq,pDBConn) ;
	exit(0) ;

}