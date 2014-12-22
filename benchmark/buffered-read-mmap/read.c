#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#define PAGE_SIZE 4096

int read_file() {

    int i = 0;
    int f = open("input_file", O_RDONLY);
    if (f < 0) return -1;
    void * addr = NULL;
    int br = 0;
    while(i < 20) {
        
        addr = mmap(addr, PAGE_SIZE, PROT_READ, MAP_SHARED, f, i*PAGE_SIZE);
        if (addr == MAP_FAILED) {
            printf("Error during mmap()\n");
            perror(NULL);
            return -1;
        }
        
        printf("Mapped %d chars at %llu\n", PAGE_SIZE, 
            (long long unsigned int) addr);
        int j, val;
        for (j = 0; j < PAGE_SIZE; j++) val += ((char *)addr)[j];
        
        br += j;
        
        int res = munmap(addr, PAGE_SIZE);
        if (res == -1) {
            printf("Error during munmap()\n");
            return -1;
        }
        
        i++;
    }    
    
    printf("Read %d (%d) bytes using mmap\n", br, br/4);
    close(f);
    return 0;
}

int main(void) {
    return read_file();
}
