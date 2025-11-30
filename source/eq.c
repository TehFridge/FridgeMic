       
#include "eq.h"
#include <math.h>

#define NUM_BANDS 5
static EQBand eqBands[NUM_BANDS];

static inline float db2lin(float db) { return powf(10.0f, db/20.0f); }

void EQ_CalcCoeffs(EQBand* band) {
    float A = db2lin(band->gain_db);
    float w0 = 2.0f * M_PI * (band->freq / 32730.0f);                       
    float cosw0 = cosf(w0);
    float sinw0 = sinf(w0);
    float alpha = sinw0 / (2.0f * band->Q);
    float b0, b1, b2, a0, a1, a2;

    if (band->is_shelf) {
        float sqrtA = sqrtf(A);
        if (band->is_high) {
            b0 =    A*( (A+1)+(A-1)*cosw0 +2*sqrtA*alpha );
            b1 = -2*A*( (A-1)+(A+1)*cosw0 );
            b2 =    A*( (A+1)+(A-1)*cosw0 -2*sqrtA*alpha );
            a0 =        (A+1)-(A-1)*cosw0 +2*sqrtA*alpha;
            a1 =  2*( (A-1)-(A+1)*cosw0 );
            a2 =        (A+1)-(A-1)*cosw0 -2*sqrtA*alpha;
        } else {
            b0 =    A*( (A+1)-(A-1)*cosw0 +2*sqrtA*alpha );
            b1 =  2*A*( (A-1)-(A+1)*cosw0 );
            b2 =    A*( (A+1)-(A-1)*cosw0 -2*sqrtA*alpha );
            a0 =        (A+1)+(A-1)*cosw0 +2*sqrtA*alpha;
            a1 = -2*( (A-1)+(A+1)*cosw0 );
            a2 =        (A+1)+(A-1)*cosw0 -2*sqrtA*alpha;
        }
    } else {                  
        b0 = 1 + alpha*A;
        b1 = -2*cosw0;
        b2 = 1 - alpha*A;
        a0 = 1 + alpha/A;
        a1 = -2*cosw0;
        a2 = 1 - alpha/A;
    }

    band->b0 = b0/a0;
    band->b1 = b1/a0;
    band->b2 = b2/a0;
    band->a1 = a1/a0;
    band->a2 = a2/a0;
}

void EQ_Init(void) {
                  
    float freqs[NUM_BANDS] = { 100.0f, 300.0f, 1000.0f, 3000.0f, 8000.0f };
    for (int i = 0; i < NUM_BANDS; i++) {
        eqBands[i].freq = freqs[i];
        eqBands[i].Q = 0.707f;            
        eqBands[i].gain_db = 0.0f;
        eqBands[i].is_shelf = (i==0 || i==4);                              
        eqBands[i].is_high = (i==4);
        eqBands[i].in1 = eqBands[i].in2 = 0.0f;
        eqBands[i].out1 = eqBands[i].out2 = 0.0f;
        EQ_CalcCoeffs(&eqBands[i]);
    }
}

void EQ_UpdateBand(int band, float gain_db) {
    if (band >= 0 && band < NUM_BANDS) {
        eqBands[band].gain_db = gain_db;
        EQ_CalcCoeffs(&eqBands[band]);
    }
}

s16 EQ_ProcessSample(s16 sample) {
    float in = (float)sample;
    float out = in;
    for (int i = 0; i < NUM_BANDS; i++) {
        EQBand* b = &eqBands[i];
        float y = b->b0*out + b->b1*b->in1 + b->b2*b->in2 - b->a1*b->out1 - b->a2*b->out2;
        b->in2 = b->in1;
        b->in1 = out;
        b->out2 = b->out1;
        b->out1 = y;
        out = y;
    }
    if (out > 32767.0f) out = 32767.0f;
    if (out < -32768.0f) out = -32768.0f;
    return (s16)out;
}
