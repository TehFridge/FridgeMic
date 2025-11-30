         
#ifndef TEXT_H
#define TEXT_H
#include <citro2d.h>
#include <stdlib.h>
extern C2D_TextBuf g_staticBuf;
extern C2D_TextBuf kupon_text_Buf;
extern C2D_Text g_staticText[100];
                        
                           
extern C2D_Text g_kuponText[100];
extern C2D_Font font[1];
void initText();
void drawShadowedText(C2D_Text* text, float x, float y, float depth, float scaleX, float scaleY, u32 color, u32 shadowColor, bool shadow_border);
void drawShadowedTextWrapped(C2D_Text* text, float x, float y, float depth, float scaleX, float scaleY, u32 color, u32 shadowColor);
void drawShadowedText_noncentered(C2D_Text* text, float x, float y, float depth, float scaleX, float scaleY, u32 color, u32 shadowColor);
#endif