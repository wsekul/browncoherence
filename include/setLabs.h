#ifndef SETLABS_H
#define SETLABS_H

#define finalPuncP 0
#define dtP 1
#define ccP 2
#define modalP 3
#define negP 4
#define relP 5
#define aspectP 6
#define quoteP 7

#define openCloseQ 1
#define openQ 2
#define closeQ 3
#define infV 1
#define perfV 2
#define progV 3
#define passiveV 4
#define doV 5
#define theV 1
#define aV 2

#include "SymbolTable.h"

extern stIndex dolLabel;
extern stIndex blankLabel;
extern stIndex auxLabel;
extern stIndex auxgLabel;
extern stIndex npLabel;
extern stIndex nnpLabel;
extern stIndex nnLabel;
extern stIndex nnsLabel;
extern stIndex nnpsLabel;
extern stIndex ppLabel;
extern stIndex ccLabel;
extern stIndex cdLabel;
extern stIndex prpLabel;
extern stIndex prpdLabel;
extern stIndex conjpLabel;
extern stIndex commaLabel;
extern stIndex mdLabel;
extern stIndex exLabel;
extern stIndex wdtLabel;
extern stIndex dtLabel;
extern stIndex pdtLabel;
extern stIndex whnpLabel;
extern stIndex whppLabel;
extern stIndex whadvpLabel;
extern stIndex wpdLabel;
extern stIndex wrbLabel;
extern stIndex wpLabel;
extern stIndex sLabel;
extern stIndex s1Label;
extern stIndex vpLabel;
extern stIndex vbgLabel;
extern stIndex vbLabel;
extern stIndex vbpLabel;
extern stIndex vbzLabel;
extern stIndex vbnLabel;
extern stIndex vbdLabel;
extern stIndex posLabel;
extern stIndex sbarLabel;
extern stIndex sinvLabel;
extern stIndex advpLabel;
extern stIndex rbLabel;
extern stIndex colonLabel;
extern stIndex periodLabel;
extern stIndex questionLabel;
extern stIndex exclLabel;
extern stIndex toLabel;
extern stIndex symLabel;
extern stIndex codeLabel;
extern stIndex thatLabel;
extern stIndex theLabel;
extern stIndex aLabel;
extern stIndex anLabel;
extern stIndex inLabel;
extern stIndex openLabel;
extern stIndex closedLabel;
extern stIndex halfOpenLabel;
extern stIndex halfClosedLabel;
extern stIndex notLabel;
extern stIndex ntLabel;;
extern stIndex itLabel;
extern stIndex qpLabel;
extern stIndex ofLabel;
extern stIndex persLabel;
extern stIndex orgLabel;
extern stIndex locLabel;
extern stIndex pctLabel;
extern stIndex timeLabel;
extern stIndex dateLabel;
extern stIndex mnyLabel;
extern char* pros[14];
extern stIndex prossi[27];
extern stIndex ge_prossi[14];
extern stIndex argIns[7];
extern stIndex haves[4];
extern stIndex bes[7];
extern stIndex vbns[3];
extern stIndex dos[4];
const int N_PRNS = 27;
const int N_GE_PROS = 14;

void setLabs(SymbolTable* gst);

#endif
