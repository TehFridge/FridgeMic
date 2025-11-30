            
#ifndef SPRITES_H
#define SPRITES_H
#include <citro2d.h>
#include <stdlib.h>
typedef struct {
    int currentFrame;
    u64 lastFrameTime;
    int loopedtimes;
    int loops;
    bool done;
    
    int halt_at_frame;
    int halt_for_howlong;         
    u64 haltStartTime;                             
    bool halting;                                           
} SpriteAnimState;


extern C2D_SpriteSheet logo, splash;
extern C2D_Image bgtop, bgdown, splash1, splash2, logo3ds;
void ResetAnimState(SpriteAnimState* anim);
void PlaySprite(float scale, C2D_SpriteSheet frames, int framerate, int framecount, float x, float y, SpriteAnimState* anim, int direction, float depth);
void spritesInit();
#endif