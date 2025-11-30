#ifndef GLOBAL_DRAWS_H
#define GLOBAL_DRAWS_H

extern C3D_RenderTarget* top;
extern C3D_RenderTarget* bottom;

float easeInQuad(float t, float start, float end, float duration);

float easeOutQuad(float t, float start, float end, float duration);

float easeHop(float t, float start, float end, float duration);

void Update_Kwadraty();

void Draw_Kwadraty(u32 kolor1, u32 kolor2);

void drawCaptureWaveform();
#endif