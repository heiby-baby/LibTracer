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
    
    printf("Начало программы\n");
    
    for(int i = 0; i < 150; i++) {
        // Выделяем память с помощью malloc
        char* buffer = (char*)malloc(data_len + i + 1);
        if (!buffer) {
            perror("malloc");
            return 1;
        }

        // Выделяем память с помощью calloc
        char* buffer2 = (char*)calloc(data_len + i + 1, sizeof(char));
        if (!buffer2) {
            perror("calloc");
            free(buffer);
            return 1;
        }

        // Открываем файл для записи (создаём, если его нет)
        int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
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
        ssize_t bytes_read = read(fd, buffer, data_len + i);
        if (bytes_read == -1) {
            perror("read");
            close(fd);
            free(buffer);
            free(buffer2);
            return 1;
        }
        buffer[bytes_read] = '\0';  // Добавляем завершающий нулевой символ

        printf("Итерация %d: Прочитано из файла: %s\n", i, buffer);

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

        printf("Итерация %d: Прочитано после lseek: %s\n", i, buffer2);

        // Закрываем файл
        if (close(fd) == -1) {
            perror("close");
            free(buffer);
            free(buffer2);
            return 1;
        }

        sleep(1);

        // Используем realloc для изменения размера буфера
        char* temp = (char*)realloc(buffer, new_data_len + 1);
        if (!temp) {
            perror("realloc");
            free(buffer);
            free(buffer2);
            return 1;
        }
        buffer = temp;

        // Копируем новые данные в буфер
        strncpy(buffer, new_data, new_data_len);
        buffer[new_data_len] = '\0';

        printf("Итерация %d: Новые данные в буфере: %s\n", i, buffer);

        // Освобождаем память
        free(buffer);
        free(buffer2);

        printf("Итерация %d завершена.\n", i);
    }
    
    printf("Программа завершена.\n");
    return 0;
}