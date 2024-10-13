/*
    Mandelbrot calculating & rendering kernel. 
*/

#include <stdio.h>
#include <stdlib.h> 
#include <offload.h>
#include <omp.h>

#include "config.h"
#include "mandelbrot.h"
#include "utils.h"

TARGET_ATTRIBUTE
void mandelbrotDo(
        long long  size_x,
        long long  size_y,
        long long  size_x_half,
        long long  size_y_half,
        long long  size_length,
        PRECISION  pixels_per_identity,

        int escape_limit, 
        PRECISION offset_z_real,
        PRECISION offset_z_imag,

        int num_coprocessors, 
        int total_threads, 
        int *threads, 

        long long *deliver_begins,
        long long *deliver_lengths,

        char *painting_board_r, 
        char *painting_board_g,
        char *painting_board_b
    ) {

    int *signals;
    signals = (int *)malloc(sizeof(int) * num_coprocessors);

    for (int card_index = 0; card_index < num_coprocessors; card_index++) {
        int num_threads_mic = threads[card_index];
        int tasks_begin_mic = deliver_begins[card_index];
        int tasks_length_mic = deliver_lengths[card_index]; 

        char *painting_board_r_mic = &(painting_board_r[tasks_begin_mic]);
        char *painting_board_g_mic = &(painting_board_g[tasks_begin_mic]);
        char *painting_board_b_mic = &(painting_board_b[tasks_begin_mic]);

#pragma offload target(mic : card_index) signal(&(signals[card_index])) \
        in(num_threads_mic) \
        in(size_x, size_y, size_x_half, size_y_half, size_length, pixels_per_identity) \
        in(escape_limit, offset_z_real, offset_z_imag) \
        in(tasks_begin_mic, tasks_length_mic) \
        out(painting_board_r_mic : length(tasks_length_mic)) \
        out(painting_board_g_mic : length(tasks_length_mic)) \
        out(painting_board_b_mic : length(tasks_length_mic)) 
        {
            omp_set_num_threads(num_threads_mic);

            int *escape_steps; 
            escape_steps = (int *)malloc(sizeof(int) * tasks_length_mic); 

            // Calculate
#pragma omp parallel for
            for (long long task_index = 0; task_index < tasks_length_mic; task_index++) {
                escape_steps[task_index] = 0;
                int now_length = tasks_begin_mic + task_index; 
                int now_y = now_length / size_x; 
                int now_x = now_length % size_x;
                PRECISION z_real = 0; 
                PRECISION z_imag = 0; 
                PRECISION c_real = (PRECISION)(now_x - size_x_half) / (PRECISION)pixels_per_identity + offset_z_real;
                PRECISION c_imag = (PRECISION)(size_y_half - now_y) / (PRECISION)pixels_per_identity + offset_z_imag; // Flip upside down. 
                PRECISION a, b; 

                while ((escape_steps[task_index] < escape_limit) && (z_real * z_real + z_imag * z_imag < 4)) { 
                    a = z_real * z_real - z_imag * z_imag; 
                    z_imag = 2 * z_real * z_imag + c_imag; 
                    z_real = a + c_real; 

                    escape_steps[task_index] += 1; 
                }
            }
#pragma omp barrier
            // Render 
#pragma omp parallel for
            for (long long task_index = 0; task_index < tasks_length_mic; task_index++) {
                painting_board_r_mic[task_index] = getR(escape_steps[task_index], escape_limit);
                painting_board_g_mic[task_index] = getG(escape_steps[task_index], escape_limit);
                painting_board_b_mic[task_index] = getB(escape_steps[task_index], escape_limit);
            }
#pragma omp barrier
        }
    }
    for (int card_index = 0; card_index < num_coprocessors; card_index++) {
#pragma offload_wait target(mic : card_index) wait(&(signals[card_index]))
    }
}