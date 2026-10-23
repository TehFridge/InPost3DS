#include "cwav_shit.h"
#include "logs.h"
#include <stdlib.h> 
#include <stdio.h> 
#include <3ds.h>
#include <citro2d.h> 

#define FILE_COUNT (sizeof(fileList) / sizeof(fileList[0]))

const char* fileList[] = {
    "romfs:/sfx/collectedall.bcwav", 
    "romfs:/bgm/menumusic.bcwav",   
    "romfs:/bgm/intro.bcwav",       
    "romfs:/sfx/change.bcwav",      
    "romfs:/sfx/go.bcwav",          
    "romfs:/guy_va/guy_1.bcwav", 
    "romfs:/guy_va/guy_2.bcwav",
    "romfs:/guy_va/guy_3.bcwav",
    "romfs:/guy_va/guy_4.bcwav",
    "romfs:/guy_va/guy_5.bcwav",
    "romfs:/guy_va/guy_6.bcwav", 
    "romfs:/guy_va/guy_7.bcwav",
    "romfs:/guy_va/guy_8.bcwav",
    "romfs:/guy_va/guy_9.bcwav",
    "romfs:/guy_va/guy_10.bcwav", 
    "romfs:/guy_va/guy_11.bcwav",
    "romfs:/guy_va/guy_12.bcwav",
    "romfs:/guy_va/guy_13.bcwav",
    "romfs:/guy_va/guy_14.bcwav",
    "romfs:/guy_va/guy_15.bcwav",
    "romfs:/guy_va/guy_16.bcwav", 
    "romfs:/guy_va/guy_17.bcwav",
    "romfs:/guy_va/guy_18.bcwav",
    "romfs:/guy_va/guy_19.bcwav",
    "romfs:/guy_va/guy_20.bcwav", 
    "romfs:/guy_va/guy_21.bcwav",
    "romfs:/guy_va/guy_22.bcwav", 
    "romfs:/guy_va/guy_23.bcwav",
    "romfs:/guy_va/guy_24.bcwav",
    "romfs:/guy_va/guy_25.bcwav",
    "romfs:/guy_va/guy_26.bcwav", 
    "romfs:/female_va/female_1.bcwav", 
    "romfs:/female_va/female_2.bcwav",
    "romfs:/female_va/female_3.bcwav",
    "romfs:/female_va/female_4.bcwav",
    "romfs:/female_va/female_5.bcwav",
    "romfs:/female_va/female_6.bcwav", 
    "romfs:/female_va/female_7.bcwav",
    "romfs:/female_va/female_8.bcwav",
    "romfs:/female_va/female_9.bcwav",
    "romfs:/female_va/female_10.bcwav", 
    "romfs:/female_va/female_11.bcwav",
    "romfs:/female_va/female_12.bcwav",
    "romfs:/female_va/female_13.bcwav",
    "romfs:/female_va/female_14.bcwav",
    "romfs:/female_va/female_15.bcwav",
    "romfs:/female_va/female_16.bcwav", 
    "romfs:/female_va/female_17.bcwav",
    "romfs:/female_va/female_18.bcwav",
    "romfs:/female_va/female_19.bcwav",
    "romfs:/female_va/female_20.bcwav", 
    "romfs:/female_va/female_21.bcwav",
    "romfs:/female_va/female_22.bcwav", 
    "romfs:/female_va/female_23.bcwav",
    "romfs:/female_va/female_24.bcwav",
    "romfs:/female_va/female_25.bcwav",
    "romfs:/female_va/female_26.bcwav", 
    "romfs:/arhn_va/arhn_1.bcwav", 
    "romfs:/arhn_va/arhn_2.bcwav",
    "romfs:/arhn_va/arhn_3.bcwav",
    "romfs:/arhn_va/arhn_4.bcwav",
    "romfs:/arhn_va/arhn_5.bcwav",
    "romfs:/arhn_va/arhn_6.bcwav",
    "romfs:/arhn_va/arhn_7.bcwav",
    "romfs:/arhn_va/arhn_8.bcwav",
    "romfs:/arhn_va/arhn_9.bcwav",
    "romfs:/arhn_va/arhn_10.bcwav",
    "romfs:/arhn_va/arhn_11.bcwav",
    "romfs:/arhn_va/arhn_12.bcwav",
    "romfs:/arhn_va/arhn_13.bcwav",
    "romfs:/arhn_va/arhn_14.bcwav",
    "romfs:/arhn_va/arhn_15.bcwav",
    "romfs:/arhn_va/arhn_16.bcwav",
    "romfs:/arhn_va/arhn_17.bcwav",
    "romfs:/arhn_va/arhn_18.bcwav",
    "romfs:/arhn_va/arhn_19.bcwav",
    "romfs:/arhn_va/arhn_20.bcwav",
    "romfs:/arhn_va/arhn_21.bcwav",
    "romfs:/arhn_va/arhn_22.bcwav", 
    "romfs:/arhn_va/arhn_23.bcwav",
    "romfs:/arhn_va/arhn_24.bcwav",
    "romfs:/arhn_va/arhn_25.bcwav",
    "romfs:/arhn_va/arhn_26.bcwav",
    "romfs:/bgm/credits.bcwav",
    "romfs:/bgm/nadaj.bcwav",
    "romfs:/guy_va/guy_27.bcwav",
    "romfs:/guy_va/guy_28.bcwav",
    "romfs:/guy_va/guy_29.bcwav",
    "romfs:/guy_va/guy_30.bcwav", 
    "romfs:/guy_va/guy_31.bcwav",
    "romfs:/guy_va/guy_32.bcwav", 
    "romfs:/guy_va/guy_33.bcwav",
    "romfs:/female_va/female_27.bcwav",
    "romfs:/female_va/female_28.bcwav",
    "romfs:/female_va/female_29.bcwav",
    "romfs:/female_va/female_30.bcwav", 
    "romfs:/female_va/female_31.bcwav",
    "romfs:/female_va/female_32.bcwav", 
    "romfs:/female_va/female_33.bcwav",
    "romfs:/arhn_va/arhn_27.bcwav",
    "romfs:/arhn_va/arhn_28.bcwav",
    "romfs:/arhn_va/arhn_29.bcwav",
    "romfs:/arhn_va/arhn_30.bcwav", 
    "romfs:/arhn_va/arhn_31.bcwav",
    "romfs:/arhn_va/arhn_32.bcwav", 
    "romfs:/arhn_va/arhn_33.bcwav",
};

const u8 maxSPlayList[] = {
    2, 2, 2, 2, 2, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

typedef struct {
    const char* filename;
    CWAV* cwav;
    void* buffer;
    bool loaded;
    u32 size; 
} CWAVEntry;

static CWAVEntry cwavList[FILE_COUNT];

const char* cwavGetStatusString(cwavStatus_t status) {
    switch (status) {
        case CWAV_NOT_ALLOCATED:           return "CWAV_NOT_ALLOCATED";
        case CWAV_SUCCESS:                 return "CWAV_SUCCESS";
        case CWAV_INVALID_ARGUMENT:        return "CWAV_INVALID_ARGUMENT";
        case CWAV_FILE_OPEN_FAILED:        return "CWAV_FILE_OPEN_FAILED";
        case CWAV_FILE_READ_FAILED:        return "CWAV_FILE_READ_FAILED";
        case CWAV_UNKNOWN_FILE_FORMAT:     return "CWAV_UNKNOWN_FILE_FORMAT";
        case CWAV_INVAID_INFO_BLOCK:       return "CWAV_INVAID_INFO_BLOCK";
        case CWAV_INVAID_DATA_BLOCK:       return "CWAV_INVAID_DATA_BLOCK";
        case CWAV_UNSUPPORTED_AUDIO_ENCODING: return "CWAV_UNSUPPORTED_AUDIO_ENCODING";
        case CWAV_INVALID_CWAV_CHANNEL:    return "CWAV_INVALID_CWAV_CHANNEL";
        case CWAV_NO_CHANNEL_AVAILABLE:    return "CWAV_NO_CHANNEL_AVAILABLE";
        default:                           return "UNKNOWN_STATUS";
    }
}

bool loadCwavIndex(u32 index)
{
    if (index >= FILE_COUNT)
        return false;

    CWAVEntry* entry = &cwavList[index];

    if (entry->loaded && entry->cwav)
        return true;

    CWAV* cwav = calloc(1, sizeof(*cwav));
    if (!cwav)
        return false;

    log_to_file(
        "[CWAV] Alloc index=%u ptr=%p size=%zu\n",
        index,
        (void*)cwav,
        sizeof(*cwav)
    );

    log_to_file(
        "[CWAV] Loading index=%u '%s'\n",
        index,
        entry->filename
    );

    cwavFileLoad(cwav, entry->filename, maxSPlayList[index]);

    log_to_file(
        "[CWAV] Load result index=%u status=%s\n",
        index,
        cwavGetStatusString(cwav->loadStatus)
    );

    if (cwav->loadStatus != CWAV_SUCCESS)
    {
        cwavFileFree(cwav);
        free(cwav);
        return false;
    }

    entry->cwav = cwav;
    entry->loaded = true;

    return true;
}

void unloadCwavIndex(u32 index)
{
    if (index >= FILE_COUNT)
        return;

    CWAVEntry* entry = &cwavList[index];

    if (!entry->cwav)
        return;

    log_to_file(
        "[CWAV] Unloading index=%u '%s' ptr=%p\n",
        index,
        entry->filename,
        (void*)entry->cwav
    );

    cwavStop(entry->cwav, -1, -1);

    cwavFileFree(entry->cwav);
    free(entry->cwav);

    entry->cwav = NULL;
    entry->loaded = false;

    log_to_file(
        "[CWAV] Unloaded index=%u\n",
        index
    );
}

void initCwavSystem(void) {
    for (u32 i = 0; i < FILE_COUNT; i++) {
        cwavList[i].filename = fileList[i];
        cwavList[i].cwav = NULL;
        cwavList[i].buffer = NULL;
        cwavList[i].loaded = false;
        cwavList[i].size = 0; 
    }
    log_to_file("[INFO] CWAV system initialized (%d entries)\n", FILE_COUNT);
}

void freeAllCwavs(void) {
    for (u32 i = 0; i < FILE_COUNT; i++) {
        unloadCwavIndex(i);
    }
    log_to_file("[INFO] All CWAVs unloaded.\n");
}

CWAV* getCwav(u32 index) {
    if (index >= FILE_COUNT) return NULL;
    if (!cwavList[index].loaded) loadCwavIndex(index);
    return cwavList[index].cwav;
}

void playCwav(u32 index, bool stereo) {
    CWAV* cwav = getCwav(index);
    
    log_to_file("[CWAV] Request Play Index: %d | PTR: %p\n", index, cwav);

    if (cwav) {
        
        if (cwav->loadStatus != CWAV_SUCCESS) {
             log_to_file("[ERR] Attempted to play invalid CWAV (Status: %s)\n", cwavGetStatusString(cwav->loadStatus));
             return;
        }

        cwavPlayResult result;

        if (stereo)
            result = cwavPlay(cwav, 0, 1);
        else
            result = cwavPlay(cwav, 0, -1);
        
        log_to_file("[CWAV] Play '%s' Result: %s (L:%d R:%d)\n", 
                    fileList[index], 
                    cwavGetStatusString(result.playStatus),
                    result.monoLeftChannel,
                    result.rightChannel);
        
    } else {
        log_to_file("[CWAV] Play failed: Index %d returned NULL pointer.\n", index);
    }
}

void stopCwav(u32 index) {
    if (index >= FILE_COUNT) return;
    CWAVEntry* entry = &cwavList[index];
    
    if (!entry->loaded || !entry->cwav) {
        log_to_file("[CWAV] Stop ignored: Index %d not loaded.\n", index);
        return;
    }

    cwavStop(entry->cwav, -1, -1);
    log_to_file("[CWAV] Stopped '%s' | PTR: %p\n", entry->filename, entry->cwav);
}