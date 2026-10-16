#ifndef CWAV_SHIT_H
#define CWAV_SHIT_H

#include <3ds.h>
#include <cwav.h>
#include <citro2d.h>
#include "../main.h"

extern u8* captureBuf;
extern ndspWaveBuf captureWaveBuf; 
extern const u32 CAPTURE_SIZE;

extern size_t linear_bytes_used;
void initCwavSystem(void);
void freeAllCwavs(void);


CWAV* getCwav(u32 index);
void playCwav(u32 index, bool stereo);
void stopCwav(u32 index);


bool loadCwavIndex(u32 index);
void unloadCwavIndex(u32 index);

#endif