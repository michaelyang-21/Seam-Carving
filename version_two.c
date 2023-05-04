#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "c_img.h"
#include "c_img.c"

// The function will compute the dual-gradient energy function
void calc_energy(struct rgb_img *im, struct rgb_img **grad)
{
    *grad = malloc(sizeof(struct rgb_img));
    (*grad)->width = im->width;
    (*grad)->height = im->height;
    (*grad)->raster = malloc(sizeof(uint8_t) * 3 * im->width * im->height);

    int x, y;
    int r1, g1, b1;
    for (y = 0; y < im->height; y++)
    {
        for (x = 0; x < im->width; x++)
        {
            r1 = get_pixel(im, y, x, 0);
            g1 = get_pixel(im, y, x, 1);
            b1 = get_pixel(im, y, x, 2);

            // Compute the energy of the pixel
            // subtract the cell after it from the cell before it
            // % helps it wrap around
            int r_x = get_pixel(im, y, (x+1)%(im->width), 0) - get_pixel(im, y, (x+im->width-1)%(im->width), 0);
            int g_x = get_pixel(im, y, (x+1)%(im->width), 1) - get_pixel(im, y, (x+im->width-1)%(im->width), 1);
            int b_x = get_pixel(im, y, (x+1)%(im->width), 2) - get_pixel(im, y, (x+im->width-1)%(im->width), 2);

            int r_y = get_pixel(im, (y+1)%(im->height), x, 0) - get_pixel(im, (y+im->height-1)%(im->height), x, 0);
            int g_y = get_pixel(im, (y+1)%(im->height), x, 1) - get_pixel(im, (y+im->height-1)%(im->height), x, 1);
            int b_y = get_pixel(im, (y+1)%(im->height), x, 2) - get_pixel(im, (y+im->height-1)%(im->height), x, 2);

            int x_delta = r_x*r_x + g_x*g_x + b_x*b_x;
            int y_delta = r_y*r_y + g_y*g_y + b_y*b_y;

            double dual_gradient = sqrt(x_delta + y_delta);

            // save energy value to the gradient image
            uint8_t save_energy = (uint8_t) (dual_gradient / 10);
            set_pixel(*grad, y, x, save_energy, save_energy, save_energy);
        }
    }
}

// computes the *best_arr seam using dynamic programming
// allocate memory for *best_arr
void dynamic_seam(struct rgb_img *grad, double **best_arr)
{
    *best_arr = malloc(sizeof(double) * (grad->width*grad->height));
    for (int i = 0; i < grad->width*grad->height; i++)
    {
        (*best_arr)[i] = 0;
    }

    // initialize the first row of the best_arr
    for (int i = 0; i < grad->width; i++)
    {
        (*best_arr)[i] = get_pixel(grad, 0, i, 0);
    }

    // compute the best_arr
    for (int i = 1; i < grad->height; i++)
    {
        for (int j = 0; j < grad->width; j++)
        {
            // compute the minimum of the three adjacent pixels
            double minimum = (*best_arr)[(i-1)*grad->width + j];

            // check if the pixel is on the left edge
            if (j > 0)
            {
                if ((*best_arr)[(i-1)*grad->width + j-1] < minimum)
                {
                    minimum = (*best_arr)[(i-1)*grad->width + j-1];
                }
            }

            // check if the pixel is on the right edge
            if (j < grad->width-1)
            {
                if ((*best_arr)[(i-1)*grad->width + j+1] < minimum)
                {
                    minimum = (*best_arr)[(i-1)*grad->width + j+1];
                }
            }
            // add the energy of the current pixel to the minimum
            (*best_arr)[i*grad->width + j] = minimum + get_pixel(grad, i, j, 0);
        }
    }

    // printing the best_arr
    // testing print
    // for (int i = 0; i < grad->height; i++)
    // {
    //     for (int j = 0; j < grad->width; j++)
    //     {
    //         printf("%f ", (*best_arr)[i*grad->width + j]);
    //     }
    //     printf("\n");
    // }
    
}

void recover_path(double *best, int height, int width, int **path)
{
    // allocate memory for the path list array
    *path = malloc(sizeof(int)*height);

    double total_energy = INFINITY; 

    // find the start point on the last row
    int start = 0;
    for (int i = 1; i < width; i++)
    {
        if (best[(height-1)*width + i] < total_energy)
        {
            total_energy = best[(height-1)*width + i];
            start = i;
        }
    }

    // save the start point to the path list
    (*path)[height-1] = start;

    // recover the path
    for (int i = height-2; i >= 0; i--)
    {
        // dummy variable to store location of minimum energy
        int index = start;

        int min = best[i*width + index];

        // check if the pixel is on the left edge
        if (index > 0)
        {
            if (best[i*width + index-1] < best[i*width + index])
            {
                min = best[i*width + index-1];
                start = index - 1;
            }
        }

        // check if the pixel is on the right edge
        if (index < width-1)
        {
            if (best[i*width + index+1] < min)
            {
                start = index + 1;
            }
        }
        // save the start point to the path list
        (*path)[i] = start;
    }
    
    // print path
    // for (int i = 0; i < height; i++)
    // {
    //     printf("%d ", (*path)[i]);
    // }

}

void remove_seam(struct rgb_img *src, struct rgb_img **dest, int *path)
{
    // allocate memory for the destination image
    *dest = malloc(sizeof(struct rgb_img));
    (*dest)->width = src->width - 1;
    (*dest)->height = src->height;
    (*dest)->raster = malloc(sizeof(uint8_t) * 3 * (*dest)->width * (*dest)->height);

    // copy source to dest
    int x, y;
    for (y = 0; y < src->height; y++)
    {
        for (x = 0; x < path[y]; x++)
        {
            // copy the pixel to the destination image
            set_pixel(*dest, y, x, get_pixel(src, y, x, 0), get_pixel(src, y, x, 1), get_pixel(src, y, x, 2));
        }
        for (x = path[y]+1; x < src->width; x++)
        {
            // copy the pixel to the destination image
            set_pixel(*dest, y, x-1, get_pixel(src, y, x, 0), get_pixel(src, y, x, 1), get_pixel(src, y, x, 2));
        }
    }
}

void remove_seam_test(struct rgb_img *src, struct rgb_img **dest, int *path)
{
    *dest = malloc(sizeof(struct rgb_img));
    (*dest)->width = src->width;
    (*dest)->height = src->height;
    (*dest)->raster = malloc(sizeof(uint8_t) * 3 * (*dest)->width * (*dest)->height);

    // copy source to dest
    for (int y = 0; y < src->height; y++)
    {
        for (int x = 0; x < src->width; x++)
        {
            // copy the pixel to the destination image
            if (x != path[y])
            {
                set_pixel(*dest, y, x, get_pixel(src, y, x, 0), get_pixel(src, y, x, 1), get_pixel(src, y, x, 2));
            }
            else
            {
                set_pixel(*dest, y, x, 0, 0, 0);
            }
        }
    }
}

int main()
{
    struct rgb_img *im;
    struct rgb_img *cur_im;
    struct rgb_img *grad;

    double *best;
    int *path;

    read_in_img(&im, "C:\\Users\\Qiren\\Desktop\\ESC190\\project_two\\HJoceanSmall.bin");
    for(int i = 0; i < 200; i++){
        printf("i = %d\n", i);
        calc_energy(im,  &grad);
        dynamic_seam(grad, &best);
        recover_path(best, grad->height, grad->width, &path);
        remove_seam(im, &cur_im, path);

        char filename[200];
        sprintf(filename, "C:\\Users\\Qiren\\Desktop\\ESC190\\project_two\\img.bin");
        write_img(cur_im, filename);


        destroy_image(im);
        destroy_image(grad);
        free(best);
        free(path);
        im = cur_im;
    }
    destroy_image(im);
}