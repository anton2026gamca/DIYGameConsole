// Compile with: gcc main.c -o overlay lib/*.c -Ilib -lX11 -lXrender -lXfixes -lXft -lpng -lXext -lasound -lgpiod `pkg-config --cflags --libs freetype2`

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "lib4x11.h"
#include "pisugar.c"
#include "volume.c"
#include "gpiobutton.c"

unsigned int window_width = 800;
unsigned int window_height = 480;

#ifdef DEBUG
    #define VOLUME_DEBUG
    #define GPIOBUTTON_DEBUG
#endif

typedef struct {
    Display* display;
    Window window;
    Pixmap buffer;
    int screen_num;
    XVisualInfo vinfo;
    GC gc;
} Context;
typedef struct {
    int targetFPS;
    double targetFrameTime;
    struct timespec frame_start_time;
    struct timespec time_now;
    double deltatime;
    double fps;
} TimeManagement;
typedef struct {
    int x;
    int y;
    bool btn_left_down;
    bool btn_left_pressed;
    bool btn_left_released;
} MouseData;

Context ctx;
TimeManagement tm = {60, 0.016666};
MouseData mouse = {0, 0, false, false, false};

GPIOButton volume_up;
GPIOButton volume_down;

void SetTargetFPS(int fps)
{
    tm.targetFPS = fps;
    tm.targetFrameTime = 1.0 / fps;
}

/// @brief 
/// @param x Mouse X return
/// @param y Mouse Y return
void GetMousePosition(int *x, int *y)
{
    int root_x, root_y = 0;
    unsigned int mask = 0;
    Window child_win, root_win;
    XQueryPointer(ctx.display, XRootWindow(ctx.display, ctx.screen_num),
        &child_win, &root_win,
        &root_x, &root_y, x, y, &mask);
}

/// @brief Update mouse variables
void MouseUpdate(XEvent event)
{
    GetMousePosition(&mouse.x, &mouse.y);
    mouse.btn_left_pressed = event.type == ButtonPress;
    mouse.btn_left_released = event.type == ButtonRelease;
    if (mouse.btn_left_pressed)
        mouse.btn_left_down = true;
    else if (mouse.btn_left_released)
        mouse.btn_left_down = false;
}

/// @brief Initialize the window
int Init()
{
    ctx.display = XOpenDisplay(NULL);
    if (!ctx.display)
    {
        fprintf(stderr, "Unable to open X ctx.display\n");
        return 1;
    }

    ctx.screen_num = DefaultScreen(ctx.display);
    Window root = RootWindow(ctx.display, ctx.screen_num);

    unsigned int screen_width, screen_height;
    int x, y;
    unsigned int border_width, depth;
    if (!XGetGeometry(ctx.display, root, &root, &x, &y, &screen_width, &screen_height, &border_width, &depth)) {
        fprintf(stderr, "Failed to get screen geometry\n");
        return 1;
    }
    window_width = screen_width;
    window_height = screen_height;

    // Find an ARGB visual for transparency
    if (!XMatchVisualInfo(ctx.display, ctx.screen_num, 32, TrueColor, &ctx.vinfo))
    {
        fprintf(stderr, "No ARGB visual found\n");
        return 1;
    }

    // Create a colormap
    Colormap colormap = XCreateColormap(ctx.display, root, ctx.vinfo.visual, AllocNone);

    // Create a transparent window
    XSetWindowAttributes attrs;
    attrs.colormap = colormap;
    attrs.border_pixel = 0;
    attrs.background_pixel = 0; // Fully transparent background
    attrs.override_redirect = True; // Bypass window manager

    ctx.window = XCreateWindow(ctx.display, root, 0, 0, window_width, window_height, 0, ctx.vinfo.depth, InputOutput, ctx.vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attrs);

    // Make the window input transparent (click-through)
    XserverRegion region = XFixesCreateRegion(ctx.display, NULL, 0);
    XFixesSetWindowShapeRegion(ctx.display, ctx.window, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(ctx.display, region);

    // Select input events
    XSelectInput(ctx.display, ctx.window, ButtonPressMask | ButtonReleaseMask);

    // Map the window
    XMapWindow(ctx.display, ctx.window);

    // Create a graphics context (GC)
    ctx.gc = XCreateGC(ctx.display, ctx.window, 0, NULL);
    
    // Create a double-buffering Pixmap
    ctx.buffer = XCreatePixmap(ctx.display, ctx.window, window_width, window_height, ctx.vinfo.depth);

    clock_gettime( CLOCK_REALTIME, &tm.frame_start_time );
    tm.frame_start_time.tv_sec -= 1;
    clock_gettime( CLOCK_REALTIME, &tm.time_now );

    return 0;
}

/// @brief Deinitialize the window
void Deinit()
{
    XFreePixmap(ctx.display, ctx.buffer);
    XFreeGC(ctx.display, ctx.gc);
    XDestroyWindow(ctx.display, ctx.window);
    XCloseDisplay(ctx.display);
}

/// @brief Change the input mask of the window
/// @param rectangles Rectangles where the window is input-opaque
void UpdateInputMask(XRectangle *rectangles, int num_rectangles)
{
    // Create a pixmap for the mask
    Pixmap mask = XCreatePixmap(ctx.display, ctx.window, window_width, window_height, 1);
    GC mask_gc = XCreateGC(ctx.display, mask, 0, NULL);

    // Set everything as input-transparent
    XSetForeground(ctx.display, mask_gc, 0); // Transparent
    XFillRectangle(ctx.display, mask, mask_gc, 0, 0, window_width, window_height);

    // Set the specified rectangles as input-opaque
    XSetForeground(ctx.display, mask_gc, 1); // Opaque
    for (int i = 0; i < num_rectangles; i++) {
        XFillRectangle(ctx.display, mask, mask_gc, rectangles[i].x, rectangles[i].y, rectangles[i].width, rectangles[i].height);
    }

    // Apply the input shape
    XShapeCombineMask(ctx.display, ctx.window, ShapeInput, 0, 0, mask, ShapeSet);

    // Cleanup
    XFreePixmap(ctx.display, mask);
    XFreeGC(ctx.display, mask_gc);
}

/// @brief Clear the buffer
void Clear()
{
    XSetForeground(ctx.display, ctx.gc, 0x00000000);
    XFillRectangle(ctx.display, ctx.buffer, ctx.gc, 0, 0, window_width, window_height);
}

/// @brief Draw buffer to window
void UpdateScreen()
{
    // Copy the buffer to the window
    XCopyArea(ctx.display, ctx.buffer, ctx.window, ctx.gc, 0, 0, window_width, window_height, 0, 0);
    XFlush(ctx.display);

    // Wait until deltatime is ok
    clock_gettime( CLOCK_REALTIME, &tm.time_now );
    double deltatime = (tm.time_now.tv_sec + tm.time_now.tv_nsec/1E9 - tm.frame_start_time.tv_sec - tm.frame_start_time.tv_nsec/1E9);
    do {
        clock_gettime( CLOCK_REALTIME, &tm.time_now );
        deltatime = (tm.time_now.tv_sec + tm.time_now.tv_nsec/1E9 - tm.frame_start_time.tv_sec - tm.frame_start_time.tv_nsec/1E9);
    } while (deltatime < tm.targetFrameTime);
    clock_gettime( CLOCK_REALTIME, &tm.frame_start_time );
    tm.deltatime = deltatime;
    tm.fps = 1.0 / tm.deltatime;
}

void DrawBatteryIcon(int battery_percentage, bool is_charging, Sprite *charging_icon, XFontStruct *font)
{
    int margin_top = 1, margin_right = 1;
    int pad_left = 2, pad_right = 2, pad_top = 2, pad_bottom = 2;
    int spacing = 5;
    int radius = 3;
    char text[100];
    sprintf(text, "%d%%", battery_percentage);
    int text_width = GetTextWidth(ctx.display, ctx.gc, text, font);
    int text_height = GetTextHeight(ctx.display, ctx.gc, text, font);
    int icon_width = 22;
    int width = pad_left + text_width + spacing + icon_width + 2 + pad_right;
    DrawRoundedRectangle(ctx.display, ctx.buffer, ctx.gc, window_width - width - margin_right, margin_top, width, pad_top + text_height + pad_bottom, radius, 0xFF000000);
    int text_x = window_width - spacing - (icon_width + 2) - pad_right - margin_right;
    int text_y = margin_top + pad_top + text_height / 2;
    DrawTextFromPivot(ctx.display, ctx.buffer, ctx.gc, text_x, text_y - 1, 1, 0.5, text, 0xFFFFFFFF, font);
    int icon_x = window_width - (icon_width + 2) - pad_right - margin_right;
    int icon_y = margin_top + pad_top;
    unsigned long icon_color = 0xFFFFFFFF;
    if (battery_percentage <= 10)
        icon_color = 0xFFFF0000;
    unsigned long icon_fill_color = 0xff4dff00;
    if (battery_percentage <= 10)
        icon_fill_color = 0xFFFF0000;
    else if (battery_percentage <= 20)
        icon_fill_color = 0xffff8400;
    else if (battery_percentage <= 30)
        icon_fill_color = 0xffffd500;
    else if (battery_percentage <= 40)
        icon_fill_color = 0xffc3ff00;
    else if (battery_percentage <= 50)
        icon_fill_color = 0xff84ff00;
    DrawRectangleOutline(ctx.display, ctx.buffer, ctx.gc, icon_x, icon_y, icon_width, text_height, icon_color);
    DrawRectangle(ctx.display, ctx.buffer, ctx.gc, icon_x + icon_width, icon_y + (float)(text_height - 4) / 2, 2, 4, icon_color);
    DrawRectangle(ctx.display, ctx.buffer, ctx.gc, icon_x + 1, icon_y + 1, (icon_width - 2) * battery_percentage / 100, text_height - 2, icon_fill_color);
    if (is_charging)
    {
        charging_icon->x = icon_x + (icon_width - charging_icon->width) / 2;
        charging_icon->y = icon_y + (text_height - charging_icon->height) / 2;
        DrawSprite(ctx.display, ctx.buffer, ctx.gc, charging_icon);
    }
}

void VolumeDraw(int volume, bool is_muted, struct timespec last_change, Sprite *mute_sprite)
{
    struct timespec time_now;
    clock_gettime(CLOCK_REALTIME, &time_now);
    float time_diff = (time_now.tv_sec + time_now.tv_nsec / 1E9) - (last_change.tv_sec + last_change.tv_nsec / 1E9);

    if (time_diff < VOLUMEBAR_DURATION)
    {
        int margin_top = 1, margin_left = 1;
        int pad_left = 2, pad_right = 2, pad_top = 2, pad_bottom = 2;
        int width = 102 + pad_left + pad_right;
        int height = 14;

        DrawRoundedRectangle(ctx.display, ctx.buffer, ctx.gc, margin_left, margin_top, width, height, 3, 0xFF202020);
        if (!is_muted)
        {
            DrawRectangle(ctx.display, ctx.buffer, ctx.gc, margin_left + pad_left + 1, margin_top + pad_top + 1, ((float)volume / 100) * (width - pad_left - pad_right - 2), height - pad_top - pad_bottom - 2, 0xFFFFFFFF);
            DrawRoundedRectangleOutline(ctx.display, ctx.buffer, ctx.gc, margin_left + pad_left, margin_top + pad_top, width - pad_left - pad_right, height - pad_top - pad_bottom, 3, 0xFFFFFFFF);
        }
        else
        {
            DrawRectangle(ctx.display, ctx.buffer, ctx.gc, margin_left + pad_left + 1, margin_top + pad_top + 1, ((float)volume / 100) * (width - pad_left - pad_right - 2), height - pad_top - pad_bottom - 2, 0xFFFF0000);
            DrawRoundedRectangleOutline(ctx.display, ctx.buffer, ctx.gc, margin_left + pad_left, margin_top + pad_top, width - pad_left - pad_right, height - pad_top - pad_bottom, 3, 0xFFFF0000);
            mute_sprite->x = margin_left + (width - mute_sprite->width) / 2;
            mute_sprite->y = margin_top + (height - mute_sprite->height) / 2;
            DrawSprite(ctx.display, ctx.buffer, ctx.gc, mute_sprite);
        }

    }
}

void DrawShutdownLog(int shutdown_in, XFontStruct *font)
{
    char text[25];
    sprintf(text, "Shutting down in %d", shutdown_in);
    int pad_left = 5, pad_right = 5, pad_top = 3, pad_bottom = 5;
    int margin_top = 1;
    int text_width = GetTextWidth(ctx.display, ctx.gc, text, font);
    int text_height = GetTextHeight(ctx.display, ctx.gc, text, font);
    DrawRoundedRectangle(ctx.display, ctx.buffer, ctx.gc, (window_width - (text_width + pad_left + pad_right)) / 2, margin_top, text_width + pad_left + pad_right, text_height + pad_top + pad_bottom, 3, 0xFF202020);
    DrawTextFromPivot(ctx.display, ctx.buffer, ctx.gc, window_width / 2, margin_top + pad_top, 0.5, 0, text, 0xFFFFFFFF, font);
}

void Exit()
{
    Deinit();
    VolumeDeinit();
#ifdef DEBUG
    printf("\nExiting successfully ...");
#endif
    printf("\n");
    exit(0);
}

int main()
{
    signal(SIGINT, Exit);

    // Initialize the window
    int err = Init();
    if (err > 0)
        return err;

    SetTargetFPS(15);

    VolumeInit();
    GPIOButtonInit(&volume_up, 22);
    GPIOButtonInit(&volume_down, 23);

    // Load different fonts
    XFontStruct *font_default = XLoadQueryFont(ctx.display, "fixed");
    XFontStruct *font_verysmall = XLoadQueryFont(ctx.display, "lucidasans-8");
    XFontStruct *font_small = XLoadQueryFont(ctx.display, "lucidasans-10");
    XFontStruct *font_medium = XLoadQueryFont(ctx.display, "lucidasans-14");
    XFontStruct *font_large = XLoadQueryFont(ctx.display, "lucidasans-18");

    Sprite charging = LoadSprite(ctx.display, ctx.vinfo, "resources/charging.png", 0, 0);
    Sprite mute_sprite = LoadSprite(ctx.display, ctx.vinfo, "resources/mute.png", 0, 0);

    for (int i = 0; i < BATTERY_SHOW_AVG_FROM; i++)
        battery.readings[i] = -1;
    clock_gettime(CLOCK_REALTIME, &battery.last_read_time);
    battery.last_read_time.tv_sec -= 1;

    bool pwrbtn_state = false;
    struct timespec pwrbtn_press_time = {0, 0};

    // Event loop
    XEvent event;
    while (1)
    {
        while (XPending(ctx.display))
        {
            XNextEvent(ctx.display, &event);
        }

        MouseUpdate(event);
        Clear();
        struct timespec time_now;
        clock_gettime(CLOCK_REALTIME, &time_now);

        // Draw the battery icon
        UpdateBattery();
        DrawBatteryIcon(battery.value, battery.is_charging, &charging, font_verysmall);

        // Check for volume changes
        VolumeUpdate();
        VolumeDraw(volume.value, volume.is_muted, volume.last_change_time, &mute_sprite);

        // Check for volume buttons
        GPIOButtonUpdate(&volume_up);
        if (volume_up.state == 1)
            ChangeVolumeBy(5);
        GPIOButtonUpdate(&volume_down);
        if (volume_down.state == 1)
            ChangeVolumeBy(-5);

        if (IsPowerButtonDown())
        {
            if (!pwrbtn_state)
            {
                pwrbtn_state = true;
                pwrbtn_press_time = time_now;
            }

            float diff = (time_now.tv_sec + time_now.tv_nsec / 1E9) - (pwrbtn_press_time.tv_sec + pwrbtn_press_time.tv_nsec / 1E9);
            if (diff > 3)
            {
                system("sudo shutdown -h now");
            }

            DrawShutdownLog((int)(3.9 - diff), font_small);
        }
        else if (pwrbtn_state)
        {
            pwrbtn_state = false;
        }

        // Draw buffer to window
        UpdateScreen();
    }
    
    // Cleanup
    XFreeFont(ctx.display, font_small);
    XFreeFont(ctx.display, font_medium);
    XFreeFont(ctx.display, font_large);
    Exit();

    return 0;
}
