#include <3ds.h>
#include <citro2d.h>
#include "global_draws.h"
#include "cwav_shit.h"
#include "main.h"
#include "sprites.h"
#include <math.h>
#include <stdlib.h>

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240
#define TILE_SIZE 40

float offsetX = 0.0f;
float offsetY = 0.0f;
float speedX = 0.5f;    
float speedY = 0.5f; 
C3D_RenderTarget* top;
C3D_RenderTarget* bottom;

float easeInQuad(float t, float start, float end, float duration) {
    t /= duration;
    return start + (end - start) * (t * t);
}

float easeOutQuad(float t, float start, float end, float duration) {
    t /= duration;
    return start + (end - start) * (1 - (1 - t) * (1 - t));
}

float easeHop(float t, float start, float end, float duration) {
    float s = 1.70158f * 1.5f;                    
    t /= duration;
    t -= 1.0f;
    return (end - start) * (t * t * ((s + 1) * t + s) + 1.0f) + start;
}

void Update_Kwadraty(){
    offsetX += speedX;
    offsetY += speedY;

    if (offsetX >= TILE_SIZE) offsetX = 0.0f;
    if (offsetY >= TILE_SIZE) offsetY = 0.0f;
}

void Draw_Kwadraty(u32 kolor1, u32 kolor2){
    int tilesX = SCREEN_WIDTH / TILE_SIZE + 2;
    int tilesY = SCREEN_HEIGHT / TILE_SIZE + 2;

    for (int y = 0; y < tilesY; y++) {
        for (int x = 0; x < tilesX; x++) {
            float posX = x * TILE_SIZE - offsetX;
            float posY = y * TILE_SIZE - offsetY;

            if ((x + y) % 2 == 0) {
                C2D_DrawRectSolid(posX, posY, 0, TILE_SIZE, TILE_SIZE, kolor1);
            } else {
                C2D_DrawRectSolid(posX, posY, 0, TILE_SIZE, TILE_SIZE, kolor2);
            }
        }
    }
}
