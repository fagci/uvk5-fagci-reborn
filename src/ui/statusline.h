#ifndef STATUSLINE_H
#define STATUSLINE_H

void STATUSLINE_update();
void STATUSLINE_render();
void STATUSLINE_SetText(const char *pattern, ...);
void STATUSLINE_SetTickerText(const char *pattern, ...);

#endif /* end of include guard: STATUSLINE_H */
