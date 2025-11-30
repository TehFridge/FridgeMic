#include "mic.h"
#include <3ds.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>                               
#include "logs.h"

                      
#define SAVE_DIR "sdmc:/FridgeMic"
#define FILE_PATH_FORMAT SAVE_DIR "/%lld.raw"
                          
static bool use_linear_alloc = false;
                                                         
bool Mic_Init(MicContext* mic) {
    if (!mic) {
        log_to_file("Mic_Init: mic pointer is NULL");
        return false;
    }

    memset(mic, 0, sizeof(MicContext));
    log_to_file("Mic_Init: Cleared MicContext struct (%zu bytes)", sizeof(MicContext));

                            
    mic->buffer_size = 0x30000;                
    mic->buffer = memalign(0x1000, mic->buffer_size);
    if (!mic->buffer) {
        log_to_file("Mic_Init: memalign failed for buffer (%u bytes)", mic->buffer_size);
        return false;
    }
    log_to_file("Mic_Init: buffer allocated at %p (%u bytes)", mic->buffer, mic->buffer_size);

                                                                        
    mic->raw_write_buffer = memalign(0x20, RAW_WRITE_BUFFER_SIZE); 
    if (!mic->raw_write_buffer) {
        log_to_file("Mic_Init: memalign failed for raw_write_buffer (%u bytes)", RAW_WRITE_BUFFER_SIZE);
        free(mic->buffer);
        return false;
    }
    log_to_file("Mic_Init: raw_write_buffer allocated at %p (%u bytes)", mic->raw_write_buffer, RAW_WRITE_BUFFER_SIZE);

                              
    Result res = micInit(mic->buffer, mic->buffer_size);
    if (R_FAILED(res)) {
        log_to_file("Mic_Init: micInit failed 0x%08X", (unsigned)res);
        free(mic->raw_write_buffer);
        free(mic->buffer);
        return false;
    }
    log_to_file("Mic_Init: micInit succeeded");

    res = MICU_SetPower(true);
    if (R_FAILED(res)) {
        log_to_file("Mic_Init: MICU_SetPower(true) failed 0x%08X", (unsigned)res);
        micExit();
        free(mic->raw_write_buffer);
        free(mic->buffer);
        return false;
    }
    log_to_file("Mic_Init: MICU_SetPower(true) succeeded");

    s32 hw_data_size = micGetSampleDataSize();
    if (hw_data_size <= 0 || hw_data_size > 0x30000) {
        hw_data_size = 0x30000;
        log_to_file("Mic_Init: micGetSampleDataSize invalid or too large, adjusted to %d", hw_data_size);
    } else {
        log_to_file("Mic_Init: micGetSampleDataSize returned %d", hw_data_size);
    }

    mic->data_size = (u32)hw_data_size;
    mic->read_pos = 0;
    mic->recording_raw = false;
    mic->raw_fd = -1;
    mic->raw_write_pos = 0;
    mic->sample_rate = MIC_RATE_16360;                                          
    
                                              
    mic->saving_in_progress = false;

    log_to_file("Mic_Init: Initialization complete successfully");
    return true;
}


void Mic_Exit(MicContext* mic) {
    if (!mic) return;

    if (mic->recording_raw) Mic_StopRawRecording(mic);
    micExit();

    if (mic->buffer) {
        if (use_linear_alloc) linearFree(mic->buffer);
        else free(mic->buffer);
        mic->buffer = NULL;
        log_to_file("Mic_Exit: MIC buffer freed");
    }

    if (mic->raw_write_buffer) {
        free(mic->raw_write_buffer);
        mic->raw_write_buffer = NULL;
        log_to_file("Mic_Exit: RAW write buffer freed");
    }

    MICU_SetPower(false);
    log_to_file("Mic_Exit: MICU powered off");

    memset(mic, 0, sizeof(MicContext));
}

bool Mic_StartSampling(MicContext* mic) {
    if (!mic) return false;

    MICU_SampleRate rate_enum;
                                                                            
    switch (mic->sample_rate) {
        case MIC_RATE_32730: rate_enum = MICU_SAMPLE_RATE_32730; log_to_file("MICU_SAMPLE_RATE_32730"); break;
        case MIC_RATE_16360: rate_enum = MICU_SAMPLE_RATE_16360; log_to_file("MICU_SAMPLE_RATE_16360"); break;
        case MIC_RATE_10910: rate_enum = MICU_SAMPLE_RATE_10910; log_to_file("MICU_SAMPLE_RATE_10910"); break;
        case MIC_RATE_8180:  rate_enum = MICU_SAMPLE_RATE_8180;  log_to_file("MICU_SAMPLE_RATE_8180"); break;
        default:
                                                                  
            mic->sample_rate = MIC_RATE_16360;
            rate_enum = MICU_SAMPLE_RATE_16360;
            log_to_file("MICU_SAMPLE_RATE_16360 (default)");
            break;
    }

    log_to_file("Mic_StartSampling: using buffer size %u", mic->data_size);

                                                                               
    Result res = MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED,
                                    rate_enum,                                           
                                    0,
                                    mic->data_size,
                                    true);
    if (R_FAILED(res)) {
        u32 summary     = R_SUMMARY(res);
        u32 description = R_DESCRIPTION(res);

        log_to_file("Mic_StartSampling failed: 0x%08X", (unsigned)res);
        log_to_file("  Summary: %u  Description: %u", summary, description);
    } else {
        mic->sampling_active = true;
        log_to_file("Mic_StartSampling succeeded");
    }

    return R_SUCCEEDED(res);
}

void Mic_StopSampling(MicContext* mic) {
    if (!mic) return;
    MICU_StopSampling();
    mic->sampling_active = false;
    log_to_file("Mic_StopSampling: stopped sampling");
}

void Mic_SetGain(MicContext* mic, u8 gain) {
    if (!mic) return;
    mic->gain = gain;
    MICU_SetGain(gain);
    log_to_file("Mic_SetGain: set gain to %u", gain);
}

bool Mic_SetSampleRate(MicContext* mic, MicSampleRate rate) {
    if (!mic) return false;

                                   
    if (rate > MIC_RATE_8180) rate = MIC_RATE_16360;                                               

    mic->sample_rate = rate;

    Result res = MICU_AdjustSampling((MICU_SampleRate)rate);
    if (R_FAILED(res)) {
        log_to_file("Mic_SetSampleRate failed: 0x%08X", (unsigned)res);
        return false;
    }

    s32 hw_data_size = micGetSampleDataSize();
    if (hw_data_size <= 0) {
        log_to_file("Mic_SetSampleRate: micGetSampleDataSize failed");
        return false;
    }
    mic->data_size = (u32)hw_data_size;

    log_to_file("Mic_SetSampleRate OK: enum %d (buffer=%u)", (int)rate, mic->data_size);
    return true;
}

                                                    
static void flushRawBuffer(MicContext* mic) {
    if (!mic || mic->raw_write_pos == 0 || mic->raw_fd < 0) return;

                                                                                     
                                                                      
    GSPGPU_FlushDataCache(mic->raw_write_buffer, mic->raw_write_pos);

    ssize_t wrote = write(mic->raw_fd, mic->raw_write_buffer, mic->raw_write_pos);
    if (wrote < 0) log_to_file("flushRawBuffer: write failed: %s", strerror(errno));
    
                                                                                                    
    mic->raw_write_pos = 0;
}
static void Mic_SaveThread(void* arg) {
    MicContext* mic = (MicContext*)arg;
    
    log_to_file("Mic_SaveThread: started file I/O for FD %d", mic->raw_fd);

                                             
    flushRawBuffer(mic); 

                                              
    if (mic->raw_fd >= 0) {
                                                                             
                                 
        fsync(mic->raw_fd); 
        close(mic->raw_fd);
    }

    mic->raw_fd = -1;
    
                                                                          
                                               
    log_to_file("Mic_SaveThread: finished saving and closing.");

                                         
                                                   
    mic->saving_in_progress = false;
}
bool Mic_StartRawRecording(MicContext* mic) {
    if (!mic) return false;

                                                        
                                              
    if (mkdir(SAVE_DIR, 0777) != 0 && errno != EEXIST) {
        log_to_file("Mic_StartRawRecording: Failed to create directory %s: %s", SAVE_DIR, strerror(errno));
        return false;
    }
    log_to_file("Mic_StartRawRecording: Directory %s ensured.", SAVE_DIR);
                          

        char file_name[1024];
                                                   
        snprintf(file_name,sizeof(file_name), FILE_PATH_FORMAT, time(NULL));
        
        mic->raw_fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (mic->raw_fd < 0) {
                log_to_file("Mic_StartRawRecording: open failed for file %s: %s", file_name, strerror(errno));
                return false;
        }
        mic->recording_raw = true;
        mic->raw_write_pos = 0;
        log_to_file("Mic_StartRawRecording: started recording, fd=%d", mic->raw_fd);
        return true;
}

void Mic_StopRawRecording(MicContext* mic) {
    if (!mic || !mic->recording_raw) return;

    log_to_file("Mic_StopRawRecording: Launching save thread...");

                                                                         
    mic->recording_raw = false; 

                                       
                                                                     
    mic->saving_in_progress = true;

                                                                                
                                                                                 
                                                             
    Thread save_thread = threadCreate(Mic_SaveThread, mic, 0x2000, 0x3F, -2, true);
    
    if (save_thread == NULL) {
                                     
        log_to_file("Mic_StopRawRecording: FAILED to create thread! Executing synchronously.");
        Mic_SaveThread(mic);                                                                    
    } else {
                                                                                            
                                       
        log_to_file("Mic_StopRawRecording: Save thread launched. UI should remain responsive.");
    }
}
void Mic_WriteToRawBuffer(MicContext* mic, s16* src, u32 num_samples) {
    if (!mic || !mic->recording_raw || !src || num_samples == 0) return;
    u32 copy_size = num_samples * sizeof(s16);
    if (mic->raw_write_pos + copy_size > RAW_WRITE_BUFFER_SIZE) flushRawBuffer(mic);
    if (copy_size > RAW_WRITE_BUFFER_SIZE) copy_size = RAW_WRITE_BUFFER_SIZE;
    memcpy(mic->raw_write_buffer + mic->raw_write_pos, (const u8*)src, copy_size);
    mic->raw_write_pos += copy_size;
                                                                                                                                                
}