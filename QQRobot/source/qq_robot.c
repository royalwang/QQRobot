
#include "qq_robot.h"

qq_robot * qq_robot_new( char * psId, char * psPasswd, qq_robot * pLastItem )
{
	qq_robot * pNewItem = NULL ;
	NEW( pNewItem, sizeof(qq_robot) ) ;

	strcpy(pNewItem->psId,psId) ;
	strcpy(pNewItem->psPassword,psPasswd) ;
	pNewItem->pFirst = NULL ;
	pNewItem->pNext = NULL ;

	// 创建 qqclient 对象
	NEW( pNewItem->pQQ, sizeof(qqclient) );
	qqclient_create(pNewItem->pQQ,atoi(psId),psPasswd) ;


	if(pLastItem)
	{
		pNewItem->pFirst = pLastItem->pFirst ;
		pLastItem->pNext = pNewItem ;
	}

	return pNewItem ;
}
