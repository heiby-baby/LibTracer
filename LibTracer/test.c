#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    const char* filename = "testfile.txt";
    const char* data = "Hello, World!";
    const char* new_data = "This is a test.";
    size_t data_len = strlen(data);
    size_t new_data_len = strlen(new_data);

    // Используем malloc для выделения памяти
    char* buffer = (char*)malloc(data_len + 1);
    if (!buffer) {
        perror("malloc");
        return 1;
    }

    // Используем calloc для выделения и обнуления памяти
    char* buffer2 = (char*)calloc(data_len + 1, sizeof(char));
    if (!buffer2) {
        perror("calloc");
        free(buffer);
        return 1;
    }

    // Открываем файл для записи (создаём, если его нет)
    //O_CREAT - создать файл если не существует, O_WRONLY - открыть только для записи 
    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        perror("open");
        free(buffer);
        free(buffer2);
        return 1;
    }

    // Записываем данные в файл
    ssize_t bytes_written = write(fd, data, data_len);
    if (bytes_written == -1) {
        perror("write");
        close(fd);
        free(buffer);
        free(buffer2);
        return 1;
    }

    // Закрываем файл
    if (close(fd) == -1) {
        perror("close");
        free(buffer);
        free(buffer2);
        return 1;
    }

    // Открываем файл для чтения
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        free(buffer);
        free(buffer2);
        return 1;
    }

    // Читаем данные из файла
    ssize_t bytes_read = read(fd, buffer, data_len);
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        free(buffer);
        free(buffer2);
        return 1;
    }
    buffer[bytes_read] = '\0';  // Добавляем завершающий нулевой символ

    printf("Прочитано из файла: %s\n", buffer);

    // Используем lseek для перемещения в начало файла
    off_t offset = lseek(fd, 0, SEEK_SET);
    if (offset == -1) {
        perror("lseek");
        close(fd);
        free(buffer);
        free(buffer2);
        return 1;
    }

    // Читаем данные снова (для демонстрации lseek)
    bytes_read = read(fd, buffer2, data_len);
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        free(buffer);
        free(buffer2);
        return 1;
    }
    buffer2[bytes_read] = '\0';  // Добавляем завершающий нулевой символ

    printf("Прочитано после lseek: %s\n", buffer2);

    // Закрываем файл
    if (close(fd) == -1) {
        perror("close");
        free(buffer);
        free(buffer2);
        return 1;
    }

    // Используем realloc для изменения размера буфера
    buffer = (char*)realloc(buffer, new_data_len + 1);
    if (!buffer) {
        perror("realloc");
        free(buffer2);
        return 1;
    }

    // Копируем новые данные в буфер
    strncpy(buffer, new_data, new_data_len);
    buffer[new_data_len] = '\0';

    printf("Новые данные в буфере: %s\n", buffer);

    // Освобождаем память
    free(buffer);
    free(buffer2);

    printf("Программа завершена.\n");
    return 0;
}