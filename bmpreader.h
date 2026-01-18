//
// Created by 1alex on 05.01.2026.
//

#ifndef LABIP_BMPREADER_H
#define LABIP_BMPREADER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#pragma pack(push,1)
//1
struct BMPFileHeader{
    uint16_t bfType;      // Отметка "BM" (0x4D42)
    uint32_t bfSize;
    uint16_t bfReserved1; // (0)
    uint16_t bfReserved2; //(0)
    uint32_t bfOffBits;
};

struct BMPInfoHeader{
    uint32_t biSize;          // Размер структуры (40)
    int32_t  biWidth;         // Ширина в пикселях
    int32_t  biHeight;        // Высота в пикселях
    uint16_t biPlanes;        // Количество плоскостей (1)
    uint16_t biBitCount;      // Глубина цвета (24)
    uint32_t biCompression;   // Сжатие (0 для BI_RGB)
    uint32_t biSizeImage;     // Размер данных (может быть 0 для BI_RGB)
    int32_t  biXPelsPerMeter; // Разрешение по X
    int32_t  biYPelsPerMeter; // Разрешение по Y
    uint32_t biClrUsed;       // Цветов в палитре (0)
    uint32_t biClrImportant;  // Важных цветов (0)
};

struct Pixel{
    uint8_t b;
    uint8_t g;
    uint8_t r;
};

struct BMPImage {
    struct BMPFileHeader fileHeader;
    struct BMPInfoHeader infoHeader;
    struct Pixel *data;
};
#pragma pack(pop)

// Основные функции для работы с BMP
struct BMPImage* load_bmp(const char* filename);           // Алиас для readBMP
struct BMPImage* readBMP(const char* filename);            // Загрузка BMP
int save_bmp(const char* filename, struct BMPImage* img);  // Сохранение BMP
void free_bmp(struct BMPImage *img);                       // Освобождение памяти

// Вспомогательные функции
int validate_bmp(struct BMPImage* img);                    // Проверка корректности BMP
void print_bmp_info(struct BMPImage* img);                 // Вывод информации о BMP

#endif //LABIP_BMPREADER_H