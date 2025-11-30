#ifndef MIC_H
#define MIC_H

#include <3ds.h>
#include <stdbool.h>

#define RAW_WRITE_BUFFER_SIZE 0x40000          

typedef enum {
    MIC_RATE_32730 = 0,
    MIC_RATE_16360,
    MIC_RATE_10910,
    MIC_RATE_8180
} MicSampleRate;

typedef struct {
    u8* buffer;
    u32 buffer_size;
    u32 data_size;
    u32 read_pos;

    bool sampling_active;
    u32 sample_rate;
    u8 gain;

    bool recording_raw;
    int raw_fd;
    u8* raw_write_buffer;
    u32 raw_write_pos;

                  
                                                                       
                                                                         
                                                                               
    volatile bool saving_in_progress;

} MicContext;

bool Mic_Init(MicContext* mic);                      
void Mic_Exit(MicContext* mic);
bool Mic_StartSampling(MicContext* mic);
void Mic_StopSampling(MicContext* mic);
void Mic_SetGain(MicContext* mic, u8 gain);
bool Mic_SetSampleRate(MicContext* mic, MicSampleRate rate);

bool Mic_StartRawRecording(MicContext* mic);
void Mic_StopRawRecording(MicContext* mic);
void Mic_WriteToRawBuffer(MicContext* mic, s16* src, u32 num_samples);

#endif