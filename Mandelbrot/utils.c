#include <stdio.h>
#include <offload.h>

#include "config.h"
#include "utils.h"

// Choose a method to deliver tasks to each cards.
void deliveringTasks(
    int *accordence_parts,
    int accordence_total,
    int accordence_length, 
    long long num_tasks,
    long long *deliver_begins,
    long long *deliver_lengths
    ) {
    // Assume homogenius, deliver according to thread numbers. 
    // The truth. 
    deliver_begins[0] = 0; 
    // let's count their tasks
    for (int index = 0; index < accordence_length; index++) {
        long long task_num = (num_tasks * accordence_parts[index]) / accordence_total;
        deliver_lengths[index] = task_num; 
        if (index > 0) {
            deliver_begins[index] = deliver_begins[index - 1] + deliver_lengths[index - 1]; 
        }
    }
    // complete those remains
    deliver_lengths[accordence_length - 1] += (num_tasks - (deliver_begins[accordence_length - 1] + deliver_lengths[accordence_length - 1]));
}

TARGET_ATTRIBUTE
char getR(int n, int escape) {
    return (char)(n * 255 / (16 * escape) + 16);
    //return (char)(255-(ESCAPE/2*n)*(ESCAPE/2*n));
    //return 0;
    //return (char)(255-n*255/ESCAPE);
    //return (char)(255-4*atan(n/ESCAPE)/(3.1415926535)*255);
}

TARGET_ATTRIBUTE
char getG(int n, int escape) {
    //return (char)(255-(ESCAPE/2*(n-ESCAPE/2))*(ESCAPE/2*(n-ESCAPE/2)));
    //return 0;
    return (char)(n * 255 / (16 * escape) + 16);
    //return (char)(255-4*atan(n/ESCAPE)/(3.1415926535)*255);
}

TARGET_ATTRIBUTE
char getB(int n, int escape) {
    //return (char)(255-(ESCAPE/2*(n-ESCAPE))*(ESCAPE/2*(n-ESCAPE)));
    //return 0;
    //return (char)(255-n*255/ESCAPE);
    return (char)(n * 255 / escape);
    //return (char)(255-4*atan(n/ESCAPE)/(3.1415926535)*255);
}

void writeBMP(char *R, char *G, char *B, const char* filename, int H, int W) {
    // Renormalizing size
    int l = (W) / 4 * 4;
    // Headers
    int bmi[] = { l*H + 54,0,54,40,W,H,1 | 3 * 8 << 16,0,l*H,0,0,100,0 };
    FILE *fp = fopen(filename, "wb");
    fprintf(fp, "BM");
    fwrite(&bmi, 52, 1, fp);
    // Write image
    //fwrite(img, 1, l*H, fp);
    for (int index = 0; index < l * H; index++) {
        fwrite(&(R[index]), 1, 1, fp);
        fwrite(&(G[index]), 1, 1, fp);
        fwrite(&(B[index]), 1, 1, fp);
    }
    fclose(fp);
}
