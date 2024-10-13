/*
    Visual Stuido Project Template For Xeon Phi Offloading Programs.
    Extracted From Official Tutorials.
    2024/10/11, Resbi. 
*/
#include <stdio.h>
#include <offload.h>
#include <omp.h>

int main() {
    // Start your codes here... 
    int mic_num = _Offload_number_of_devices();
    int sig1, sig2, test; 

    printf("# of co-proc: %d\n", mic_num);

    for (int mic_index = 0; mic_index < mic_num; mic_index++) { 
        test = 0; 
#pragma offload target(mic : mic_index) signal(&sig1) \
        inout(test)
        {
            char *hostname;
            int hostname_size;
            FILE *HostnameReading = fopen("/etc/hostname", "r");
            fseek(HostnameReading, 0, SEEK_END);
            hostname_size = ftell(HostnameReading);
            fseek(HostnameReading, 0, SEEK_SET);
            hostname = (char *)malloc(sizeof(char) * hostname_size);
            fread(hostname, 1, hostname_size, HostnameReading);
            fclose(HostnameReading);

            printf("Card %d :\n", mic_index);

            //printf("\ttest: %d\n", test); 
            printf("\tHello World from %s", hostname);
            printf("\t%d threads\n", kmp_get_affinity_max_proc());

            free(hostname);
            test += 1; 
        }
#pragma offload_wait target(mic : mic_index) wait(&sig1)

        /*
        // Test modifying part of data. 
        char data[10]; 
        char *data_temp; 

        data_temp = data + 2;

        for (int i = 0; i < 10; i++) {
            data[i] = 'O';
            printf("%c", data[i]); 
        }
        printf("\n");

#pragma offload target(mic : mic_index) signal(&sig2) \
        inout(test) \
        inout(data_temp:length(3)) 
        {
            printf("test: %d\n", test);
            for (int i = 0; i < 3; i++) {
                data_temp[i] = 'I';
                //printf("%c", data_temp[i]);
            }
            //printf("\n");
            test += 1; 
        }
#pragma offload_wait target(mic : mic_index) wait(&sig2)

        for (int i = 0; i < 10; i++) {
            printf("%c", data[i]);
        }
        printf("\n");
        printf("test: %d\n", test);
        */
    }
    return 0; 
}