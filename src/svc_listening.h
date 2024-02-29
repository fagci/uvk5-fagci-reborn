#ifndef SVC_LISTEN_H
#define SVC_LISTEN_H

#include "helper/lootlist.h"
void SVC_LISTEN_Init(void);
void SVC_LISTEN_Update(void);
void SVC_LISTEN_Deinit(void);

extern Loot *(*gListenFn)(void);

#endif /* end of include guard: SVC_LISTEN_H */
