#include "adapter.h"
#include "../radio.h"

void VFO2CH(VFO *src, Preset *p, CH *dst) {
  dst->fRX = src->fRX;
  dst->fTX = RADIO_GetTXFEx(src, p);
  dst->codeRx = src->codeRx;
  dst->codeTx = src->codeTx;
  dst->codeTypeRx = src->codeTypeRx;
  dst->codeTypeTx = src->codeTypeTx;
  dst->power = src->power;
}

void CH2VFO(CH *src, VFO *dst) {
  dst->fRX = src->fRX;
  dst->fTX = src->fTX;
  dst->codeRx = src->codeRx;
  dst->codeTx = src->codeTx;
  dst->codeTypeRx = src->codeTypeRx;
  dst->codeTypeTx = src->codeTypeTx;
  dst->power = src->power;
}
