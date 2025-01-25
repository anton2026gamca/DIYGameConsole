#include "lib4x11.h"
#include <X11/Xlib.h>
#include <stdbool.h>

void DrawButton(Display *display, Drawable buffer, GC gc, Button btn, int mouse_x, int mouse_y, bool mouse_pressed)
{
    if (IsPointInRect(mouse_x, mouse_y, btn.x, btn.y, btn.width, btn.height) && mouse_pressed)
        DrawRectangle(display, buffer, gc, btn.x, btn.y, btn.width, btn.height, btn.pressed_color);
    else if (IsPointInRect(mouse_x, mouse_y, btn.x, btn.y, btn.width, btn.height))
        DrawRectangle(display, buffer, gc, btn.x, btn.y, btn.width, btn.height, btn.hover_color);
    else
        DrawRectangle(display, buffer, gc, btn.x, btn.y, btn.width, btn.height, btn.normal_color);
    if (btn.outline_color > 0)
        DrawRectangleOutline(display, buffer, gc, btn.x, btn.y, btn.width, btn.height, btn.outline_color);
    DrawTextCentered(display, buffer, gc, btn.x + btn.width / 2, btn.y + btn.height / 2, btn.text, btn.text_color, btn.font);
}

bool IsButtonPressed(Button btn, int mouse_x, int mouse_y, bool mouse_pressed)
{
    if (mouse_pressed && IsPointInRect(mouse_x, mouse_y, btn.x, btn.y, btn.width, btn.height))
        return true;
    return false;
}