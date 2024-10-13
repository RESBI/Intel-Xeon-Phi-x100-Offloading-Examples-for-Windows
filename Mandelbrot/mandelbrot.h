#pragma once

void mandelbrotDo(
    long long  size_x,
    long long  size_y,
    long long  size_x_half,
    long long  size_y_half,
    long long  size_length,
    PRECISION pixels_per_identity,

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
    );