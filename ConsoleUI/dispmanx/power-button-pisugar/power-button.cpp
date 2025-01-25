
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <wiringPi.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

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

unsigned short int isPwrBtnPressed;
struct timespec pwrBtnPressTime;

void checkPwrBtnPressed()
{
    FILE *fp;
    char var[40];

    fp = popen("i2cget -y 1 0x57 0x02", "r");
    while (fgets(var, sizeof(var), fp) != NULL)
    {
        //printf("%s", var);
    }
    pclose(fp);

    if (var[0] == 0)
    {
        return;
    }
    
    char *output;
    int reg = strtol(var, &output, 16);
    int isPressed = reg & 0x01;

    if (isPwrBtnPressed == 0 && isPressed == 1)
    {
        clock_gettime(CLOCK_REALTIME, &pwrBtnPressTime);
    }

    isPwrBtnPressed = isPressed;

    return;
}

void cleanup(int signo)
{
    exit(0);
}

#define PWR_ICON_WIDTH 30

int main(int argc , char *argv[])
{
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGHUP, cleanup);
    
	dispmanx_init();
    
    char* pwrIconDirs[3] = {"icon/icon3.bmp", "icon/icon2.bmp", "icon/icon1.bmp"};
    OBJ_ pwrIcon[3];
    struct DISPMANX_ELEMENT pwrIcon_element[3];
    
    for (int i = 0; i < 3; i++)
    {
        pwrIcon[i].pixels_ = LoadBitmap( pwrIconDirs[i], &pwrIcon[i].width_, &pwrIcon[i].height_ );
        pwrIcon[i].layer_ = 100;
        pwrIcon[i].x_ = -300;
        pwrIcon[i].y_ = 0;

        pwrIcon_element[i] = dispmanx_element_create(
            VC_IMAGE_ARGB8888, pwrIcon[i].width_, pwrIcon[i].height_, pwrIcon[i].x_, pwrIcon[i].y_, pwrIcon[i].width_, pwrIcon[i].height_, pwrIcon[i].layer_ );
        dispmanx_element_write( &pwrIcon_element[i], pwrIcon[i].pixels_ );
    }

	while(1)
	{
		this_thread::sleep_for( chrono::milliseconds(100) );
        
        for (int i = 0; i < 3; i++)
        {
            pwrIcon[i].x_ = -300;
		}
        
        checkPwrBtnPressed();
        if (isPwrBtnPressed)
        {
            struct timespec timeNow;
            clock_gettime(CLOCK_REALTIME, &timeNow);
            float timeDiff;
            timeDiff = (timeNow.tv_sec + timeNow.tv_nsec/1E9 - pwrBtnPressTime.tv_sec - pwrBtnPressTime.tv_nsec/1E9);
            
            if (timeDiff > 3)
            {
                system("sudo shutdown -h now");
            }
            else if (timeDiff > 2)
            {
                pwrIcon[2].x_ = 800 / 2 - PWR_ICON_WIDTH / 2;
            }
            else if (timeDiff > 1)
            {
                pwrIcon[1].x_ = 800 / 2 - PWR_ICON_WIDTH / 2;
            }
            else
            {
                pwrIcon[0].x_ = 800 / 2 - PWR_ICON_WIDTH / 2;
            }
        }

		DISPMANX_UPDATE_HANDLE_T update = dispmanx_start_update(10);
        for (int i = 0; i < 3; i++)
        {
            dispmanx_element_move( update, &pwrIcon_element[i], pwrIcon[i].x_, pwrIcon[i].y_ );
		}
        dispmanx_sync( update );
	}
    
    cleanup(0);
}

