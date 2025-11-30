       
#pragma once

#include <3ds.h>

typedef struct {
    float b0, b1, b2, a1, a2;
    float in1, in2;
    float out1, out2;
    float gain_db;
    float freq;
    float Q;
    bool is_shelf;
    bool is_high;
} EQBand;

void EQ_Init(void);
void EQ_UpdateBand(int band, float gain_db);
s16 EQ_ProcessSample(s16 sample);
