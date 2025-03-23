#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

struct shared_data {
    pthread_mutex_t mutex;
    int counter;
};

int main() {
    int created = 0;
    int fd = shm_open("/my_shared_mem", O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1) {
        if (errno == EEXIST) {
            fd = shm_open("/my_shared_mem", O_RDWR, 0666);
            if (fd == -1) {
                perror("shm_open existing");
                exit(1);
            }
        } else {
            perror("shm_open create");
            exit(1);
        }
    } else {
        created = 1;
        if (ftruncate(fd, sizeof(struct shared_data)) == -1) {
            perror("ftruncate");
            exit(1);
        }
    }

    struct shared_data *data = mmap(NULL, sizeof(struct shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(1);
    }
    close(fd);

    if (created) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        if (pthread_mutex_init(&data->mutex, &attr) != 0) {
            perror("pthread_mutex_init");
            exit(1);
        }
        pthread_mutexattr_destroy(&attr);
        data->counter = 0;
    }

    printf("Enter number to reset counter:\n");

    char buf[10];
    int num, rc;
    do {
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            printf("Input error!\n");
            break;
        }
        num = atoi(buf);

        rc = pthread_mutex_trylock(&data->mutex);
        if (rc == EBUSY) {
            printf("Mutex busy. Waiting...\n");
            pthread_mutex_lock(&data->mutex);
        } else if (rc != 0) {
            printf("Error locking mutex: %s\n", strerror(rc));
            break;
        }

        data->counter = num;
        printf("Counter set to: %d\n", data->counter);
        pthread_mutex_unlock(&data->mutex);
    } while(1);

    munmap(data, sizeof(struct shared_data));
    return 0;
}