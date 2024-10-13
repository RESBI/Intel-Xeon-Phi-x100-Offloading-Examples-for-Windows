#pragma once
void deliveringTasks(
    int *accordence_parts,
    int accordence_total,
    int accordence_length,
    long long num_tasks,
    long long *deliver_begins,
    long long *deliver_lengths
    );

void writeBMP(char *R, char *G, char *B, const char* filename, int H, int W);
TARGET_ATTRIBUTE char getR(int n, int escape);
TARGET_ATTRIBUTE char getG(int n, int escape);
TARGET_ATTRIBUTE char getB(int n, int escape);
