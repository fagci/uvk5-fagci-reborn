#ifndef SVC_RENDER_H
#define SVC_RENDER_H

#include <stdbool.h>
#include <stdint.h>

extern uint32_t gLastRender;

void SVC_RENDER_Init(void);
void SVC_RENDER_Update(void);
void SVC_RENDER_Deinit(void);

#endif /* end of include guard: SVC_RENDER_H */
