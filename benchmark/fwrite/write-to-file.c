#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static int buffer[1024] = { 0 };

void write_data(int fd, int * buf, int size) {
    
    write(fd, buf, size*sizeof(int));
    
}

int main(void) {

    int fp = open("input_file", O_RDWR|O_TRUNC|O_CREAT, S_IRWXU|S_IRWXG);
    if (fp < 0) {
        printf("I couldn't open results.dat for writing.\n");
        perror(NULL);
        exit(-1);
    }

    write_data(fp, buffer, 1024);
    
    close(fp);
    return 0;
}
