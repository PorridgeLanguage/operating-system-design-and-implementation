#include "ulib.h"

#define MAX_LINES 1024
#define LINE_LENGTH 256
#define FILE_SIZE 4096

char *lines[MAX_LINES];  // Array to hold the lines
int line_count = 0;      // Number of lines read

int partition(char *arr[], int low, int high);

// Function to implement strdup
char *my_strdup(const char *s) {
    int len = strlen(s)+0x1;
    char *copy = (char *)malloc(len); // Allocate memory
    if (copy) {
        memset(copy, 0, len);
        strcpy(copy, s); // Copy the string
    }
    return copy;
}

// Compare function for qsort
int compare(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// Function to implement quicksort
void quicksort(char *arr[], int low, int high) {
    if (low < high) {
        int pivot = partition(arr, low, high);
        quicksort(arr, low, pivot - 1);
        quicksort(arr, pivot + 1, high);
    }
}

// Partition function for quicksort
int partition(char *arr[], int low, int high) {
    char *pivot = arr[high];
    int i = (low - 1);
    for (int j = low; j < high; j++) {
        if (strcmp(arr[j], pivot) < 0) {
            i++;
            // Swap arr[i] and arr[j]
            char *temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    // Swap arr[i + 1] and arr[high] (or pivot)
    char *temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    return (i + 1);
}

// Function to read lines from the file and sort them
void sort_file(int fd) {
    struct stat st;
    fstat(fd, &st);
    if(!st.size){
        return;
    }
    else if (st.size < 0){
        printf("file size error!\n");
        return;
    }
    char *buf = malloc(FILE_SIZE);
    memset(buf, 0, FILE_SIZE);

    int n = 0;
    int type = st.type;
    int total_read = 0;
    while((n = read(fd, buf+total_read, FILE_SIZE-total_read)) > 0) {
        total_read += n;
        if(type == TYPE_FIFO) break;
        if(total_read == FILE_SIZE) break; // for the sake of reading more than buffer size 
    }
    
    char seps[] = "\n\0";
    char *token = strtok(buf, seps);
	while(token != NULL){
        lines[line_count++] = my_strdup(token); // Store the line
        if (line_count >= MAX_LINES) break; // Prevent overflow
		token=strtok(NULL,seps); // use strtok to getline
	}

    quicksort(lines, 0, line_count - 1); // Sort the lines

    // Write the sorted lines to standard output
    for (int i = 0; i < line_count; i++) {
        printf("%s\n", lines[i]); // Output the line
        free(lines[i]); // Free allocated memory
    }
}

int main(int argc, char *argv[]) {
    int fd = 0;
    if (argc > 2) {
        fprintf(2, "usage: sort filename\n");
        exit(1);
    }
    else if (argc == 2){
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            printf("sort: cannot open %s\n", argv[1]);
            exit(1);
        }
    }

    sort_file(fd);
    close(fd);
    exit(0);
}