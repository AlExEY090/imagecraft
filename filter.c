
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "bmpreader.h"
#include "filter.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
//1
struct Pixel checkPixel(struct BMPImage *img, int x, int y) {
    int w = img->infoHeader.biWidth;
    int h = abs(img->infoHeader.biHeight);
    if (x < 0) x = 0;
    if (x >= w) x = w - 1;
    if (y < 0) y = 0;
    if (y >= h) y = h - 1;
    return img->data[y * w + x];
}

void apply_transform(struct BMPImage *img, PixelTransform transform, void* params) {
    int w = img->infoHeader.biWidth;
    int h = abs(img->infoHeader.biHeight);
    struct Pixel *new = malloc(w * h * sizeof(struct Pixel));
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            new[y * w + x] = transform(x, y, img, params);
        }
    }
    free(img->data);
    img->data = new;
}

struct Pixel formula_transform(int x, int y, struct BMPImage *img, void * params) {
    struct formulaFilter *filter = (struct formulaFilter *)params;
    struct Pixel p = img->data[y * img->infoHeader.biWidth + x];
    float res = filter->coef[0] * p.r +
                filter->coef[1] * p.g +
                filter->coef[2] * p.b;
    if (res < 0) res = 0;
    if (res > 255) res = 255;
    uint8_t gray = (uint8_t)res;
    return (struct Pixel){gray, gray, gray};
}

struct Pixel matrix_transform(int x, int y, struct BMPImage *img, void * params) {
    struct matrixFilter *filter = (struct matrixFilter *)params;
    float newR = 0, newG = 0, newB = 0;
    int offset = filter->size / 2;
    for (int i = 0; i < filter->size; i++) {
        for (int j = 0; j < filter->size; j++) {
            struct Pixel p = checkPixel(img, x + (j - offset), y + (i - offset));
            float weight = filter->matrix[i * filter->size + j];
            newR += p.r * weight;
            newG += p.g * weight;
            newB += p.b * weight;
        }
    }

    newR = fmaxf(0, fminf(255, newR));
    newG = fmaxf(0, fminf(255, newG));
    newB = fmaxf(0, fminf(255, newB));

    return (struct Pixel){(uint8_t)newB, (uint8_t)newG, (uint8_t)newR};
}

struct matrixFilter * create_gauss_kernel(int radius, float sigma) {
    int size = 2 * radius + 1;
    struct matrixFilter *p = malloc(sizeof(struct matrixFilter));
    p->size = size;
    p->matrix = malloc(size * size * sizeof(float));
    float sum = 0.0f;
    int offset = size / 2;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int dx = x - offset;
            int dy = y - offset;
            float val = (1.0f / (2.0f * (float)M_PI * sigma * sigma)) *
                         expf(-(float)(dx*dx + dy*dy) / (2.0f * sigma * sigma));
            p->matrix[y * size + x] = val;
            sum += val;
        }
    }

    if (sum > 0) {
        for (int i = 0; i < size * size; i++) {
            p->matrix[i] /= sum;
        }
    }

    return p;
}

struct Pixel transformer_vortex(int x, int y, struct BMPImage *img, void *params) {
    struct vortex *v = (struct vortex *)params;
    int w = img->infoHeader.biWidth;
    int h = abs(img->infoHeader.biHeight);
    float cx = w / 2.0f;
    float cy = h / 2.0f;
    float dx = x - cx;
    float dy = y - cy;
    float r = hypotf(dx, dy);
    if (r < v->radius) {
        float current_angle = atan2f(dy, dx);
        float twist = v->angle * (v->radius - r) / v->radius;
        float new_angle = current_angle + twist;
        float src_x = cx + r * cosf(new_angle);
        float src_y = cy + r * sinf(new_angle);
        return checkPixel(img, (int)src_x, (int)src_y);
    }
    return img->data[y * w + x];
}

struct Pixel transformer_crystallize(int x, int y, struct BMPImage *img, void *params) {
    struct CrystalParams *p = (struct CrystalParams *)params;

    if (p->points_count == 0) {
        return img->data[y * img->infoHeader.biWidth + x];
    }

    int nearest_idx = 0;
    float min_dist = 1e10f;

    for (int i = 0; i < p->points_count; i++) {
        float dx = (float)(x - p->coords_x[i]);
        float dy = (float)(y - p->coords_y[i]);
        float dist_sq = dx * dx + dy * dy;

        if (dist_sq < min_dist) {
            min_dist = dist_sq;
            nearest_idx = i;
        }
    }

    int cx = p->coords_x[nearest_idx];
    int cy = p->coords_y[nearest_idx];

    int w = img->infoHeader.biWidth;
    int h = abs(img->infoHeader.biHeight);
    if (cx < 0) cx = 0;
    if (cx >= w) cx = w - 1;
    if (cy < 0) cy = 0;
    if (cy >= h) cy = h - 1;

    return img->data[cy * w + cx];
}

struct BMPImage* crop_image(struct BMPImage *src, void *params) {
    struct CropParams *p = (struct CropParams *)params;
    int32_t src_w = src->infoHeader.biWidth;
    int32_t src_h = abs(src->infoHeader.biHeight);

    if (p->new_width <= 0 || p->new_height <= 0) {
        fprintf(stderr, "Ошибка: неверные размеры для crop\n");
        return src;
    }

    if (p->new_width > src_w) p->new_width = src_w;
    if (p->new_height > src_h) p->new_height = src_h;

    struct BMPImage *new_img = malloc(sizeof(struct BMPImage));
    memcpy(&new_img->fileHeader, &src->fileHeader, sizeof(struct BMPFileHeader));
    memcpy(&new_img->infoHeader, &src->infoHeader, sizeof(struct BMPInfoHeader));

    new_img->infoHeader.biWidth = p->new_width;
    new_img->infoHeader.biHeight = (src->infoHeader.biHeight < 0) ? -p->new_height : p->new_height;
    new_img->data = malloc(p->new_width * p->new_height * sizeof(struct Pixel));

    for (int y = 0; y < p->new_height; y++) {
        for (int x = 0; x < p->new_width; x++) {
            new_img->data[y * p->new_width + x] = src->data[y * src_w + x];
        }
    }

    int padding = (4 - (p->new_width * 3) % 4) % 4;
    new_img->infoHeader.biSizeImage = (p->new_width * 3 + padding) * p->new_height;
    new_img->fileHeader.bfSize = 54 + new_img->infoHeader.biSizeImage;

    return new_img;
}

int compare_uint8(const void *a, const void *b) {
    return (*(uint8_t*)a - *(uint8_t*)b);
}

struct Pixel transformer_median(int x, int y, struct BMPImage *img, void *params) {
    struct MedianParams *p = (struct MedianParams *)params;
    int size = p->window_size;
    int offset = size / 2;
    int total_pixels = size * size;

    if (size <= 0 || size % 2 == 0) {
        return img->data[y * img->infoHeader.biWidth + x];
    }

    uint8_t *r_values = malloc(total_pixels * sizeof(uint8_t));
    uint8_t *g_values = malloc(total_pixels * sizeof(uint8_t));
    uint8_t *b_values = malloc(total_pixels * sizeof(uint8_t));

    int count = 0;
    for (int ky = 0; ky < size; ky++) {
        for (int kx = 0; kx < size; kx++) {
            struct Pixel pix = checkPixel(img, x + (kx - offset), y + (ky - offset));
            r_values[count] = pix.r;
            g_values[count] = pix.g;
            b_values[count] = pix.b;
            count++;
        }
    }

    qsort(r_values, total_pixels, sizeof(uint8_t), compare_uint8);
    qsort(g_values, total_pixels, sizeof(uint8_t), compare_uint8);
    qsort(b_values, total_pixels, sizeof(uint8_t), compare_uint8);

    struct Pixel result = {
        b_values[total_pixels / 2],
        g_values[total_pixels / 2],
        r_values[total_pixels / 2]
    };

    free(r_values); free(g_values); free(b_values);
    return result;
}

struct Pixel shift_transform(int x, int y, struct BMPImage *img, void *params) {
    struct formulaFilter *f = (struct formulaFilter *)params;
    struct Pixel p = img->data[y * img->infoHeader.biWidth + x];

    uint8_t newR = (uint8_t)(f->coef[0] - p.r);
    uint8_t newG = (uint8_t)(f->coef[1] - p.g);
    uint8_t newB = (uint8_t)(f->coef[2] - p.b);

    return (struct Pixel){newB, newG, newR};
}

struct Pixel threshold_transform(int x, int y, struct BMPImage *img, void *params) {
    struct EdgeDetectParams *filter = (struct EdgeDetectParams *)params;
    struct Pixel p = img->data[y * img->infoHeader.biWidth + x];

    // Изображение уже в grayscale, берем любой канал
    uint8_t gray = p.r;
    uint8_t color = (gray > filter->threshold) ? 255 : 0;

    return (struct Pixel){color, color, color};
}

// ===== ДЕСТРУКТОРЫ =====

void destroy_matrix_filter(void *ptr) {
    struct matrixFilter *f = (struct matrixFilter *)ptr;
    if (f) {
        if (f->matrix) free(f->matrix);
        free(f);
    }
}

void destroy_formula_filter(void *ptr) {
    struct formulaFilter *f = (struct formulaFilter *)ptr;
    if (f) free(f);
}

void destroy_vortex_params(void *ptr) {
    struct vortex *v = (struct vortex *)ptr;
    if (v) free(v);
}

void destroy_crystal_params(void *ptr) {
    struct CrystalParams *p = (struct CrystalParams *)ptr;
    if (p) {
        if (p->coords_x) free(p->coords_x);
        if (p->coords_y) free(p->coords_y);
        free(p);
    }
}

void destroy_crop_params(void *ptr) {
    struct CropParams *p = (struct CropParams *)ptr;
    if (p) free(p);
}

void destroy_median_params(void *ptr) {
    struct MedianParams *p = (struct MedianParams *)ptr;
    if (p) free(p);
}

void destroy_edge_params(void *ptr) {
    struct EdgeDetectParams *e = (struct EdgeDetectParams *)ptr;
    if (e) free(e);
}