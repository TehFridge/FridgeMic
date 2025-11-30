#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "scene_mainmenu.h"
#include "scene_manager.h"
#include "main.h"
#include "logs.h"
#include "sprites.h"
#include "global_draws.h"

#include "eq.h"
#include "mic.h"
#include "text.h"

                  
#define SERVER_PORT 5000
#define VOLUME_THRESHOLD 50
#define SCOPE_SAMPLES 512
#define POINT_RADIUS 14

            
#define EQ_POINTS 5
#define EQ_MIN_GAIN -12.0f
#define EQ_MAX_GAIN 12.0f
#define EQ_X0 20
#define EQ_Y0 20
#define EQ_W 280
#define EQ_H 140

                       
static MicContext mic;
static bool initialized = false;
static int sockfd = -1;
static struct sockaddr_in server_addr;
static char SERVER_IP[256];

static u32 volume_threshold = VOLUME_THRESHOLD;
static bool muted = false;
                                                                     
static u32 last_mic_volume = 0;
static bool eq_on = false;
static bool eqmode = false;
static int current_band = 0;
static float eq_gains[EQ_POINTS] = {0,0,0,0,0};

                      
static s16 scopeBuf[SCOPE_SAMPLES];
static int scopeWrite = 0;

                  
typedef struct { float gain_db; } EQPoint;
static EQPoint eq_points[EQ_POINTS] = { {0},{0},{0},{0},{0} };
static int grabbed_band = -1;
static const float band_x_norm[EQ_POINTS] = { 0.05f, 0.25f, 0.50f, 0.73f, 0.95f };

                           
static void makeSocketNonBlocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags >= 0) fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

                                                    
static int getSampleRateHz(MicSampleRate rate) {
    switch(rate) {
        case MIC_RATE_32730: return 32730;
        case MIC_RATE_16360: return 16360;
        case MIC_RATE_10910: return 10910;
        case MIC_RATE_8180:  return 8180;
        default:             return 16360;                    
    }
}

                                                              
static void changeSampleRate(bool up) {
    MicSampleRate current_rate = (MicSampleRate)mic.sample_rate;
    MicSampleRate new_rate = current_rate;

    if (up) {                                            
        switch(current_rate) {
            case MIC_RATE_8180:  new_rate = MIC_RATE_10910; break;
            case MIC_RATE_10910: new_rate = MIC_RATE_16360; break;
            case MIC_RATE_16360: new_rate = MIC_RATE_32730; break;
            case MIC_RATE_32730: new_rate = MIC_RATE_8180;  break;        
        }
    } else {                                              
        switch(current_rate) {
            case MIC_RATE_8180:  new_rate = MIC_RATE_32730; break;        
            case MIC_RATE_10910: new_rate = MIC_RATE_8180;  break;
            case MIC_RATE_16360: new_rate = MIC_RATE_10910; break;
            case MIC_RATE_32730: new_rate = MIC_RATE_16360; break;
        }
    }

    if (new_rate != current_rate) {
        if (Mic_SetSampleRate(&mic, new_rate)) {
            log_to_file("Sample rate changed to enum %d", new_rate);

                                             
            if (sockfd >= 0) {
                char srate_cmd[32];
                int new_rate_hz = getSampleRateHz(new_rate);
                snprintf(srate_cmd, sizeof(srate_cmd), "SRATE:%d", new_rate_hz);
                
                                          
                sendto(sockfd, srate_cmd, strlen(srate_cmd), 0,
                       (struct sockaddr*)&server_addr, sizeof(server_addr));
                       
                log_to_file("Sent SRATE command: %s", srate_cmd);
            }
                              
            
        } else {
            log_to_file("Failed to change sample rate");
        }
    }
}


                         
void drawMicWaveform(void) {
    const float centerY = 120.0f;
    const float scaleY  = 80.0f;
    const int width     = 400;
    const int step      = SCOPE_SAMPLES / width;
    if (step <= 0) return;

    int idx = scopeWrite;
    float prevX = 0.0f;
    float prevY = centerY;

    const u32 waveColor   = C2D_Color32(0, 255, 0, 255);
    const u32 shadowColor = C2D_Color32(0, 0, 0, 120);

    for (int x = 0; x < width; x++) {
        if (idx >= SCOPE_SAMPLES) idx -= SCOPE_SAMPLES;
        float y = centerY + ((float)scopeBuf[idx] * (scaleY / 32768.0f));
        float fx = (float)x;

        if (x > 0) {
            C2D_DrawLine(prevX, prevY + 3.0f, shadowColor,
                         fx,    y     + 3.0f, shadowColor, 2.5f, 0.2f);
            C2D_DrawLine(prevX, prevY, waveColor,
                         fx,    y,     waveColor, 2.0f, 0.3f);
        }

        prevX = fx;
        prevY = y;
        idx += step;
    }
}

void drawEQEditor(void) {
    const u32 grid  = C2D_Color32(70,70,70,255);
    const u32 curve = C2D_Color32(0,220,255,255);
    const u32 node  = C2D_Color32(255,220,0,255);

           
    for (int i = 0; i <= 4; i++) {
        float y = EQ_Y0 + (EQ_H / 4.0f) * i;
        C2D_DrawLine(EQ_X0, y, grid, EQ_X0 + EQ_W, y, grid, 1, 0);
    }

            
    for (int i = 0; i < EQ_POINTS-1; i++) {
        float x1 = EQ_X0 + band_x_norm[i] * EQ_W;
        float x2 = EQ_X0 + band_x_norm[i+1] * EQ_W;
        float y1 = EQ_Y0 + (1 - (eq_points[i].gain_db - EQ_MIN_GAIN)/(EQ_MAX_GAIN-EQ_MIN_GAIN)) * EQ_H;
        float y2 = EQ_Y0 + (1 - (eq_points[i+1].gain_db - EQ_MIN_GAIN)/(EQ_MAX_GAIN-EQ_MIN_GAIN)) * EQ_H;
        C2D_DrawLine(x1, y1, curve, x2, y2, curve, 3.0f, 0.3f);
    }

             
    for (int i = 0; i < EQ_POINTS; i++) {
        float x = EQ_X0 + band_x_norm[i] * EQ_W;
        float y = EQ_Y0 + (1 - (eq_points[i].gain_db - EQ_MIN_GAIN)/(EQ_MAX_GAIN-EQ_MIN_GAIN)) * EQ_H;
        C2D_DrawCircleSolid(x, y, 0, 6, node);
    }
}
                                    
                    
static SwkbdState swkbd;
static char server_ip_input[256];

static float fade_alpha = 1.0f;      // 1 = fully black, 0 = visible
static bool fading_in = true;
            
void sceneMainmenuInit(void) {
    initialized = false;
    sockfd = -1;
    volume_threshold = VOLUME_THRESHOLD;
    muted = false;
                                                 
    last_mic_volume = 0;
    eq_on = false;
    eqmode = false;
    current_band = 0;
    scopeWrite = 0;
    grabbed_band = -1;

    for (int i = 0; i < EQ_POINTS; i++)
        eq_points[i].gain_db = eq_gains[i];

                       
    if (!Mic_Init(&mic)) {
        log_to_file("Mic_Init failed");
        return;
    }
    log_to_file("Mic_Init success");

                                        
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, sizeof(server_ip_input));
    swkbdSetInitialText(&swkbd, server_ip_input);
    swkbdSetHintText(&swkbd, "Enter Server IP");

    SwkbdButton button = swkbdInputText(&swkbd, server_ip_input, sizeof(server_ip_input));
    if (button == SWKBD_BUTTON_CONFIRM) {
        snprintf(SERVER_IP, sizeof(SERVER_IP), "%s", server_ip_input);
        log_to_file("SWKBD input done: %s", SERVER_IP);

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    } else {
        log_to_file("SWKBD cancelled");
        return;
    }

                     
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        Mic_Exit(&mic);
        log_to_file("Socket creation failed");
        return;
    }
    makeSocketNonBlocking(sockfd);
    log_to_file("Socket initialized");

                      
    EQ_Init();
    for (int i = 0; i < EQ_POINTS; i++)
        EQ_UpdateBand(i, eq_gains[i]);
    log_to_file("EQ initialized");

                               
    mic.gain = 0x30;
    Mic_SetGain(&mic, mic.gain);
    log_to_file("Mic gain set to %u", mic.gain);

                                      
    mic.sample_rate = MIC_RATE_32730;               
                                                    
                                                 

    if (!Mic_StartSampling(&mic)) {
        close(sockfd);
        Mic_Exit(&mic);
        log_to_file("Mic_StartSampling failed");
        return;
    }
    log_to_file("Mic_StartSampling succeeded");
    
                                                                 
                                                            
    char srate_cmd[32];
    int new_rate_hz = getSampleRateHz(mic.sample_rate);
    snprintf(srate_cmd, sizeof(srate_cmd), "SRATE:%d", new_rate_hz);
    sendto(sockfd, srate_cmd, strlen(srate_cmd), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    log_to_file("Sent initial SRATE command: %s", srate_cmd);
                      

    initialized = true;
    fade_alpha = 1.0f;
    fading_in = true;

}

void sceneMainmenuUpdate(uint32_t kDown, uint32_t kHeld) {
    if (!initialized) return;

                                                       

    Update_Kwadraty();
                                                   

    touchPosition t;
    hidTouchRead(&t);
                                                                 
    if (kDown & KEY_SELECT) eqmode = !eqmode;
    if (kDown & KEY_B) muted = !muted;
                        
    if (eqmode) {
        if (t.px || t.py) {
            if (grabbed_band == -1) {
                for (int i = 0; i < EQ_POINTS; i++) {
                    float px = EQ_X0 + band_x_norm[i] * EQ_W;
                    if (fabsf(t.px - px) < POINT_RADIUS) { grabbed_band = i; break; }
                }
            }
            if (grabbed_band >= 0) {
                float ny = (float)(t.py - EQ_Y0) / EQ_H;
                ny = fmaxf(0.0f, fminf(1.0f, ny));
                float gain = EQ_MAX_GAIN - ny * (EQ_MAX_GAIN - EQ_MIN_GAIN);
                eq_points[grabbed_band].gain_db = gain;
                eq_gains[grabbed_band] = gain;
                EQ_UpdateBand(grabbed_band, gain);
                log_to_file("EQ band %d updated: %.2f", grabbed_band, gain);
            }
        } else grabbed_band = -1;
    } else {
                               
        if (kDown & KEY_DUP && volume_threshold < 1000) volume_threshold += 10;
        if (kDown & KEY_DDOWN && volume_threshold >= 10) volume_threshold -= 10;
        if (kDown & KEY_DRIGHT && mic.gain < 63) { mic.gain++; Mic_SetGain(&mic, mic.gain); log_to_file("Mic gain increased to %u", mic.gain); }
        if (kDown & KEY_DLEFT && mic.gain > 0) { mic.gain--; Mic_SetGain(&mic, mic.gain); log_to_file("Mic gain decreased to %u", mic.gain); }
        
                                       
        if (kDown & KEY_L) changeSampleRate(false);              
        if (kDown & KEY_R) changeSampleRate(true);             
        
        if (kDown & KEY_Y) eq_on = !eq_on;
        
                                               
        if (kDown & KEY_A) {
                                                           
            if (!mic.recording_raw && !mic.saving_in_progress) {
                if (!Mic_StartRawRecording(&mic)) {
                    mic.recording_raw = false;                                
                }
                log_to_file("Raw recording started");
            } 
                                        
            else if (mic.recording_raw) {
                Mic_StopRawRecording(&mic);                                    
                log_to_file("Raw recording stop requested");
            }
                                                                            
        }
                              
    }


                             
                                              
    const u32 MAX_SAMPLES_PER_FRAME = 1024; 

                                
    u32 hw_pos = micGetLastSampleOffset();
    u32 available_bytes;

    if (hw_pos >= mic.read_pos)
        available_bytes = hw_pos - mic.read_pos;
    else
        available_bytes = mic.data_size - mic.read_pos + hw_pos;

                                               
    u32 available_samples = available_bytes / sizeof(s16);

                                   
    if (available_samples > 0) {
        
                                                                     
        u32 samples_to_process = (available_samples > MAX_SAMPLES_PER_FRAME) ? MAX_SAMPLES_PER_FRAME : available_samples;
        
                                                                          
        s16 temp_buffer[MAX_SAMPLES_PER_FRAME];
        u32 volume_accum = 0;

        for (u32 i = 0; i < samples_to_process; i++) {
                                                          
                                                                                   
            u32 current_byte_pos = (mic.read_pos + (i * sizeof(s16))) % mic.data_size;
            
                                                 
            s16 sample;
            memcpy(&sample, &mic.buffer[current_byte_pos], sizeof(s16));

            volume_accum += abs(sample);

            if (eq_on) sample = EQ_ProcessSample(sample);
            
                                              
            temp_buffer[i] = sample;

                                     
            scopeBuf[scopeWrite] = sample;
            scopeWrite = (scopeWrite + 1) % SCOPE_SAMPLES;
        }

        last_mic_volume = volume_accum / samples_to_process;

                                                                                   
        if (mic.recording_raw)
            Mic_WriteToRawBuffer(&mic, temp_buffer, samples_to_process);

                      
                                                                             
        if (!muted && last_mic_volume > volume_threshold && !mic.recording_raw && !mic.saving_in_progress && sockfd >= 0) {
            sendto(sockfd, temp_buffer, samples_to_process * sizeof(s16), 0,
                   (struct sockaddr*)&server_addr, sizeof(server_addr));
        }

                                                                                  
          
                                                 
                                       
                                  
                                                 
         
          

                                                                      
        mic.read_pos = (mic.read_pos + (samples_to_process * sizeof(s16))) % mic.data_size;
    }
    if (fading_in) {
        fade_alpha -= 0.04f;    
        if (fade_alpha <= 0.0f) {
            fade_alpha = 0.0f;
            fading_in = false;
        }
    }

                                                     
}




                 

                                              
static const char* getSampleRateString(void) {
    switch(mic.sample_rate) {
        case MIC_RATE_32730: return "32730 Hz";
        case MIC_RATE_16360: return "16360 Hz";
        case MIC_RATE_10910: return "10910 Hz";
        case MIC_RATE_8180:  return "8180 Hz";
        default:             return "Unknown";
    }
}

void sceneMainmenuRender(void) {
    const u32 white=C2D_Color32(255,255,255,255);
    const u32 black=C2D_Color32(0,0,0,255);
    const u32 red  =C2D_Color32(255,0,0,255);
    C2D_Text tempText;
    char text_buffer[1024];
    C2D_TargetClear(top,C2D_Color32(0,0,0,255));
    C2D_SceneBegin(top);
    Draw_Kwadraty(C2D_Color32(76,178,100,255),C2D_Color32(42,103,42,255));
                                               
                                                                
    if (mic.saving_in_progress) {
        const u32 savingColor = C2D_Color32(255, 220, 0, 255);          
        const u32 bgColor = C2D_Color32(0, 0, 0, 200);                             

                          
        const char* saving_text = "Saving... Please Wait";
        C2D_TextBufClear(g_staticBuf);
        C2D_TextFontParse(&tempText, font[0], g_staticBuf, saving_text);
        C2D_TextOptimize(&tempText);

        float text_width, text_height;
        C2D_TextGetDimensions(&tempText, 0.7f, 0.7f, &text_width, &text_height);
        
        float x = 5;                              
        float y = 210;                               

                                                     
                                                                                               
        
                        
        drawShadowedText_noncentered(&tempText, x, y, 0.5f, 0.7f, 0.7f, savingColor, black);
    }
    drawMicWaveform();

    C2D_TargetClear(bottom,C2D_Color32(0,0,0,255));
    C2D_SceneBegin(bottom);
    Draw_Kwadraty(C2D_Color32(76,178,100,255),C2D_Color32(42,103,42,255));



    if(!initialized) {
        C2D_TextBufClear(g_staticBuf);
        C2D_TextFontParse(&tempText,font[0],g_staticBuf,"Mic/Network Init Failed!");
        C2D_TextOptimize(&tempText);
        drawShadowedText_noncentered(&tempText,10,10,0.5f,0.6f,0.6f,red,black);
        return;
    }

    float meter_width=200.0f;
    float filled=(last_mic_volume*meter_width)/1000.0f;
    if(filled>meter_width) filled=meter_width;

    C2D_TextBufClear(g_staticBuf);
    C2D_TextFontParse(&tempText,font[0],g_staticBuf,"Mic In:");
    C2D_TextOptimize(&tempText);
    drawShadowedText_noncentered(&tempText,10,198,0.5f,0.5f,0.5f,white,black);

    C2D_DrawRectSolid(70,200,0.5f,meter_width,20,C2D_Color32(50,50,50,255));
    C2D_DrawRectSolid(70,200,0.5f,filled,20,C2D_Color32(255,200,0,255));

                                                               
    snprintf(text_buffer,sizeof(text_buffer),
        "Streaming to: %s\n"
        "Threshold: %lu\n"
        "Mic Gain: %u\n"
        "Muted (B): %s\n"
        "Raw Rec (A): %s\n"
        "Sample Rate (L/R): %s",
        SERVER_IP, volume_threshold, mic.gain,
        muted?"ON":"OFF",
        mic.recording_raw ? "ON" : (mic.saving_in_progress ? "SAVING..." : "OFF"),
        getSampleRateString()                       
    );

    C2D_TextBufClear(g_staticBuf);
    C2D_TextFontParse(&tempText,font[0],g_staticBuf,text_buffer);
    C2D_TextOptimize(&tempText);
    if(!eqmode) drawShadowedText_noncentered(&tempText,10,10,0.5f,0.5f,0.5f,white,black);

    snprintf(text_buffer,sizeof(text_buffer),"EQ Mode (Select): %s\nEQ On (Y): %s", eqmode?"EDIT":"STREAM", eq_on?"ON":"OFF");
    C2D_TextBufClear(g_staticBuf);
    C2D_TextFontParse(&tempText,font[0],g_staticBuf,text_buffer);
    C2D_TextOptimize(&tempText);
    drawShadowedText_noncentered(&tempText,10,120,0.5f,0.5f,0.5f,white,black);

    if(eqmode) drawEQEditor();

    if (fade_alpha > 0.0f) {
        u32 fadeColor = C2D_Color32(0, 0, 0, (u8)(fade_alpha * 255));

        C2D_SceneBegin(top);
        C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, fadeColor);

        C2D_SceneBegin(bottom);
        C2D_DrawRectSolid(0, 0, 1.0f, 320, 240, fadeColor);
    }

}

               
void sceneMainmenuExit(void) {
    if(initialized) Mic_Exit(&mic);
    if(sockfd>=0) { close(sockfd); sockfd=-1; }
}