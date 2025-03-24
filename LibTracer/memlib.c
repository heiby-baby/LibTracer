#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <sys/syscall.h> 
#include <unistd.h>       
#include <linux/time.h>
// Перечисление с числовыми значениями функциями
typedef enum {
    LOG_OPEN,
    LOG_CLOSE,
    LOG_LSEEK,
    LOG_READ,
    LOG_WRITE,
    LOG_MALLOC,
    LOG_CALLOC,
    LOG_REALLOC,
    LOG_FREE
} LogType;


// Структура в которой хранится наш лог
struct LogEntry {
    LogType type;
    struct timespec timestamp;
    union {
        struct {
            char filename[256];
            int flags;
            int file_descriptor;
        } open;
        struct {
            int file_descriptor;
            int return_code;
        } close;
        struct {
            int file_descriptor;
            off_t requested_offset;
            int whence;
            off_t resulted_offset;
        } lseek;
        struct {
            int file_descriptor;
            void* buffer_pointer;
            int count;
            int bytes_read;
        } read;
        struct {
            int file_descriptor;
            void* buffer_pointer;
            int count;
            int bytes_written;
        } write;
        struct {
            int bytes_requested;
            void * new_mem_pointer;

        } malloc;
        struct {
            
        } realloc;
        struct {
            
        } free;
    };
} ;


// Безопасная функция вывода, отладочкая
// (Безопасная по причине того, что не вызывает рекурсвиных вызовов)
void save_print(const char* format, ...) {
    char buffer[128];  // Статический буфер на стеке
    va_list args;
    // Инициализируем va_list
    va_start(args, format);

    // Форматируем строку в буфер
    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    // Завершаем работу с va_list
    va_end(args);

    // Выводим содержимое буфера в stderr с помощью syscall
    if (len > 0) {
        syscall(SYS_write, STDERR_FILENO, buffer, len);
    }
}


void create_log(LogType logType, ...) {
    struct LogEntry log;
    log.type = logType;
    clock_gettime(CLOCK_REALTIME, &log.timestamp);
    
    va_list args;
    va_start(args, logType);
    
    switch (logType) {
    }
    
    va_end(args);
}
// Обёртка для malloc
void* malloc(size_t size) {
    // Создаём указатель на функцию с сигнатурой функции malloc
    static void* (*original_malloc)(size_t) = NULL;

    // Получаем указатель на оригинальную функцию malloc
    if (!original_malloc) {
        // RTLD_NEXT значит: получить malloc который был бы вызван,
        // если бы не было обёртки
        original_malloc = dlsym(RTLD_NEXT, "malloc");
        if (!original_malloc) {
            safe_print("Ошибка при получении оригинального malloc\n");
            exit(1);
        }
    }

    // Вызываем оригинальную функцию malloc
    void* ptr = original_malloc(size);
    // Выводим информацию о вызове malloc
    create_log(LOG_MALLOC, size, ptr);
    return ptr;
}

// Обёртка для free
void free(void* ptr) {
    // Создаём указатель на функцию с сигнатурой функции free
    static void (*original_free)(void*) = NULL;

    // Получаем указатель на оригинальную функцию free
    if (!original_free) {
        // RTLD_NEXT значит: получить free который был бы вызван,
        // если бы не было обёртки
        original_free = dlsym(RTLD_NEXT, "free");
        if (!original_free) {
            safe_print("Ошибка при получении оригинального free\n");
            exit(1);
        }
    }

    // Выводим информацию о вызове free
    create_log(LOG_FREE, ptr);

    // Вызываем оригинальную функцию free
    original_free(ptr);
}

// Обёртка для realloc
void* realloc(void* ptr, size_t size) {
    // Создаём указатель на функцию с сигнатурой функции realloc
    static void* (*original_realloc)(void*, size_t) = NULL;

    // Получаем указатель на оригинальную функцию realloc
    if (!original_realloc) {
        // RTLD_NEXT значит: получить realloc который был бы вызван,
        // если бы не было обёртки
        original_realloc = dlsym(RTLD_NEXT, "realloc");
        if (!original_realloc) {
            safe_print("Ошибка при получении оригинального realloc\n");
            exit(1);
        }
    }

    // Вызываем оригинальную функцию realloc
    void* new_ptr = original_realloc(ptr, size);
    // Выводим информацию о вызове realloc
    create_log(LOG_REALLOC, size, ptr, new_ptr);
    return new_ptr;
}

// // Обёртка для calloc
// void* calloc(size_t num, size_t size) {
//     // Создаём указатель на функцию с сигнатурой функции calloc
//     static void* (*original_calloc)(size_t, size_t) = NULL;
//     // Получаем указатель на оригинальную функцию calloc
//     if (!original_calloc) {
//         // RTLD_NEXT значит: получить realloc который был бы вызван,
//         // если бы не было обёртки
//         original_calloc = dlsym(RTLD_NEXT, "calloc");
//         if (!original_calloc) {
//             safe_print("Ошибка при получении оригинального calloc\n");
//             exit(1);
//         }
//     }
//     void* ptr = original_calloc(num, size);
//     // Выводим информацию о вызове calloc
//     create_log(LOG_MALLOC, num * size, ptr); // Логируем как malloc, так как calloc - это malloc + memset
//     return ptr;
//     // Вызываем оригинальную функцию calloc
//     return original_calloc(num, size);
// }

// Обёртка для open
int open(const char* pathname, int flags, ...) {
    // Создаём указатель на функцию с сигнатурой функции open
    static int (*original_open)(const char*, int, ...) = NULL;
    // Получаем указатель на оригинальную функцию open,
    if (!original_open) {
        // RTLD_NEXT значит: получить open который был бы вызван,
        // если бы не было обёртки
        original_open = dlsym(RTLD_NEXT, "open");
        if (!original_open) {
            safe_print("Ошибка при получении оригинального open\n");
            exit(1);
        }
    }

    // Обработка variadic аргумента (mode_t)
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    // Выводим информацию о вызове open
    safe_print("Вызван open для файла: %s с флагами: %d и режимом: %d\n", pathname, flags, mode);

    // Вызываем оригинальную функцию open
    if (flags & O_CREAT) {
        return original_open(pathname, flags, mode);  // Три аргумента
    } else {
        return original_open(pathname, flags);  // Два аргумента
    }
}

// Обёртка для close
int close(int fd) {
     // Создаём указатель на функцию с сигнатурой функции close
    static int (*original_close)(int) = NULL;
    // Получаем указатель на оригинальную функцию close
    if (!original_close) {
        // RTLD_NEXT значит: получить close который был бы вызван,
        // если бы не было обёртки
        original_close = dlsym(RTLD_NEXT, "close");
        if (!original_close) {
            safe_print("Ошибка при получении оригинального close\n");
            exit(1);
        }
    }

    // Выводим информацию о вызове close
    safe_print("Вызван close для файлового дескриптора: %d\n", fd);

    // Вызываем оригинальную функцию close
    return original_close(fd);
}

// Обёртка для write
ssize_t write(int fd, const void* buf, size_t count) {
    // Получаем указатель на оригинальную функцию write
    static ssize_t (*original_write)(int, const void*, size_t) = NULL;
    // Получаем указатель на оригинальную функцию write
    // RTLD_NEXT значит: получить write который был бы вызван,
        // если бы не было обёртки
    if (!original_write) {
        original_write = dlsym(RTLD_NEXT, "write");
        if (!original_write) {
            safe_print("Ошибка при получении оригинального write\n");
            exit(1);
        }
    }

    // Выводим информацию о вызове write
    safe_print("Вызван write для файлового дескриптора: %d, размер: %zu\n", fd, count);

    // Вызываем оригинальную функцию write
    return original_write(fd, buf, count);
}

// Обёртка для read
ssize_t read(int fd, void* buf, size_t count) {
    // Получаем указатель на оригинальную функцию read
    static ssize_t (*original_read)(int, void*, size_t) = NULL;
    // Получаем указатель на оригинальную функцию read
    if (!original_read) {
        // RTLD_NEXT значит: получить read который был бы вызван,
        // если бы не было обёртки
        original_read = dlsym(RTLD_NEXT, "read");
        if (!original_read) {
            safe_print("Ошибка при получении оригинального read\n");
            exit(1);
        }
    }

    // Выводим информацию о вызове read
    safe_print("Вызван read для файлового дескриптора: %d, размер: %zu\n", fd, count);

    // Вызываем оригинальную функцию read
    return original_read(fd, buf, count);
}

// Обёртка для lseek
off_t lseek(int fd, off_t offset, int whence) {
    // Получаем указатель на оригинальную функцию lseek
    static off_t (*original_lseek)(int, off_t, int) = NULL;
    // Получаем указатель на оригинальную функцию lseej
    if (!original_lseek) {
        // RTLD_NEXT значит: получить lseek который был бы вызван,
        // если бы не было обёртки
        original_lseek = dlsym(RTLD_NEXT, "lseek");
        if (!original_lseek) {
            safe_print("Ошибка при получении оригинального lseek\n");
            exit(1);
        }
    }

    // Выводим информацию о вызове lseek
    safe_print("Вызван lseek для файлового дескриптора: %d, смещение: %ld, whence: %d\n", fd, offset, whence);

    // Вызываем оригинальную функцию lseek
    return original_lseek(fd, offset, whence);
}