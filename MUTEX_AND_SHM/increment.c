#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

struct shared_data {
    pthread_mutex_t mutex;
    int counter;
};

int main() {
    int fd = shm_open("/my_shared_mem", O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    struct shared_data *data = mmap(NULL, sizeof(struct shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(1);
    }
    close(fd);

    do {
        usleep(10);
        pthread_mutex_lock(&data->mutex);
        data->counter++;
        printf("Increment: %d\n", data->counter);
        sleep(1);
        pthread_mutex_unlock(&data->mutex);
    } while(1);

    munmap(data, sizeof(struct shared_data));
    return 0;
}