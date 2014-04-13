#include <mysql.h>
#include <stdio.h>

MYSQL * db_init() ;
int _db_query(char * psSQL,MYSQL * pDBConn,char * psFile,char * psFunc, int nLine) ;

#define db_query(psSQL, pDBConn) _db_query(psSQL, pDBConn, __FILE__, (char*)__func__, __LINE__)

