
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <wiringPi.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "dispmanx.h"

using namespace std;

char* LoadBitmap ( char *FileName, int *Width, int *Height )
{
	char* buffer = nullptr;
	int imagesize = 0;
	FILE* f = fopen( FileName, "rb");

	if(f == NULL) return NULL;

	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

	// extract image height and width from header
	int width = *(int*)&info[18];
	int height = *(int*)&info[22];
	imagesize = width*height *4;
	buffer = (char*)malloc(imagesize);
	if (buffer == NULL)
	{
		fclose(f);
		return 0;
	}
	*Width = width;
	*Height = height;
	cout << "Bitmap " << FileName << " " << width << " " << height << "\n" ;

	int row_padded = (width*3 + 3) & (~3);
	char* data = (char*)malloc(row_padded);

	for(int i = 0; i < height; i++)
	{
		char* dst = buffer+(height-i-1)*width*4;
		fread(data, sizeof(unsigned char), row_padded, f);
		for(int j = 0; j < width; j++ )
		{
			// Convert (B, G, R) to (R, G, B, A)
			char B = data[j*3+0];
			char G = data[j*3+1];
			char R = data[j*3+2];
			if( B==0 && G==255 && R==0 )
			{
				//key-color!!! transparent!!
				dst[j*4+2] = 0;
				dst[j*4+1] = 0;
				dst[j*4+0] = 0;
				dst[j*4+3] = 0;
			}
			else
			{
				//opaque
				dst[j*4+0] = B;
				dst[j*4+1] = G;
				dst[j*4+2] = R;
				dst[j*4+3] = 0xFF;
			}
		}
	}
	free(data);
	fclose(f);
	return buffer;
}

typedef struct OBJ{
	int width_;
	int height_;
	int x_;
	int y_;
	int layer_;
	char* pixels_;
} OBJ_;

#define VOLUME_DOWN_BUTTON 0
#define VOLUME_UP_BUTTON 1

int progress = 10;
struct timespec lastVolumeChange;

#define AMIXER_MIN 55
#define AMIXER_MAX 100

void cleanup(int signo)
{
    pullUpDnControl(VOLUME_DOWN_BUTTON, PUD_DOWN);
    pullUpDnControl(VOLUME_UP_BUTTON, PUD_DOWN);
    
    ofstream file;
    file.open("volume-data.txt");
    file << (progress * 10) << endl;
    file.close();
    
    exit(0);
}

void UpdateAmixer()
{
    int volume = progress * 10;
    if (volume > 100)
        volume = 100;
    else if (volume < 0)
        volume = 0;

    int setto = AMIXER_MIN + (AMIXER_MAX - AMIXER_MIN) * (volume / 100.0);
    if (volume == 0)
        setto = 0;
    
    char cmd[50];
    sprintf(cmd, "amixer -q sset HDMI %d%%", setto);
    printf("%s | volume=%d\n", cmd, volume);
    
    system(cmd);
}

unsigned short int isPressed(unsigned short int button)
{
    static struct timespec lastcall;
    struct timespec thiscall;
    float timeDiff;
    
    clock_gettime(CLOCK_REALTIME, &thiscall);
    timeDiff = (thiscall.tv_sec + thiscall.tv_nsec/1E9 - lastcall.tv_sec - lastcall.tv_nsec/1E9)*10;
    lastcall = thiscall;
    
    return timeDiff > 1 ? 1 : 0;
}

void volumeUp()
{
    if (!isPressed(VOLUME_UP_BUTTON))
        return;
    
    static unsigned short int VUwasPressed;
    if (VUwasPressed)
    {
        VUwasPressed = 0;
        return;
    }
    
    progress++;
    if (progress > 10)
        progress = 10;
    clock_gettime(CLOCK_REALTIME, &lastVolumeChange);
    UpdateAmixer();
    
    VUwasPressed = 1;
}

void volumeDown()
{
    if (!isPressed(VOLUME_UP_BUTTON))
        return;
    
    static unsigned short int VDwasPressed;
    if (VDwasPressed)
    {
        VDwasPressed = 0;
        return;
    }
    
    progress--;
    if (progress < 0)
        progress = 0;
    clock_gettime(CLOCK_REALTIME, &lastVolumeChange);
    UpdateAmixer();
    
    VDwasPressed = 1;
}

int main(int argc , char *argv[])
{
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGHUP, cleanup);
    
    wiringPiSetup();
    
    pinMode(VOLUME_UP_BUTTON, INPUT);
    pinMode(VOLUME_DOWN_BUTTON, INPUT);
    
    wiringPiISR(VOLUME_UP_BUTTON, INT_EDGE_BOTH, volumeUp);
    wiringPiISR(VOLUME_DOWN_BUTTON, INT_EDGE_BOTH, volumeDown);
    
    ifstream file("volume-data.txt");
    
    string text;
    getline (file, text);
    
    progress = stoi(text, 0, 10) / 10;
    UpdateAmixer();
    
    file.close();
    
	dispmanx_init();
    
    char* barDirs[11] = {(char*)"volumebar/bar0.bmp", (char*)"volumebar/bar10.bmp", (char*)"volumebar/bar20.bmp", (char*)"volumebar/bar30.bmp", (char*)"volumebar/bar40.bmp", (char*)"volumebar/bar50.bmp", (char*)"volumebar/bar60.bmp", (char*)"volumebar/bar70.bmp", (char*)"volumebar/bar80.bmp", (char*)"volumebar/bar90.bmp", (char*)"volumebar/bar100.bmp"};
    OBJ_ bar[11];
    struct DISPMANX_ELEMENT bar_element[11];
    
    for (int i = 0; i < 11; i++)
    {
        bar[i].pixels_ = LoadBitmap( barDirs[i], &bar[i].width_, &bar[i].height_ );
        bar[i].layer_ = 100;
        bar[i].x_ = -300;
        bar[i].y_ = 0;

        bar_element[i] = dispmanx_element_create(
            VC_IMAGE_ARGB8888, bar[i].width_, bar[i].height_, bar[i].x_, bar[i].y_, bar[i].width_, bar[i].height_, bar[i].layer_ );
        dispmanx_element_write( &bar_element[i], bar[i].pixels_ );
    }

	while(1)
	{
		this_thread::sleep_for( chrono::milliseconds(10) );
        
        for (int i = 0; i < 11; i++)
        {
            bar[i].x_ = -300;
		}
        
        struct timespec timeNow;
        clock_gettime(CLOCK_REALTIME, &timeNow);
        float timeDiff;
        timeDiff = (timeNow.tv_sec + timeNow.tv_nsec/1E9 - lastVolumeChange.tv_sec - lastVolumeChange.tv_nsec/1E9);
        
        if (timeDiff < 2)
            bar[progress].x_ = 0;

		DISPMANX_UPDATE_HANDLE_T update = dispmanx_start_update(10);
        for (int i = 0; i < 11; i++)
        {
            dispmanx_element_move( update, &bar_element[i], bar[i].x_, bar[i].y_ );
		}
        dispmanx_sync( update );
	}
    
    cleanup(0);
}

