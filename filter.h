//
// Created by 1alex on 07.01.2026.
//

#ifndef LABIP_FILTER_H
#define LABIP_FILTER_H
struct matrixFilter {
    int size;
    float *matrix;
};
struct formulaFilter {
    float *coef;//coef[0]-r,coef[2]-b
};
typedef struct Pixel (*PixelTransform)(int x, int y, struct BMPImage *img, void *params);
struct vortex {
    float angle;
    float radius;
};
struct CrystalParams{
    int points_count;
    int *coords_x;
    int *coords_y;
};
struct CropParams{
    int32_t new_width;
    int32_t new_height;
};
struct MedianParams{
    int window_size; // Обычно 3 (для окна 3x3) или 5
};

#define createFilter(structType,exName,fieldName,value)\
structType * exName =(structType*)malloc(sizeof(structType));\
exName->fieldName=value;
#define setData_memcpy(exNameP, fieldName, type, values_array, count)\
{\
size_t num_elements = (size_t)count;\
size_t size_bytes = num_elements * sizeof(type); \
memcpy(exNameP->fieldName, values_array, size_bytes);\
}

void apply_transform(struct BMPImage *img,PixelTransform transform,void* params);
struct Pixel formula_transform(int x, int y, struct BMPImage *img, void * params);
struct Pixel matrix_transform(int x, int y, struct BMPImage *img, void * params);

#endif //LABIP_FILTER_H