#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "bmpreader.h"
#include "filter.h"
//1
/*
 gcc -o image_processor main.c filter.c bmpreader.c -lm
*/
typedef struct BMPImage* (*SpecialTransform)(struct BMPImage *img, void *params);


struct FilterNode {
    enum { PIXEL_TRANSFORM, SPECIAL_TRANSFORM } type;
    union {
        PixelTransform pixel_transform;
        SpecialTransform special_transform;
    } transform;
    ParamsDestructor destructor;
    void *params;
    struct FilterNode *next;
};

void add_pixel_filter(struct FilterNode **head,
                     PixelTransform transform,
                     ParamsDestructor destructor,
                     void *params) {
    struct FilterNode *node = malloc(sizeof(struct FilterNode));
    node->type = PIXEL_TRANSFORM;
    node->transform.pixel_transform = transform;
    node->destructor = destructor;
    node->params = params;
    node->next = NULL;

    if (*head == NULL) {
        *head = node;
    } else {
        struct FilterNode *temp = *head;
        while (temp->next) temp = temp->next;
        temp->next = node;
    }
}

void add_special_filter(struct FilterNode **head,
                       SpecialTransform transform,
                       ParamsDestructor destructor,
                       void *params) {
    struct FilterNode *node = malloc(sizeof(struct FilterNode));
    node->type = SPECIAL_TRANSFORM;
    node->transform.special_transform = transform;
    node->destructor = destructor;
    node->params = params;
    node->next = NULL;

    if (*head == NULL) {
        *head = node;
    } else {
        struct FilterNode *temp = *head;
        while (temp->next) temp = temp->next;
        temp->next = node;
    }
}

void apply_filter_chain(struct BMPImage **img, struct FilterNode *head) {
    struct FilterNode *current = head;
    while (current) {
        if (current->type == SPECIAL_TRANSFORM) {
            // Специальные фильтры (crop)
            struct BMPImage *new_img = current->transform.special_transform(*img, current->params);
            if (*img != new_img) {
                free((*img)->data);
                free(*img);
                *img = new_img;
            }
        } else {
            // Обычные пиксельные трансформеры
            apply_transform(*img, current->transform.pixel_transform, current->params);
        }
        current = current->next;
    }
}

void destroy_filter_chain(struct FilterNode *head) {
    struct FilterNode *current = head;
    while (current) {
        struct FilterNode *next = current->next;

        if (current->destructor && current->params) {
            current->destructor(current->params);
        }

        free(current);
        current = next;
    }
}

struct FilterNode *parse_arguments(int argc, char **argv, int img_width, int img_height) {
    struct FilterNode *head = NULL;

    for (int i = 3; i < argc; i++) {

        if (strcmp(argv[i], "-gs") == 0) {
            struct formulaFilter *f = malloc(sizeof(struct formulaFilter));
            f->coef[0] = 0.299f;
            f->coef[1] = 0.587f;
            f->coef[2] = 0.114f;
            add_pixel_filter(&head, formula_transform, destroy_formula_filter, f);
        }

        else if (strcmp(argv[i], "-blur") == 0 && i + 1 < argc) {
            float sigma = atof(argv[++i]);
            if (sigma <= 0) sigma = 1.0f;
            struct matrixFilter *kernel = create_gauss_kernel(2, sigma);
            add_pixel_filter(&head, matrix_transform, destroy_matrix_filter, kernel);
        }

        else if (strcmp(argv[i], "-med") == 0 && i + 1 < argc) {
            struct MedianParams *p = malloc(sizeof(struct MedianParams));
            p->window_size = atoi(argv[++i]);
            if (p->window_size % 2 == 0) p->window_size++;
            if (p->window_size < 3) p->window_size = 3;
            add_pixel_filter(&head, transformer_median, destroy_median_params, p);
        }

        else if (strcmp(argv[i], "-vortex") == 0 && i + 2 < argc) {
            struct vortex *p = malloc(sizeof(struct vortex));
            p->angle = atof(argv[++i]);
            p->radius = atof(argv[++i]);
            if (p->radius <= 0) p->radius = 100.0f;
            add_pixel_filter(&head, transformer_vortex, destroy_vortex_params, p);
        }

        else if (strcmp(argv[i], "-neg") == 0) {
            struct formulaFilter *f = malloc(sizeof(struct formulaFilter));
            f->coef[0] = 255;
            f->coef[1] = 255;
            f->coef[2] = 255;
            add_pixel_filter(&head, shift_transform, destroy_formula_filter, f);
        }

        else if (strcmp(argv[i], "-crystallize") == 0 && i + 1 < argc) {
            int count = atoi(argv[++i]);
            if (count > 0) {
                struct CrystalParams *p = malloc(sizeof(struct CrystalParams));
                p->points_count = count;
                p->coords_x = malloc(count * sizeof(int));
                p->coords_y = malloc(count * sizeof(int));

                if (i + 2 * count < argc) {
                    for (int j = 0; j < count; j++) {
                        p->coords_x[j] = atoi(argv[++i]);
                        p->coords_y[j] = atoi(argv[++i]);
                    }
                } else {
                    for (int j = 0; j < count; j++) {
                        p->coords_x[j] = rand() % img_width;
                        p->coords_y[j] = rand() % img_height;
                    }
                }
                add_pixel_filter(&head, transformer_crystallize, destroy_crystal_params, p);
            }
        }

        else if (strcmp(argv[i], "-sharp") == 0) {
            struct matrixFilter *p = malloc(sizeof(struct matrixFilter));
            p->size = 3;
            p->matrix = malloc(9 * sizeof(float));
            float sharp_kernel[9] = {0.0f, -1.0f, 0.0f,
                                     -1.0f, 5.0f, -1.0f,
                                      0.0f, -1.0f, 0.0f};
            memcpy(p->matrix, sharp_kernel, 9 * sizeof(float));
            add_pixel_filter(&head, matrix_transform, destroy_matrix_filter, p);
        }

        else if (strcmp(argv[i], "-edge") == 0 && i + 1 < argc) {
            double t = atof(argv[++i]);


            struct formulaFilter *f = malloc(sizeof(struct formulaFilter));
            f->coef[0] = 0.299f;
            f->coef[1] = 0.587f;
            f->coef[2] = 0.114f;
            add_pixel_filter(&head, formula_transform, destroy_formula_filter, f);

            struct matrixFilter *p = malloc(sizeof(struct matrixFilter));
            p->size = 3;
            p->matrix = malloc(9 * sizeof(float));
            float edge_kernel[9] = { 0.0f, -1.0f,  0.0f,
                                     -1.0f, 4.0f, -1.0f,
                                     0.0f,  -1.0f, 0.0f};
            memcpy(p->matrix, edge_kernel, 9 * sizeof(float));
            add_pixel_filter(&head, matrix_transform, destroy_matrix_filter, p);

            struct EdgeDetectParams *e = malloc(sizeof(struct EdgeDetectParams));
            e->threshold = t*255;
            add_pixel_filter(&head, threshold_transform, destroy_edge_params, e);
        }

        else if (strcmp(argv[i], "-crop") == 0 && i + 2 < argc) {
            struct CropParams *p = malloc(sizeof(struct CropParams));
            p->new_width = atoi(argv[++i]);
            p->new_height = atoi(argv[++i]);
            if (p->new_width <= 0) p->new_width = 100;
            if (p->new_height <= 0) p->new_height = 100;
            add_special_filter(&head, crop_image, destroy_crop_params, p);
        }

        else {
            fprintf(stderr, "unknown argument- '%s'\n", argv[i]);
        }
    }

    return head;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s input.bmp output.bmp [filters]\n", argv[0]);
        printf("\nFilters:\n");
        printf("  -gs                    - grayscale\n");
        printf("  -blur <sigma>          - Gaussian blur\n");
        printf("  -med <size>            - median filter\n");
        printf("  -vortex <angle> <radius> - vortex effect\n");
        printf("  -neg                   - negative\n");
        printf("  -crystallize <count> [x1 y1 ...] - crystallize\n");
        printf("  -sharp                 - sharpen\n");
        printf("  -edge <threshold>      - edge detection (0-255)\n");
        printf("  -crop <width> <height> - crop image\n");
        return 1;
    }

    char *input_file = argv[1];
    char *output_file = argv[2];

    printf("Processing image:\n");
    printf("  Input:  %s\n", input_file);
    printf("  Output: %s\n", output_file);

    struct BMPImage *img = load_bmp(input_file);
    if (!img) {
        fprintf(stderr, "Error: could not load file '%s'\n", input_file);
        return 1;
    }

    srand((unsigned int)time(NULL));

    int img_width = img->infoHeader.biWidth;
    int img_height = abs(img->infoHeader.biHeight);

    printf("  Size: %d x %d pixels\n", img_width, img_height);

    struct FilterNode *filters = parse_arguments(argc, argv, img_width, img_height);

    if (filters) {
        printf("  Applying filters...\n");
        apply_filter_chain(&img, filters);
        printf("  New size: %d x %d pixels\n",
               img->infoHeader.biWidth,
               abs(img->infoHeader.biHeight));
    } else if (argc > 3) {
        printf("  Warning: arguments specified but no filters recognized\n");
    }

    if (save_bmp(output_file, img) != 0) {
        fprintf(stderr, "Error: could not save file '%s'\n", output_file);
        if (filters) destroy_filter_chain(filters);
        free_bmp(img);
        return 1;
    }

    printf("  Done! Image successfully saved.\n");

    if (filters) destroy_filter_chain(filters);
    free_bmp(img);

    return 0;
}
