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

/*
#ifdef __linux__
#define MALLOC_ALIGNED aligned_alloc
#else

#define MALLOC_ALIGNED _aligned_malloc
#endif
#define ALIGNMENT_SIZE 32
*/

#define SUM_TYPE unsigned int

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
    int sig; 

    printf("[HOST] %d Coprocessors detected!\n", num_coprocs); 

    printf("[HOST] Generating data (size = %u bytes)...\n", data_size); 
    data = (char *)dataGen(data_size);
    printf("[HOST] Checking data sum...\n");
    data_sum = getSum(data_size, data);
    printf("[HOST] data sum = %u\n", data_sum);

    for (int card_index = 0; card_index < num_coprocs; card_index++) {
        printf("[HOST] Transferring data to mic%d...\n", card_index);

#pragma offload target(mic : card_index) signal(&sig) \
        in(card_index) \
        in(data_size) \
        in(data : length(data_size))
        {
           printf("[MIC%d] Done! Now checksum...\n", card_index);
           SUM_TYPE temp_data_sum = 0;
           printf("[MIC%d] Checking data sum...\n", card_index);
           temp_data_sum = getSum(data_size, data);
           printf("[MIC%d] data sum = %u\n", card_index, temp_data_sum);
        }
#pragma offload_wait target(mic : card_index) wait(&sig)
    }

    free(data);
    return 0; 
}