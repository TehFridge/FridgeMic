#include "cwav_shit.h"
#include "logs.h"
#define FILE_COUNT (sizeof(fileList) / sizeof(fileList[0]))

                
const char* fileList[] = {
    "romfs:/splash.bcwav"
};

static const u8 maxSPlayList[] = {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};

                  
typedef struct {
    const char* filename;
    CWAV* cwav;
    void* buffer;
    bool loaded;
} CWAVEntry;

static CWAVEntry cwavList[FILE_COUNT];

                                                                
                                                                 
                                                                

bool loadCwavIndex(u32 index) {
    if (index >= FILE_COUNT)
        return false;

    CWAVEntry* entry = &cwavList[index];
    if (entry->loaded)
        return true;                  

    FILE* file = fopen(entry->filename, "rb");
    if (!file) {
        log_to_file("[WARN] Failed to open %s\n", entry->filename);
        return false;
    }

    fseek(file, 0, SEEK_END);
    u32 fileSize = ftell(file);
    rewind(file);

    void* buffer = linearAlloc(fileSize);
    if (!buffer) {
        fclose(file);
        svcBreak(USERBREAK_PANIC);
        return false;
    }

    fread(buffer, 1, fileSize, file);
    fclose(file);

    CWAV* cwav = (CWAV*)malloc(sizeof(CWAV));
    if (!cwav) {
        linearFree(buffer);
        svcBreak(USERBREAK_PANIC);
        return false;
    }

    cwavLoad(cwav, buffer, maxSPlayList[index]);

    cwav->dataBuffer = buffer;

    if (cwav->loadStatus != CWAV_SUCCESS) {
        cwavFree(cwav);
        linearFree(buffer);
        free(cwav);
        log_to_file("[WARN] Failed to load CWAV: %s\n", entry->filename);
        return false;
    }

    entry->cwav = cwav;
    entry->buffer = buffer;
    entry->loaded = true;

    log_to_file("[INFO] Loaded CWAV: %s\n", entry->filename);
    return true;
}

void unloadCwavIndex(u32 index) {
    if (index >= FILE_COUNT)
        return;

    CWAVEntry* entry = &cwavList[index];
    if (!entry->loaded)
        return;

    cwavFree(entry->cwav);
    linearFree(entry->buffer);
    free(entry->cwav);

    entry->cwav = NULL;
    entry->buffer = NULL;
    entry->loaded = false;

    log_to_file("[INFO] Unloaded CWAV: %s\n", entry->filename);
}

                                                                
                                                                 
                                                                

void initCwavSystem(void) {
    for (u32 i = 0; i < FILE_COUNT; i++) {
        cwavList[i].filename = fileList[i];
        cwavList[i].cwav = NULL;
        cwavList[i].buffer = NULL;
        cwavList[i].loaded = false;
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
    if (index >= FILE_COUNT)
        return NULL;
    if (!cwavList[index].loaded)
        loadCwavIndex(index);
    return cwavList[index].cwav;
}

                                             
void playCwav(u32 index, bool stereo) {
    CWAV* cwav = getCwav(index);
    if (cwav)
        if (stereo)
            cwavPlay(cwav, 0, 1);                             
        else
            cwavPlay(cwav, 0, -1);
}

void stopCwav(u32 index) {
    if (index >= FILE_COUNT)
        return;

    CWAVEntry* entry = &cwavList[index];
    if (!entry->loaded || !entry->cwav)
        return;

    cwavStop(entry->cwav, -1, -1);                      
    log_to_file("[INFO] Stopped CWAV: %s\n", entry->filename);
}