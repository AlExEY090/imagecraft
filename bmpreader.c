
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bmpreader.h"
//1
// Загрузка BMP файла
struct BMPImage* readBMP(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", filename);
        return NULL;
    }

    struct BMPImage *img = malloc(sizeof(struct BMPImage));
    if (!img) {
        fclose(f);
        return NULL;
    }

    // Читаем заголовки
    if (fread(&img->fileHeader, sizeof(struct BMPFileHeader), 1, f) != 1) {
        fprintf(stderr, "Error: cannot read file header\n");
        fclose(f);
        free(img);
        return NULL;
    }

    if (fread(&img->infoHeader, sizeof(struct BMPInfoHeader), 1, f) != 1) {
        fprintf(stderr, "Error: cannot read info header\n");
        fclose(f);
        free(img);
        return NULL;
    }

    // Проверка сигнатуры BMP
    if (img->fileHeader.bfType != 0x4D42) { // 'BM'
        fprintf(stderr, "Error: not a BMP file (signature: 0x%04X)\n", img->fileHeader.bfType);
        fclose(f);
        free(img);
        return NULL;
    }

    // Проверка битности
    if (img->infoHeader.biBitCount != 24) {
        fprintf(stderr, "Error: only 24-bit BMP supported. This is %d-bit\n",
                img->infoHeader.biBitCount);
        fclose(f);
        free(img);
        return NULL;
    }

    int width = img->infoHeader.biWidth;
    int height = img->infoHeader.biHeight;

    // Обрабатываем отрицательную высоту
    int abs_height = height;
    if (height < 0) {
        abs_height = -height;
    }

    // Выделяем память для пикселей
    img->data = malloc(width * abs_height * sizeof(struct Pixel));
    if (!img->data) {
        fprintf(stderr, "Error: cannot allocate memory for image data\n");
        fclose(f);
        free(img);
        return NULL;
    }

    // Расчет паддинга (выравнивание строк по 4 байта)
    int row_size = width * 3;  // 3 байта на пиксель
    int padding = (4 - (row_size % 4)) % 4;

    // Переходим к началу данных пикселей
    fseek(f, img->fileHeader.bfOffBits, SEEK_SET);

    printf("Loading BMP: %dx%d, padding=%d, offset=%u\n",
           width, abs_height, padding, img->fileHeader.bfOffBits);

    // Чтение данных
    if (height > 0) {
        // Обычный BMP: строки идут снизу вверх
        for (int y = abs_height - 1; y >= 0; y--) {
            // Читаем строку пикселей
            for (int x = 0; x < width; x++) {
                struct Pixel pixel;
                if (fread(&pixel, sizeof(struct Pixel), 1, f) != 1) {
                    fprintf(stderr, "Error: cannot read pixel at (%d, %d)\n", x, y);
                    fclose(f);
                    free(img->data);
                    free(img);
                    return NULL;
                }
                img->data[y * width + x] = pixel;
            }
            // Пропускаем паддинг
            if (padding > 0) {
                fseek(f, padding, SEEK_CUR);
            }
        }
    } else {
        // Отрицательная высота: строки идут сверху вниз
        for (int y = 0; y < abs_height; y++) {
            for (int x = 0; x < width; x++) {
                struct Pixel pixel;
                if (fread(&pixel, sizeof(struct Pixel), 1, f) != 1) {
                    fprintf(stderr, "Error: cannot read pixel at (%d, %d)\n", x, y);
                    fclose(f);
                    free(img->data);
                    free(img);
                    return NULL;
                }
                img->data[y * width + x] = pixel;
            }
            // Пропускаем паддинг
            if (padding > 0) {
                fseek(f, padding, SEEK_CUR);
            }
        }
    }

    fclose(f);
    printf("Successfully loaded BMP file\n");
    return img;
}

// Алиас для совместимости
struct BMPImage* load_bmp(const char* filename) {
    return readBMP(filename);
}

// Сохранение BMP файла
int save_bmp(const char* filename, struct BMPImage* img) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: invalid image pointer\n");
        return -1;
    }

    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: cannot create file '%s'\n", filename);
        return -1;
    }

    int width = img->infoHeader.biWidth;
    int height = img->infoHeader.biHeight;
    int abs_height = (height < 0) ? -height : height;

    // Расчет паддинга
    int row_size = width * 3;
    int padding = (4 - (row_size % 4)) % 4;

    // Обновляем заголовки
    img->infoHeader.biSizeImage = (row_size + padding) * abs_height;
    img->fileHeader.bfSize = 54 + img->infoHeader.biSizeImage;
    img->fileHeader.bfOffBits = 54;

    printf("Saving BMP: %dx%d, padding=%d\n", width, abs_height, padding);

    // Записываем заголовки
    fwrite(&img->fileHeader, sizeof(struct BMPFileHeader), 1, f);
    fwrite(&img->infoHeader, sizeof(struct BMPInfoHeader), 1, f);

    // Записываем данные
    uint8_t pad_byte = 0;

    if (height > 0) {
        // Строки снизу вверх
        for (int y = abs_height - 1; y >= 0; y--) {
            for (int x = 0; x < width; x++) {
                struct Pixel pixel = img->data[y * width + x];
                fwrite(&pixel, sizeof(struct Pixel), 1, f);
            }
            // Записываем паддинг
            if (padding > 0) {
                fwrite(&pad_byte, 1, padding, f);
            }
        }
    } else {
        // Строки сверху вниз
        for (int y = 0; y < abs_height; y++) {
            for (int x = 0; x < width; x++) {
                struct Pixel pixel = img->data[y * width + x];
                fwrite(&pixel, sizeof(struct Pixel), 1, f);
            }
            // Записываем паддинг
            if (padding > 0) {
                fwrite(&pad_byte, 1, padding, f);
            }
        }
    }

    fclose(f);
    printf("Successfully saved BMP file\n");
    return 0;
}

// Проверка BMP
int validate_bmp(struct BMPImage* img) {
    if (!img) return 0;
    if (img->fileHeader.bfType != 0x4D42) return 0;
    if (img->infoHeader.biBitCount != 24) return 0;
    if (!img->data) return 0;
    return 1;
}

// Вывод информации
void print_bmp_info(struct BMPImage* img) {
    if (!img) {
        printf("Image: NULL\n");
        return;
    }

    printf("=== BMP Information ===\n");
    printf("Signature: %c%c (0x%04X)\n",
           (char)(img->fileHeader.bfType & 0xFF),
           (char)(img->fileHeader.bfType >> 8),
           img->fileHeader.bfType);
    printf("File size: %u bytes\n", img->fileHeader.bfSize);
    printf("Data offset: %u bytes\n", img->fileHeader.bfOffBits);
    printf("Width: %d pixels\n", img->infoHeader.biWidth);
    printf("Height: %d pixels\n", img->infoHeader.biHeight);
    printf("Bit depth: %d bits\n", img->infoHeader.biBitCount);
    printf("Compression: %u\n", img->infoHeader.biCompression);
    printf("Image size: %u bytes\n", img->infoHeader.biSizeImage);
    printf("=====================\n");
}

// Освобождение памяти
void free_bmp(struct BMPImage *img) {
    if (img) {
        if (img->data) {
            free(img->data);
        }
        free(img);
    }
}