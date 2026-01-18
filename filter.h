#ifndef LABIP_FILTER_H
#define LABIP_FILTER_H

#include "bmpreader.h"
#include <time.h>
//1
struct matrixFilter {
    int size;
    float *matrix;
};

struct formulaFilter {
    float coef[3];  // coef[0]-r, coef[1]-g, coef[2]-b
};

typedef struct Pixel (*PixelTransform)(int x, int y, struct BMPImage *img, void *params);
typedef void (*ParamsDestructor)(void *params);

struct vortex {
    float angle;
    float radius;
};

struct CrystalParams {
    int points_count;
    int *coords_x;
    int *coords_y;
};

struct CropParams {
    int32_t new_width;
    int32_t new_height;
};

struct MedianParams {
    int window_size;
};

struct EdgeDetectParams {
    int threshold;
};
#if 0
struct FilterNode {
    PixelTransform transform;
    ParamsDestructor destructor;
    void *params;
    struct FilterNode *next;
};
#endif
// Вспомогательные функции
struct Pixel checkPixel(struct BMPImage *img, int x, int y);

// Деструкторы с правильной сигнатурой
void destroy_matrix_filter(void *f);
void destroy_formula_filter(void *f);
void destroy_vortex_params(void *v);
void destroy_crystal_params(void *p);
void destroy_crop_params(void *p);
void destroy_median_params(void *p);
void destroy_edge_params(void *e);

// Основные функции
void apply_transform(struct BMPImage *img, PixelTransform transform, void* params);
struct Pixel formula_transform(int x, int y, struct BMPImage *img, void *params);
struct Pixel matrix_transform(int x, int y, struct BMPImage *img, void *params);
struct matrixFilter *create_gauss_kernel(int radius, float sigma);
struct Pixel transformer_vortex(int x, int y, struct BMPImage *img, void *params);
struct Pixel transformer_crystallize(int x, int y, struct BMPImage *img, void *params);
struct BMPImage* crop_image(struct BMPImage *src, void *params);
struct Pixel transformer_median(int x, int y, struct BMPImage *img, void *params);
struct Pixel shift_transform(int x, int y, struct BMPImage *img, void *params);
struct Pixel threshold_transform(int x, int y, struct BMPImage *img, void *params);

#endif // LABIP_FILTER_H