#include "text.h"
C2D_TextBuf g_staticBuf;
C2D_TextBuf kupon_text_Buf;
C2D_Text g_staticText[100];
                        
                           
C2D_Text g_kuponText[100];
C2D_Font font[1];
void initText(){
    g_staticBuf = C2D_TextBufNew(256);
	kupon_text_Buf = C2D_TextBufNew(512);
	                                  
    font[0] = C2D_FontLoad("romfs:/bold.bcfnt");
    C2D_TextFontParse(&g_staticText[0], font[0], g_staticBuf, "Wciśnij A.");
    C2D_TextOptimize(&g_staticText[0]);
	C2D_TextFontParse(&g_staticText[1], font[0], g_staticBuf, "Ładowanie...");
	C2D_TextOptimize(&g_staticText[1]);
	C2D_TextFontParse(&g_staticText[2], font[0], g_staticBuf, "Nie wykryto danych konta Żappka.\nWciśnij A by kontynuować");
	C2D_TextOptimize(&g_staticText[2]);
	C2D_TextFontParse(&g_staticText[3], font[0], g_staticBuf, "Tak");
	C2D_TextFontParse(&g_staticText[4], font[0], g_staticBuf, "Nie");
	C2D_TextFontParse(&g_staticText[5], font[0], g_staticBuf, "Wprowadź numer telefonu.");
	C2D_TextOptimize(&g_staticText[5]);
	C2D_TextFontParse(&g_staticText[6], font[0], g_staticBuf, "Wprowadź kod SMS.");
	C2D_TextOptimize(&g_staticText[6]);
	                                                                           
    C2D_TextFontParse(&g_staticText[8], font[0], g_staticBuf, "Twoje Żappsy");
    C2D_TextOptimize(&g_staticText[8]);
    C2D_TextFontParse(&g_staticText[10], font[0], g_staticBuf, "B - Powrót");
    C2D_TextOptimize(&g_staticText[10]);
    C2D_TextFontParse(&g_staticText[11], font[0], g_staticBuf, "Brak internetu :(");
    C2D_TextOptimize(&g_staticText[11]);
	C2D_TextFontParse(&g_staticText[12], font[0], g_staticBuf, "Kupony");
    C2D_TextOptimize(&g_staticText[12]);
	C2D_TextFontParse(&g_staticText[13], font[0], g_staticBuf, "Gotowe :)");
    C2D_TextOptimize(&g_staticText[13]);
	C2D_TextFontParse(&g_staticText[14], font[0], g_staticBuf, "Opcje");
    C2D_TextOptimize(&g_staticText[14]);
	C2D_TextFontParse(&g_staticText[15], font[0], g_staticBuf, "Motyw:");
    C2D_TextOptimize(&g_staticText[15]);
	C2D_TextFontParse(&g_staticText[16], font[0], g_staticBuf, "(Zrestartuj aplikacje by zapisać zmiany)");
    C2D_TextOptimize(&g_staticText[16]);
	C2D_TextFontParse(&g_staticText[17], font[0], g_staticBuf, "VA");
    C2D_TextOptimize(&g_staticText[17]);
}
void drawShadowedText(C2D_Text* text, float x, float y, float depth, float scaleX, float scaleY, u32 color, u32 shadowColor, bool shadow_border) {
    static const float shadowOffsets[4][2] = {
        {0.0f, 1.8f},
        {0.0f, -0.7f},
        {-1.7f, 0.0f},
        {1.8f, 0.0f}
    };
	if (shadow_border) {
		for (int i = 0; i < 4; i++) {
			C2D_DrawText(text, C2D_WithColor,
						x + shadowOffsets[i][0], y + shadowOffsets[i][1],
						depth, scaleX, scaleY, shadowColor);
		}
	} else {
		C2D_DrawText(text, C2D_WithColor, x, y + 1.35, depth, scaleX, scaleY, color);
	}
	if (shadow_border) {
    	C2D_DrawText(text, C2D_WithColor, x, y, depth, scaleX, scaleY, color);
	} else {
		C2D_DrawText(text, C2D_WithColor, x, y, depth, scaleX, scaleY, shadowColor);
	}
}


void drawShadowedTextWrapped(C2D_Text* text, float x, float y, float depth, float scaleX, float scaleY, u32 color, u32 shadowColor) {
    static const float shadowOffsets[4][2] = {
        {0.0f, 1.8f},
        {0.0f, -0.7f},
        {-1.7f, 0.0f},
        {1.8f, 0.0f}
    };

    for (int i = 0; i < 4; i++) {
        C2D_DrawText(text, C2D_AlignCenter | C2D_WithColor,
                     x + shadowOffsets[i][0], y + shadowOffsets[i][1],
                     depth, scaleX, scaleY, shadowColor, 300.0f);
    }

    C2D_DrawText(text, C2D_AlignCenter | C2D_WithColor, x, y, depth, scaleX, scaleY, color, 300.0f);
}
void drawShadowedText_noncentered(C2D_Text* text, float x, float y, float depth, float scaleX, float scaleY, u32 color, u32 shadowColor) {
    static const float shadowOffsets[4][2] = {
        {0.0f, 1.8f},
        {0.0f, -0.7f},
        {-1.7f, 0.0f},
        {1.8f, 0.0f}
    };

    for (int i = 0; i < 4; i++) {
        C2D_DrawText(text, C2D_WithColor,
                     x + shadowOffsets[i][0], y + shadowOffsets[i][1],
                     depth, scaleX, scaleY, shadowColor);
    }

    C2D_DrawText(text, C2D_WithColor, x, y, depth, scaleX, scaleY, color);
}