/**********************************************************
*	rwlock.c       //added by mingxuan 2019-4-8
***********************************************************/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "global.h"
#include "rwlock.h"

void init_rwlock(struct rwlock *rw)
{
	rw->lock = RW_LOCK_DEFAULT;
}

//rwlock暂无用处，若使用，还需调试