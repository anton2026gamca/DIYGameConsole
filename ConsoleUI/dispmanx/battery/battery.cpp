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
	int barsize = 0;
	FILE* f = fopen( FileName, "rb");

	if(f == NULL) return NULL;

	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

	// extract bar height and width from header
	int width = *(int*)&info[18];
	int height = *(int*)&info[22];
	barsize = width*height *4;
	buffer = (char*)malloc(barsize);
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

void cleanup(int signo)
{
    exit(0);
}

#define BARS 11
#define BAR_WIDTH 76
#define DIGITS 10

#define DIGIT_0_OFFSET_X 7
#define DIGIT_1_OFFSET_X 16
#define DIGIT_2_OFFSET_X 25
#define DIGITS_OFFSET_Y 7

#define DISPLAY_AVG_FROM_MEASUREMENTS 10

int get_battery_percentage()
{
	FILE *fp,*outputfile;
    char var[40];

    fp = popen("echo $((`i2cget -y 1 0x57 0x2a`))", "r");
    while (fgets(var, sizeof(var), fp) != NULL)
    {
        //printf("%s", var);
    }
    pclose(fp);

    char *output;
    int percentage = strtol(var, &output, 10);

	return percentage;
}

int get_is_charging()
{
    FILE *fp,*outputfile;
    char var[40];

    fp = popen("echo $((`i2cget -y 1 0x57 0x02` >> 7))", "r");
    while (fgets(var, sizeof(var), fp) != NULL)
    {
        //printf("%s", var);
    }
    pclose(fp);

    char *output;
    int isCharging = strtol(var, &output, 10);

	return isCharging;
}

int main(int argc , char *argv[])
{
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGHUP, cleanup);
    
    dispmanx_init();
    
    char* barDirs[BARS] = {(char*)"resources/bar0.bmp", (char*)"resources/bar10.bmp", (char*)"resources/bar20.bmp", (char*)"resources/bar30.bmp", (char*)"resources/bar40.bmp", (char*)"resources/bar50.bmp", (char*)"resources/bar60.bmp", (char*)"resources/bar70.bmp", (char*)"resources/bar80.bmp", (char*)"resources/bar90.bmp", (char*)"resources/bar100.bmp"};
    OBJ_ bar[BARS];
    struct DISPMANX_ELEMENT bar_element[BARS];

	char* chargingDir = "resources/charging.bmp";
	OBJ_ charging;
	struct DISPMANX_ELEMENT charging_element;
    
    char* digit0Dir = "resources/1.bmp";
    OBJ_ digit0;
    struct DISPMANX_ELEMENT digit0_element;
    
    char* digit1Dirs[DIGITS] = {(char*)"resources/0.bmp", (char*)"resources/1.bmp", (char*)"resources/2.bmp", (char*)"resources/3.bmp", (char*)"resources/4.bmp", (char*)"resources/5.bmp", (char*)"resources/6.bmp", (char*)"resources/7.bmp", (char*)"resources/8.bmp", (char*)"resources/9.bmp"};
    OBJ_ digit1[DIGITS];
    struct DISPMANX_ELEMENT digit1_element[DIGITS];
    
    char* digit2Dirs[DIGITS] = {(char*)"resources/0.bmp", (char*)"resources/1.bmp", (char*)"resources/2.bmp", (char*)"resources/3.bmp", (char*)"resources/4.bmp", (char*)"resources/5.bmp", (char*)"resources/6.bmp", (char*)"resources/7.bmp", (char*)"resources/8.bmp", (char*)"resources/9.bmp"};
    OBJ_ digit2[DIGITS];
    struct DISPMANX_ELEMENT digit2_element[DIGITS];
    
    for (int i = 0; i < BARS; i++)
    {
        bar[i].pixels_ = LoadBitmap( barDirs[i], &bar[i].width_, &bar[i].height_ );
        bar[i].layer_ = 100;
        bar[i].x_ = -512;
        bar[i].y_ = 0;

        bar_element[i] = dispmanx_element_create(VC_IMAGE_ARGB8888, bar[i].width_, bar[i].height_, bar[i].x_, bar[i].y_, bar[i].width_, bar[i].height_, bar[i].layer_ );
        dispmanx_element_write( &bar_element[i], bar[i].pixels_ );
    }

	charging.pixels_ = LoadBitmap( chargingDir, &charging.width_, &charging.height_ );
	charging.layer_ = 101;
	charging.x_ = -512;
	charging.y_ = 0;
	charging_element = dispmanx_element_create(VC_IMAGE_ARGB8888, charging.width_, charging.height_, charging.x_, charging.y_, charging.width_, charging.height_, charging.layer_ );
	dispmanx_element_write( &charging_element, charging.pixels_ );
    
    digit0.pixels_ = LoadBitmap( digit0Dir, &digit0.width_, &digit0.height_ );
	digit0.layer_ = 101;
	digit0.x_ = -512;
	digit0.y_ = DIGITS_OFFSET_Y;
	digit0_element = dispmanx_element_create(VC_IMAGE_ARGB8888, digit0.width_, digit0.height_, digit0.x_, digit0.y_, digit0.width_, digit0.height_, digit0.layer_ );
	dispmanx_element_write( &digit0_element, digit0.pixels_ );
    
    for (int i = 0; i < DIGITS; i++)
    {
        digit1[i].pixels_ = LoadBitmap( digit1Dirs[i], &digit1[i].width_, &digit1[i].height_ );
        digit1[i].layer_ = 102;
        digit1[i].x_ = -512;
        digit1[i].y_ = DIGITS_OFFSET_Y;

        digit1_element[i] = dispmanx_element_create(VC_IMAGE_ARGB8888, digit1[i].width_, digit1[i].height_, digit1[i].x_, digit1[i].y_, digit1[i].width_, digit1[i].height_, digit1[i].layer_ );
        dispmanx_element_write( &digit1_element[i], digit1[i].pixels_ );
    }
    
    for (int i = 0; i < DIGITS; i++)
    {
        digit2[i].pixels_ = LoadBitmap( digit2Dirs[i], &digit2[i].width_, &digit2[i].height_ );
        digit2[i].layer_ = 102;
        digit2[i].x_ = -512;
        digit2[i].y_ = DIGITS_OFFSET_Y;

        digit2_element[i] = dispmanx_element_create(VC_IMAGE_ARGB8888, digit2[i].width_, digit2[i].height_, digit2[i].x_, digit2[i].y_, digit2[i].width_, digit2[i].height_, digit2[i].layer_ );
        dispmanx_element_write( &digit2_element[i], digit2[i].pixels_ );
    }
    
    int percentages[DISPLAY_AVG_FROM_MEASUREMENTS + 1];
    for (int i = 0; i < DISPLAY_AVG_FROM_MEASUREMENTS + 1; i++)
    {
        percentages[i] = -1;
    }
    int numPercentages = 0;

	while(1)
	{
        for (int i = 0; i < BARS; i++)
        {
            bar[i].x_ = -512;
		}

		int battery = get_battery_percentage();
        
        if (numPercentages < DISPLAY_AVG_FROM_MEASUREMENTS) 
        {
            percentages[numPercentages] = battery;
            numPercentages++;
        }
        else
        {
            for (int i = 0; i < numPercentages - 1; i++)
            {
                percentages[i] = percentages[i + 1];
            }
            percentages[numPercentages - 1] = battery;
        }
        
        int battery_avg = 0;
        for (int i = 0; i < numPercentages; i++)
        {
            battery_avg += percentages[i];
        }
        battery_avg /= numPercentages;
    
        int isCharging = get_is_charging();
        
        int displayBar = battery_avg / 10;
        
		bar[displayBar].x_ = 800 - BAR_WIDTH;
        if (isCharging == 0)
            charging.x_ = -512;
        else
            charging.x_ = 800 - BAR_WIDTH;

		cout << " " << battery_avg << " | " << battery << " | " << displayBar << " | " << (isCharging == 0 ? "Not Charging" : "  Charging  ") << " | ";
        for (int i = 0; i < numPercentages; i++)
        {
            cout << percentages[i] << " ";
        }
        cout << endl;
        
        for (int i = 0; i < DIGITS; i++)
        {
            digit1[i].x_ = -512;
            digit2[i].x_ = -512;    
        }
        if (battery_avg != 0)
            digit1[(battery_avg / 10 > 9) ? 0 : (battery_avg / 10)].x_ = 800 - BAR_WIDTH + DIGIT_1_OFFSET_X;
        digit2[battery_avg % 10].x_ = 800 - BAR_WIDTH + DIGIT_2_OFFSET_X;
        digit0.x_ = (battery_avg / 100 == 1) ? (800 - BAR_WIDTH + DIGIT_0_OFFSET_X) : -512;

		DISPMANX_UPDATE_HANDLE_T update = dispmanx_start_update(10);
        for (int i = 0; i < BARS; i++)
        {
            dispmanx_element_move( update, &bar_element[i], bar[i].x_, bar[i].y_ );
		}
        dispmanx_element_move( update, &charging_element, charging.x_, charging.y_ );
        dispmanx_element_move( update, &digit0_element, digit0.x_, digit0.y_ );
        for (int i = 0; i < DIGITS; i++)
        {
            dispmanx_element_move( update, &digit1_element[i], digit1[i].x_, digit1[i].y_ );
            dispmanx_element_move( update, &digit2_element[i], digit2[i].x_, digit2[i].y_ );
		}
        dispmanx_sync( update );
        
		this_thread::sleep_for( chrono::milliseconds(3000) );
	}
    
    cleanup(0);
}

