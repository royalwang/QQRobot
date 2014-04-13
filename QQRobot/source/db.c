#include <stdlib.h>
#include "db.h"


void db_connect(MYSQL * pDBConn)
{
	char *db_server = "localhost";
	char *db_user = "root";
	char *db_password = "root";
	char *db_database = "qqrobot";
    char *socket = "/Applications/MAMP/tmp/mysql/mysql.sock";
	if (!mysql_real_connect(pDBConn, db_server, db_user, db_password, db_database, 0, socket, 0))
	{
		fprintf(stderr, "%s\n", mysql_error(pDBConn));
		exit(1);
	}
	mysql_query(pDBConn, "SET NAMES UTF8 ;") ;
}

MYSQL * db_init()
{
	MYSQL * pDBConn = mysql_init(0);
	db_connect(pDBConn);
	return pDBConn;
}

int _db_query(char * psSQL, MYSQL * pDBConn, char * psFile, char * psFunc, int nLine)
{
	int nTry = 0 ;

	while(1)
	{
		int nRes = mysql_query(pDBConn, psSQL) ;

		if(nRes > 0)
		{
			int nErr = mysql_errno(pDBConn) ;
			char * psErr = mysql_error(pDBConn) ;

			printf("[mysql error:%d][call: %s:%d  %s()]%s\n%s\n]",nErr,psFile,nLine,psFunc,psErr,psSQL) ;

			if( nErr==2013 || nErr==2006 )
			{
				nTry ++ ;
				printf("%d  mysql ...\n", nTry) ;

				sleep(nTry) ;
				db_connect(pDBConn) ;
			}
			else
			{
				return nRes ;
			}
		}
		else
		{
			return nRes ;
		}
	}
}

