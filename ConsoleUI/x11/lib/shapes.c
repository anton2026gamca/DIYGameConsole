#include "lib4x11.h"
#include <X11/Xlib.h>
#include <stdbool.h>
#include <string.h>

void DrawRectangle(Display *display, Drawable buffer, GC gc, int x, int y, int width, int height, unsigned long color)
{
    XSetForeground(display, gc, color);
    XFillRectangle(display, buffer, gc, x, y, width, height);
}

void DrawRectangleOutline(Display *display, Drawable buffer, GC gc, int x, int y, int width, int height, unsigned long color)
{
    XSetForeground(display, gc, color);
    XDrawRectangle(display, buffer, gc, x, y, width - 1, height - 1);
}

void DrawRoundedRectangle(Display *display, Drawable buffer, GC gc, int x, int y, int width, int height, int radius, unsigned long color)
{
    XSetForeground(display, gc, color);

    // Ensure the radius does not exceed half the width or height
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;

    int diameter = 2 * radius;

    // Fill the four corner arcs
    DrawCircle(display, buffer, gc, x + radius, y + radius, radius);                                 // Top-left
    DrawCircle(display, buffer, gc, x + width - radius - 1, y + radius, radius);                         // Top-right
    DrawCircle(display, buffer, gc, x + radius, y + height - radius - 1, radius);                        // Bottom-left
    DrawCircle(display, buffer, gc, x + width - radius - 1, y + height - radius - 1, radius);                // Bottom-right

    // Fill the central rectangle (middle body)
    XFillRectangle(display, buffer, gc, x + radius, y, width - diameter, height);

    // Fill the left and right rectangles (to cover the areas between the arcs)
    XFillRectangle(display, buffer, gc, x, y + radius, radius, height - diameter);
    XFillRectangle(display, buffer, gc, x + width - radius, y + radius, radius, height - diameter);
}

void DrawRoundedRectangleOutline(Display *display, Drawable buffer, GC gc, int x, int y, int width, int height, int radius, unsigned long color)
{
    width--;
    height--;

    XSetForeground(display, gc, color);

    // Ensure the radius does not exceed half the width or height
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;

    int diameter = 2 * radius;

    // Draw the four corner arcs
    XDrawArc(display, buffer, gc, x, y, diameter, diameter, 90 * 64, 90 * 64);                       // Top-left
    XDrawArc(display, buffer, gc, x + width - diameter, y, diameter, diameter, 0, 90 * 64);          // Top-right
    XDrawArc(display, buffer, gc, x, y + height - diameter, diameter, diameter, 180 * 64, 90 * 64);  // Bottom-left
    XDrawArc(display, buffer, gc, x + width - diameter, y + height - diameter, diameter, diameter, 270 * 64, 90 * 64); // Bottom-right

    // Draw the four connecting lines
    XDrawLine(display, buffer, gc, x + radius, y, x + width - radius, y);                            // Top edge
    XDrawLine(display, buffer, gc, x + radius, y + height, x + width - radius, y + height);          // Bottom edge
    XDrawLine(display, buffer, gc, x, y + radius, x, y + height - radius);                           // Left edge
    XDrawLine(display, buffer, gc, x + width, y + radius, x + width, y + height - radius);           // Right edge
}

void DrawCircle(Display *display, Drawable buffer, GC gc, int xc, int yc, int radius) {
    int x = 0, y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        // Draw horizontal lines to fill the circle
        XDrawLine(display, buffer, gc, xc - x, yc + y, xc + x, yc + y);
        XDrawLine(display, buffer, gc, xc - x, yc - y, xc + x, yc - y);
        XDrawLine(display, buffer, gc, xc - y, yc + x, xc + y, yc + x);
        XDrawLine(display, buffer, gc, xc - y, yc - x, xc + y, yc - x);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void DrawCircleOutline(Display *display, Drawable buffer, GC gc, int xc, int yc, int radius) {
    int x = 0, y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        // Draw symmetric points
        XDrawPoint(display, buffer, gc, xc + x, yc + y);
        XDrawPoint(display, buffer, gc, xc - x, yc + y);
        XDrawPoint(display, buffer, gc, xc + x, yc - y);
        XDrawPoint(display, buffer, gc, xc - x, yc - y);
        XDrawPoint(display, buffer, gc, xc + y, yc + x);
        XDrawPoint(display, buffer, gc, xc - y, yc + x);
        XDrawPoint(display, buffer, gc, xc + y, yc - x);
        XDrawPoint(display, buffer, gc, xc - y, yc - x);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void DrawText(Display *display, Drawable buffer, GC gc, int x, int y, const char *text, unsigned long color, XFontStruct *font)
{
    XSetForeground(display, gc, color);
    XSetFont(display, gc, font->fid);
    XDrawString(display, buffer, gc, x, y, text, strlen(text));
}

/// @brief 
/// @param pivot_x 0 = left, 0.5 = center, 1 = right
/// @param pivot_y 0 = top, 0.5 = center, 1 = top
void DrawTextFromPivot(Display *display, Drawable buffer, GC gc, int x, int y, float pivot_x, float pivot_y, const char *text, unsigned long color, XFontStruct *font)
{
    int text_length = strlen(text);

    XSetFont(display, gc, font->fid);

    // Get text dimensions
    XCharStruct text_info;
    int direction;
    int ascent; /* baseline to top edge of raster */
    int descent; /* baseline to bottom edge of raster */
    XTextExtents(font, text, text_length, &direction, &ascent, &descent, &text_info);

    int text_width = text_info.width;
    int text_height = ascent;

    int new_x = x - text_width * pivot_x;
    int new_y = y + text_height * (1 - pivot_y);

    DrawText(display, buffer, gc, new_x, new_y, text, color, font);
}

void DrawTextCentered(Display *display, Drawable buffer, GC gc, int x, int y, const char *text, unsigned long color, XFontStruct *font)
{
    DrawTextFromPivot(display, buffer, gc, x, y, 0.5, 0.5, text, color, font);
}

int GetTextWidth(Display *display, GC gc, const char *text, XFontStruct *font)
{
    int text_length = strlen(text);

    XSetFont(display, gc, font->fid);

    // Get text dimensions
    XCharStruct text_info;
    int direction;
    int ascent; /* baseline to top edge of raster */
    int descent; /* baseline to bottom edge of raster */
    XTextExtents(font, text, text_length, &direction, &ascent, &descent, &text_info);

    return text_info.width;
}

int GetTextHeight(Display *display, GC gc, const char *text, XFontStruct *font)
{
    int text_length = strlen(text);

    XSetFont(display, gc, font->fid);

    // Get text dimensions
    XCharStruct text_info;
    int direction;
    int ascent; /* baseline to top edge of raster */
    int descent; /* baseline to bottom edge of raster */
    XTextExtents(font, text, text_length, &direction, &ascent, &descent, &text_info);

    return ascent;
}

bool IsPointInRect(float point_x, float point_y, float rect_x, float rect_y, float rect_width, float rect_height)
{
    return (point_x >= rect_x && point_x <= rect_x + rect_width) && (point_y >= rect_y && point_y <= rect_y + rect_height);
}