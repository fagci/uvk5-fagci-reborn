#ifndef SVC_LISTEN_H
#define SVC_LISTEN_H

#include "helper/lootlist.h"
void SVC_LISTEN_Init();
void SVC_LISTEN_Update();
void SVC_LISTEN_Deinit();

extern Loot *(*gListenFn)();

#endif /* end of include guard: SVC_LISTEN_H */
