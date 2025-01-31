/*
    Visual Stuido Project Template For Xeon Phi Offloading Programs.
    Extracted From Official Tutorials.
    2024/10/11, Resbi. 

    Mandelbrot Drawing Program, Could be accelerated by multiple Xeon Phi x100 Coprocessors.
    Todo:
        Dynamically split tasks according to performance. 
        Generate animation, or at least frames. 
*/
#include <stdio.h>
#include <stdlib.h>
#include <offload.h>
#include <omp.h>

#include "config.h"
#include "utils.h"
#include "mandelbrot.h"

#ifndef __linux__
#include <windows.h>
#endif

int main() {
    // Start your codes here... 
    int num_coprocessors = _Offload_number_of_devices();

    // Barrier signals.
    int *signals;
    signals = (int *)malloc(sizeof(int) * num_coprocessors);

    // Collect threads
    int *threads; 
    int total_threads = 0; 
    long long *deliver_begins, *deliver_lengths;
    threads = (int *)malloc(sizeof(int) * num_coprocessors);
    deliver_begins = (long long *)malloc(sizeof(long long) * num_coprocessors);
    deliver_lengths = (long long *)malloc(sizeof(long long) * num_coprocessors);

    int escape_limit = ESCAPE; // int is enough, we don't have higher performance than this. 
    PRECISION offset_z_real = OFFSET_REAL;
    PRECISION offset_z_imag = OFFSET_IMAG;
    long long size_x = PAINT_WIDTH;
    long long size_y = PAINT_HEIGH;

    // Check if size is divisible by 2
    if (((size_x % 2) | (size_y % 2)) == 1) {
        printf("Size of image should be divisible by 2!\n"); 
        return 0; 
    }

    // Half size, for rendering offset
    long long size_x_half = size_x >> 1;
    long long size_y_half = size_y >> 1;
    long long size_length = size_x * size_y;
    PRECISION pixels_per_identity = PIXELS_PER_IDENTITY;

    //PRECISION *plane_real; 
    //PRECISION *plane_imag;
    //int *escape_steps;
    char *painting_board_r;
    char *painting_board_g;
    char *painting_board_b;
    char saving_path[100] = SAVING_PATH;

    //plane_real = (PRECISION *)malloc(sizeof(PRECISION) * size_length);
    //plane_imag = (PRECISION *)malloc(sizeof(PRECISION) * size_length);
    painting_board_r = (char *)malloc(sizeof(char) * size_length);
    painting_board_g = (char *)malloc(sizeof(char) * size_length);
    painting_board_b = (char *)malloc(sizeof(char) * size_length);
    //escape_steps = (int *)malloc(sizeof(int) * size_length);

    // Timing
    unsigned long transfer_start_millisecond, transfer_end_millisecond;
    double transfer_duration_millisecond;

    // Get number of threads on each card
    for (int card_index = 0; card_index < num_coprocessors; card_index++) {
        int *temp_threads = &(threads[card_index]); 
#pragma offload target(mic : card_index) signal(&(signals[card_index])) \
        inout(temp_threads : length(1))
        {
            temp_threads[0] = kmp_get_affinity_max_proc() + MIC_THREAD_OFFSET;
        }
    }
    for (int card_index = 0; card_index < num_coprocessors; card_index++) {
#pragma offload_wait target(mic : card_index) wait(&(signals[card_index]))
        total_threads += threads[card_index];
    }

    printf("[HOST] total threads: %d\n", total_threads);
    printf("[HOST] total tasks: %d\n", size_length);

    printf("[HOST] start delivering...\n"); 

    deliveringTasks(threads, total_threads, num_coprocessors, size_length, deliver_begins, deliver_lengths);

    for (int card_index = 0; card_index < num_coprocessors; card_index++) {
        printf("[MIC%d] threads on this card: %d\n", card_index, threads[card_index]);
        //printf("[MIC%d] begin at task: %d\n", card_index, deliver_begins[card_index]);
        printf("[MIC%d] delivered tasks: %d\n", card_index, deliver_lengths[card_index]);
    }


    printf("[HOST] Start preworks...\n");

    mandelbrotPreWorks(
            size_x,
            size_y,
            size_x_half,
            size_y_half,
            size_length,
            pixels_per_identity,

            escape_limit,
            offset_z_real,
            offset_z_imag,

            num_coprocessors,
            total_threads,
            threads,

            deliver_begins,
            deliver_lengths,

            painting_board_r,
            painting_board_g,
            painting_board_b
        );

    printf("[HOST] Now start calculation...\n"); 

    transfer_start_millisecond = GetTickCount();

    mandelbrotDo(
            size_x,
            size_y,
            size_x_half,
            size_y_half,
            size_length,
            pixels_per_identity,

            escape_limit,
            offset_z_real,
            offset_z_imag,

            num_coprocessors,
            total_threads,
            threads,

            deliver_begins,
            deliver_lengths,

            painting_board_r,
            painting_board_g,
            painting_board_b
        );

    transfer_end_millisecond = GetTickCount();
    transfer_duration_millisecond = transfer_end_millisecond - transfer_start_millisecond; 

    printf("[HOST] Done!\n");
    printf("[HOST] Cost : %.3f seconds\n", transfer_duration_millisecond / 1000);
    printf("[HOST] Speed: %.3f Pixels/s\n", size_length / (transfer_duration_millisecond / 1000));

    printf("[HOST] Now saving picture...\n"); 

    writeBMP(painting_board_r, painting_board_g, painting_board_b, saving_path, size_y, size_x);

    printf("[HOST] Now do afterworks...\n");

    mandelbrotAfterWorks(
            size_x,
            size_y,
            size_x_half,
            size_y_half,
            size_length,
            pixels_per_identity,

            escape_limit,
            offset_z_real,
            offset_z_imag,

            num_coprocessors,
            total_threads,
            threads,

            deliver_begins,
            deliver_lengths,

            painting_board_r,
            painting_board_g,
            painting_board_b
        );

    free(painting_board_r);
    free(painting_board_g);
    free(painting_board_b);
    
    free(deliver_begins); 
    free(deliver_lengths);

    free(threads);
    free(signals); 

    return 0; 
}