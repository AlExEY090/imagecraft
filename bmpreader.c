#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bmpreader.h"
struct BMPImage* readBMP(const char* filename) {
    FILE *f =fopen(filename,"rb");
    if (!f) return NULL;
    struct BMPImage * img=malloc(sizeof(struct BMPImage));
    fread(&(img->fileHeader),sizeof(struct BMPFileHeader),1,f);
    fread(&(img->infoHeader),sizeof(struct BMPInfoHeader),1,f);
    int width = img->infoHeader.biWidth;
    int height = img->infoHeader.biHeight;
    img->data = malloc(width * height * sizeof(struct Pixel));
    int padding =(4-(3*width)%4)%4;
    fseek(f, (long) img->fileHeader.bfOffBits, SEEK_SET);
    for (int i=0;i<height;i++) {
        fread(&(img->data[i*width]),sizeof(struct Pixel),1,f);
        fseek(f,padding,SEEK_CUR);
    }
    fclose(f);
    return img;
}

void free_bmp(struct BMPImage *img) {
    if (img) {
        free(img->data);
        free(img);
    }
}

