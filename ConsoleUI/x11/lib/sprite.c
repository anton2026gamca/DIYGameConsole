#include "lib4x11.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <png.h>
#include <stdlib.h>

// Declare a jump buffer to handle PNG read errors
jmp_buf png_jmpbuf;

void LoadPNG(const char *filename, unsigned char **image_data, int *width, int *height, int *channels)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error opening PNG file %s\n", filename);
        exit(1);
    }

    // Initialize png structures
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        fprintf(stderr, "Error creating PNG read structure\n");
        exit(1);
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        fprintf(stderr, "Error creating PNG info structure\n");
        exit(1);
    }

    if (setjmp(png_jmpbuf)) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        fprintf(stderr, "Error during PNG read\n");
        exit(1);
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    *channels = png_get_channels(png, info);

    png_byte bit_depth = png_get_bit_depth(png, info);
    png_byte color_type = png_get_color_type(png, info);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (bit_depth == 16)
        png_set_strip_16(png);

    png_read_update_info(png, info);

    // Allocate memory for the image data
    *image_data = (unsigned char *)malloc((*width) * (*height) * (*channels));

    // Read the image data into the buffer
    png_bytep rows[*height];
    for (int i = 0; i < *height; i++) {
        rows[i] = *image_data + i * (*width) * (*channels);
    }
    png_read_image(png, rows);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
}

void DrawImage(Display *display, Drawable buffer, GC gc, XImage *ximage, int dest_x, int dest_y)
{
    int img_width = ximage->width;
    int img_height = ximage->height;
    unsigned char *img_data = (unsigned char *)ximage->data;

    // Get the dimensions of the drawable buffer (window or pixmap)
    Window root;
    int x, y;
    unsigned int buffer_width, buffer_height, border_width, depth;
    if (!XGetGeometry(display, buffer, &root, &x, &y, &buffer_width, &buffer_height, &border_width, &depth)) {
        fprintf(stderr, "Failed to get buffer geometry\n");
        return;
    }

    // Adjust the starting positions if negative
    int draw_x = (dest_x < 0) ? 0 : dest_x;
    int draw_y = (dest_y < 0) ? 0 : dest_y;

    // Calculate the drawable region considering possible cropping
    int draw_width = (dest_x + img_width > buffer_width) ? buffer_width - dest_x : img_width;
    if (dest_x < 0) draw_width += dest_x;
    int draw_height = (dest_y + img_height > buffer_height) ? buffer_height - dest_y : img_height;
    if (dest_y < 0) draw_height += dest_y;

    // If the image is completely outside the drawable area, return without drawing
    if (draw_x >= buffer_width || draw_y >= buffer_height || draw_width <= 0 || draw_height <= 0) {
        return;
    }

    // Get the background region in bulk
    XImage *bg_image = XGetImage(display, buffer, draw_x, draw_y, draw_width, draw_height, AllPlanes, ZPixmap);
    if (!bg_image) {
        fprintf(stderr, "Failed to get background image\n");
        return;
    }

    // Loop through each pixel in the drawable region and perform alpha blending
    for (int y = 0; y < draw_height; y++) {
        for (int x = 0; x < draw_width; x++) {
            int index = ((y + (draw_y - dest_y)) * img_width + (x + (draw_x - dest_x))) * 4; // Adjust for negative offsets

            // Extract RGBA components from the source (PNG) pixel
            unsigned char r = img_data[index];
            unsigned char g = img_data[index + 1];
            unsigned char b = img_data[index + 2];
            unsigned char a = img_data[index + 3];

            // Skip fully transparent pixels
            if (a == 0) continue;

            if (a == 255) {
                // Combine RGB into a single pixel value
                unsigned long pixel = 0xFF000000 | (r << 16) | (g << 8) | b;

                // Set the pixel in the background image
                XPutPixel(bg_image, x, y, pixel);
            } else {
                // Get the background pixel
                unsigned long bg_pixel = XGetPixel(bg_image, x, y);
                unsigned char bg_r = (bg_pixel & 0xFF0000) >> 16;
                unsigned char bg_g = (bg_pixel & 0x00FF00) >> 8;
                unsigned char bg_b = (bg_pixel & 0x0000FF);

                // Perform alpha blending: result = (src * alpha + dst * (255 - alpha)) / 255
                unsigned char blended_r = (r * a + bg_r * (255 - a)) / 255;
                unsigned char blended_g = (g * a + bg_g * (255 - a)) / 255;
                unsigned char blended_b = (b * a + bg_b * (255 - a)) / 255;

                // Combine blended RGB into a single pixel value
                unsigned long blended_pixel = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;

                // Set the pixel in the background image
                XPutPixel(bg_image, x, y, blended_pixel);
            }
        }
    }

    // Put the modified background image back to the buffer
    XPutImage(display, buffer, gc, bg_image, 0, 0, draw_x, draw_y, draw_width, draw_height);

    // Free the background image
    XDestroyImage(bg_image);
}

Sprite LoadSprite(Display *display, XVisualInfo vinfo, const char *filename, int x, int y)
{
    Sprite sprite;
    sprite.img_data = NULL;
    sprite.x = x;
    sprite.y = y;
    LoadPNG(filename, &sprite.img_data, &sprite.width, &sprite.height, &sprite.channels);
    sprite.ximage = XCreateImage(display, vinfo.visual, vinfo.depth, ZPixmap, 0, (char *)sprite.img_data, sprite.width, sprite.height, 32, 0);

    return sprite;
}

void DrawSprite(Display *display, Drawable buffer, GC gc, Sprite *sprite)
{
    DrawImage(display, buffer, gc, sprite->ximage, sprite->x, sprite->y);
}