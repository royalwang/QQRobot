#include "qqclient.h"
#include "memory.h"


typedef struct qq_robot
{
	char * psId ;
	char * psPassword ;
	qqclient * pQQ ;

	void * pFirst ;
	void * pNext ;
} qq_robot ;

qq_robot * qq_robot_new( char * psId, char * psPasswd, qq_robot * pLastItem ) ;
