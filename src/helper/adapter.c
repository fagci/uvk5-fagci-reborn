#include "adapter.h"
#include "../radio.h"

void VFO2CH(VFO *src, Preset *p, CH *dst) {
  dst->rx.f = src->rx.f;
  dst->tx.f = RADIO_GetTXFEx(src, p);
  dst->rx.code = src->rx.code;
  dst->tx.code = src->tx.code;
  dst->rx.codeType = src->rx.codeType;
  dst->tx.codeType = src->tx.codeType;
  dst->bw = p->band.bw;
  dst->power = src->power;
  dst->radio = src->radio == RADIO_UNKNOWN ? p->radio : src->radio;
  dst->modulation =
      src->modulation == MOD_PRST ? p->band.modulation : src->modulation;
}

void CH2VFO(CH *src, VFO *dst) {
  dst->rx.f = src->rx.f;
  dst->tx.f = src->tx.f;
  dst->rx.code = src->rx.code;
  dst->tx.code = src->tx.code;
  dst->rx.codeType = src->rx.codeType;
  dst->tx.codeType = src->tx.codeType;
  dst->power = src->power;
  dst->radio = src->radio;
  dst->modulation = src->modulation;
}
