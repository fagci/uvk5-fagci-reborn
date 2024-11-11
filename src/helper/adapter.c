#include "adapter.h"
#include "../radio.h"

void VFO2CH(VFO *src, Preset *p, CH *dst) {
  dst->rxF = src->rxF;
  dst->txF = RADIO_GetTXFEx(src, p);
  dst->code.rx.value = src->code.rx.value;
  dst->code.tx.value = src->code.tx.value;
  dst->code.rx.type = src->code.rx.type;
  dst->code.tx.type = src->code.tx.type;
  dst->bw = p->bw;
  dst->power = src->power;
  dst->radio = src->radio == RADIO_UNKNOWN ? p->radio : src->radio;
  dst->modulation =
      src->modulation == MOD_PRST ? p->modulation : src->modulation;
}

void CH2VFO(CH *src, VFO *dst) {
  dst->rxF = src->rxF;
  dst->txF = src->txF;
  dst->code.rx.value = src->code.rx.value;
  dst->code.tx.value = src->code.tx.value;
  dst->code.rx.type = src->code.rx.type;
  dst->code.tx.type = src->code.tx.type;
  dst->power = src->power;
  dst->radio = src->radio;
  dst->modulation = src->modulation;
}
