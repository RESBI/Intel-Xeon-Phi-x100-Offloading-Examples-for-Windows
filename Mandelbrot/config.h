#pragma once
// Choose which precision to use
#define FLOAT_PRECISION double
// Remain some threads for uOS on Phi
#define MIC_THREAD_OFFSET -4
// Which kind of thread affinity to use
#define OMP_THREAD_AFFINITY compact 

#define ESCAPE 1000
// The size 'd better be divisible by 2. 
#define PAINT_WIDTH 1920*8
#define PAINT_HEIGH 1080*8

// If you have higher precision then use it, double have it's own limit. 
#define PRECISION double 

#define OFFSET_REAL -1.7687788
#define OFFSET_IMAG -0.0017389

#define PIXELS_PER_IDENTITY 4000000000*8*8*4ll

#define SAVING_PATH "./MANDELBROT.bmp"