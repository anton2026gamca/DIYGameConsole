#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

#ifndef LIB4X11_H
#define LIB4X11_H

/* SHAPES */

void DrawRectangle(Display *display, Drawable buffer, GC gc, int x, int y, int width, int height, unsigned long color);
void DrawRectangleOutline(Display *display, Drawable buffer, GC gc, int x, int y, int width, int height, unsigned long color);

void DrawRoundedRectangle(Display *display, Drawable buffer, GC gc, int x, int y, int width, int height, int radius, unsigned long color);
void DrawRoundedRectangleOutline(Display *display, Drawable buffer, GC gc, int x, int y, int width, int height, int radius, unsigned long color);

void DrawCircle(Display *display, Drawable buffer, GC gc, int xc, int yc, int radius);
void DrawCircleOutline(Display *display, Drawable buffer, GC gc, int xc, int yc, int radius);

void DrawText(Display *display, Drawable buffer, GC gc, int x, int y, const char *text, unsigned long color, XFontStruct *font);
void DrawTextFromPivot(Display *display, Drawable buffer, GC gc, int x, int y, float pivot_x, float pivot_y, const char *text, unsigned long color, XFontStruct *font);
void DrawTextCentered(Display *display, Drawable buffer, GC gc, int x, int y, const char *text, unsigned long color, XFontStruct *font);
int GetTextWidth(Display *display, GC gc, const char *text, XFontStruct *font);
int GetTextHeight(Display *display, GC gc, const char *text, XFontStruct *font);

bool IsPointInRect(float point_x, float point_y, float rect_x, float rect_y, float rect_width, float rect_height);

/* SPRITE */

void LoadPNG(const char *filename, unsigned char **image_data, int *width, int *height, int *channels);
void DrawImage(Display *display, Drawable buffer, GC gc, XImage *ximage, int dest_x, int dest_y);

typedef struct
{
    XImage *ximage;
    unsigned char *img_data;
    float x;
    float y;
    int width;
    int height;
    int channels;
} Sprite;

Sprite LoadSprite(Display *display, XVisualInfo vinfo, const char *filename, int x, int y);
void DrawSprite(Display *display, Drawable buffer, GC gc, Sprite *sprite);

/* BUTTON */

typedef struct
{
    int x;
    int y;
    int width;
    int height;
    unsigned long normal_color;
    unsigned long hover_color;
    unsigned long pressed_color;
    unsigned long text_color;
    unsigned long outline_color;
    char *text;
    XFontStruct *font;
} Button;

void DrawButton(Display *display, Drawable buffer, GC gc, Button btn, int mouse_x, int mouse_y, bool mouse_pressed);
bool IsButtonPressed(Button btn, int mouse_x, int mouse_y, bool mouse_pressed);

#endif /* LIB4X11_H */