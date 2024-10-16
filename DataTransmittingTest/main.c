/*
    Visual Stuido Project Template For Xeon Phi Offloading Programs.
    Extracted From Official Tutorials.
    2024/10/11, Resbi. 

    Data Transmitting Test.
    Goal: 
        Test data completeness
            1, generate data on Host and get sum(data) % 256
            2, transmit data to Phi and checksum

        Test data transmitting speed
            1, Host -> Card
            2, Card -> Host
*/
#include <stdio.h>
#include <stdlib.h>
#include <offload.h>
#include <omp.h>
#include <time.h>
#ifndef __linux__
#include <windows.h>
#endif

/*
#ifdef __linux__
#define MALLOC_ALIGNED aligned_alloc
#else
#define _mm_free(a)      _aligned_free(a)
#define _mm_malloc(a, b) _aligned_malloc(a, b)
#endif
#define ALIGNMENT_SIZE 32
*/

#define SUM_TYPE unsigned int

#define TRANSFER_LOOP_TIMES 10; 

/*
uint64_t getMicroseconds() {
    struct timespec time_pointer;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_pointer);
    return (time_pointer.tv_sec * 1000000 + time_pointer.tv_nsec / 1000);
}
*/

char *dataGen(unsigned long data_size) {
    //char *data = (char *)MALLOC_ALIGNED(ALIGNMENT_SIZE, sizeof(char) * data_size);
    char *data = (char *)malloc(sizeof(char) * data_size);
    srand(time(NULL)); 

    omp_set_num_threads(kmp_get_affinity_max_proc());
#pragma omp parallel for
    for (unsigned long index = 0; index < data_size; index++) {
        data[index] = rand() % 256;
    }
#pragma omp barrier

    return data; 
}

/*
TARGET_ATTRIBUTE // MIC attribute
SUM_TYPE getSum(unsigned int data_size, char *data) {
    return __sec_reduce_add(data[0:data_size]);
}
*/

TARGET_ATTRIBUTE // MIC attribute
SUM_TYPE getSum(unsigned long data_size, char *data) {
#ifdef __MIC__
    int num_threads = kmp_get_affinity_max_proc();
#else
    int num_threads = kmp_get_affinity_max_proc();
#endif
    int num_splits = 4 * num_threads; 
    int num_each_split = data_size / num_splits; 
    int num_remain = data_size % num_splits;

    SUM_TYPE data_sum = 0;
    SUM_TYPE *data_sums;
    data_sums = (SUM_TYPE *)malloc(sizeof(SUM_TYPE) * (num_splits + 1));
    data_sums[num_splits] = 0; // The redundent one. 
    omp_set_num_threads(num_threads);

    // If datasize aligned with thread number, 
    if (num_remain == 0) {
#pragma omp parallel for schedule(dynamic)
        for (int index = 0; index < num_splits; index++) {
            data_sums[index] = 0;
            unsigned long base_address = index * num_each_split;
            for (int i = 0; i < num_each_split; i++) {
                unsigned long address = base_address + i;
                data_sums[index] += (SUM_TYPE)address * (SUM_TYPE)data[address];
            }
        }
#pragma omp barrier
    } 
    // If not,
    else {
#pragma omp parallel for schedule(dynamic)
        for (int index = 0; index <= num_splits; index++) {
            data_sums[index] = 0;
            int upper_bound = (index == num_splits) ? num_remain : num_each_split;
            unsigned long base_address = index * num_each_split;
            for (int i = 0; i < upper_bound; i++) {
                unsigned long address = base_address + i;
                data_sums[index] += (SUM_TYPE)address * (SUM_TYPE)data[address];
            }
        }
#pragma omp barrier
    }

    for (int i = 0; i <= num_splits; i++) {
        data_sum += data_sums[i]; 
    }

    free(data_sums); 

    return data_sum; 
}


int main() {
    // Start your codes here... 
    unsigned long data_size = 2 * 1024 * 1024 * 1024; // 2 GB, Can't bigger than 3 GB
    char *data; 
    SUM_TYPE data_sum;
    int num_coprocs = _Offload_number_of_devices();
    int transfer_loop_times = TRANSFER_LOOP_TIMES; 
    int sig; 

    printf("[HOST] %d Coprocessors detected!\n", num_coprocs); 

    printf("[HOST] Generating data (size = %u bytes)...\n", data_size); 
    data = (char *)dataGen(data_size);
    printf("[HOST] Checking data sum...\n");
    data_sum = getSum(data_size, data);
    printf("[HOST] data sum = %u\n", data_sum);

    unsigned long transfer_start_millisecond, transfer_end_millisecond;
    double transfer_duration_millisecond;

    for (int card_index = 0; card_index < num_coprocs; card_index++) {
        printf("[MIC%d] Allocating memory space on mic%d...\n", card_index, card_index);

#pragma offload target(mic : card_index) signal(&sig) \
        in(data : length(data_size) alloc_if(1) free_if(0))
        {
        }
//#pragma offload_wait target(mic : card_index) wait(&sig)

        printf("[HOST] Transferring data to mic%d for %d times...\n", card_index, transfer_loop_times);

        // GetTickCount only works on Windows
        transfer_start_millisecond = GetTickCount(); 
        for (int index_transfer_times = 0; index_transfer_times < transfer_loop_times; index_transfer_times++) {
#pragma offload target(mic : card_index) signal(&sig) \
        in(data : length(data_size) alloc_if(0) free_if(0))
            {
            }
#pragma offload_wait target(mic : card_index) wait(&sig)
        }
        transfer_end_millisecond = GetTickCount();
        transfer_duration_millisecond = (transfer_end_millisecond - transfer_start_millisecond) / transfer_loop_times; 

        printf("[HOST] Done!\n");
        printf("[HOST] Cost : %.3f seconds\n", transfer_duration_millisecond / 1000);
        printf("[HOST] Speed: %.3f MiB/s\n", (data_size / 1024 / 1024) / (transfer_duration_millisecond / 1000));

        printf("[HOST] Now begin checksum on mic%d...\n", card_index); 

        SUM_TYPE temp_data_sum = 0;
#pragma offload target(mic : card_index) signal(&sig) \
        in(card_index) \
        in(data_size) \
        out(temp_data_sum) \
        in(data : length(data_size) alloc_if(0) free_if(1))
        {
           temp_data_sum = getSum(data_size, data);
           printf("[MIC%d] data sum = %u\n", card_index, temp_data_sum);
        }
#pragma offload_wait target(mic : card_index) wait(&sig)
        if (temp_data_sum == data_sum) {
            printf("[HOST] Datasum agree!\n"); 
        }
        else {
            printf("[HOST] Datasum disagree!\n");
        }
    }

    free(data);
    return 0; 
}
