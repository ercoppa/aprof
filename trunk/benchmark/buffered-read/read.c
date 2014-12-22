#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int read_file(int f, int i) {

    if (i >= 20) return 0;

    char buf[256];
    int res = read(f, buf, 256);
    if (res <= 0) return -1;
    printf("Loaded %d chars\n", res);
    int j, val;
    for (j = 0; j < 256; j++) val += buf[j];
    i++;
    
    return read_file(f, i);
}

int open_file() {
    
    int f = open("input_file", 0);
    if (f < 0) return -1;
    read_file(f, 0);
    
    return 0;
}

int main(void) {
    
    return open_file();
}
