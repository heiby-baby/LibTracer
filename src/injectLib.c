#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>  // off_t
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h> 

//#define DEBUG

// Для отладки
#ifdef DEBUG
    #include <sys/syscall.h> 
    #include <stdio.h>
#endif

// Имя разделяемой памяти
#define SHM_NAME "/log_shm"

// Размер который выделяется под разделяемую память
#define SHM_SIZE (sizeof(struct LogEntry) * 100)


// Перечисление для статуса лога 
//(успешный или нет (если не успешный, то код ошибки))
typedef enum {
    SUCCESS,
    FAIL_OPEN,
    FAIL_CLOSE,
    FAIL_LSEEK,
    FAIL_READ,
    FAIL_WRITE,
    FAIL_MALLOC,
    FAIL_REALLOC,
    FAIL_FREE
} Status;

// Перечисление с числовыми значениями функций
typedef enum {
    LOG_OPEN,
    LOG_CLOSE,
    LOG_LSEEK,
    LOG_READ,
    LOG_WRITE,
    LOG_MALLOC,
    LOG_REALLOC,
    LOG_FREE
} LogType;

// Структура в которой хранится наш лог
struct LogEntry {
    char function[32];                  // Имя функции (например, "open", "read", "malloc" и т.д.)
    LogType logType;                    
    struct timespec timestamp;          // Временная метка (секунды и наносекунды)
    Status status;                      // Код статуса выполнения (0 в случае успеха, код ошибки в случае сбоя)

    union {                             // Объединение для хранения параметров и результатов вызовов разных функций
        struct {                        // Данные для функции open()
            char filename[256];         // Имя файла, с которым работаем
            int flags;                  // Флаги открытия (O_RDONLY, O_WRONLY и т.д.)
            int file_descriptor;        // Возвращаемый файловый дескриптор
        } open;
        
        struct {                        // Данные для функции close()
            int file_descriptor;        // Файловый дескриптор, который закрываем
            int return_code;            // Результат выполнения (0 в случае успеха, -1 при ошибке)
        } close;
        
        struct {                        // Данные для функции lseek()
            int file_descriptor;        // Файловый дескриптор
            off_t requested_offset;     // Запрашиваемое смещение
            int whence;                 // База смещения (SEEK_SET, SEEK_CUR, SEEK_END)
            off_t resulted_offset;      // Фактическое смещение после вызова
        } lseek;
        
        struct {                        // Данные для функции read()
            int file_descriptor;        // Файловый дескриптор
            void* buffer_pointer;       // Указатель на буфер для чтения
            size_t count;               // Запрошенное количество байт для чтения
            ssize_t bytes_read;         // Фактически прочитанное количество байт
        } read;
        
        struct {                        // Данные для функции write()
            int file_descriptor;        // Файловый дескриптор
            void* buffer_pointer;       // Указатель на буфер с данными для записи
            size_t count;               // Количество байт для записи
            ssize_t bytes_written;      // Фактически записанное количество байт
        } write;
        
        struct {                        // Данные для функции malloc()
            size_t bytes_requested;     // Запрашиваемый размер памяти (в байтах)
            void * new_mem_pointer;     // Указатель на выделенную память (или NULL при ошибке)
        } malloc;
        
        struct {                        // Данные для функции realloc()
            size_t bytes_requested;     // Новый запрашиваемый размер памяти
            void * current_mem_pointer; // Указатель на текущий блок памяти
            void * new_mem_pointer;     // Указатель на новый блок памяти (или NULL при ошибке)
        } realloc;
        
        struct {                        // Данные для функции free()
            void * mem_pointer;         // Указатель на освобождаемую память
        } free;
    };
};

// Структура для shared memory
typedef struct {
    int head;                   // Индекс для записи
    int tail;                   // Индекс для чтения
    struct LogEntry logs[100];  // Кольцевой буфер
} LogShm;


static LogShm *shm = NULL;

// Инициализация shared memory
void init_shm() {
    // Открываем shared memory
    const int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    // Если ошибка завершаем работу программы
    if (shm_fd == -1) {
        exit(EXIT_FAILURE);
    }

    // Маппируем shared memory в адресное пространство процесса
    shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        exit(EXIT_FAILURE);
    }
    shm->head = 0;
    shm->tail = 0;
}

// Функция для записи лога в shared memory
void write_to_shm(struct LogEntry log) {
    if (shm == NULL) {
        init_shm();
    }
    
    // Записываем лог в текущую позицию head
    shm->logs[shm->head] = log;
    
    // Обновляем head с учетом кольцевого буфера
    shm->head = (shm->head + 1) % 100;
    
    // Если буфер полный, двигаем tail
    if (shm->head == shm->tail) {
        shm->tail = (shm->tail + 1) % 100;
    }
}


// Безопасная функция вывода, отладочкая
// (Безопасная по причине того, что не вызывает рекурсвиных вызовов)
static void write_log(struct LogEntry log) {
    
    write_to_shm(log);
    
    // Для отладки будет определять DEBUG
    #ifdef DEBUG
    char buffer[1024];
    int len;
    long sec = log.timestamp.tv_sec;
    long msec = log.timestamp.tv_nsec / 1000000;

    switch (log.logType) {
        case LOG_OPEN:
            len = snprintf(buffer, sizeof(buffer), "[%ld.%03ld] %s: filename='%s', flags=%d, fd=%d, status=%d\n",
                    sec, msec,
                    log.function, log.open.filename, log.open.flags, 
                    log.open.file_descriptor, log.status);
            break;
        case LOG_CLOSE:
            len = snprintf(buffer, sizeof(buffer), "[%ld.%03ld] %s: fd=%d, ret=%d, status=%d\n",
                    sec, msec,
                    log.function, log.close.file_descriptor, 
                    log.close.return_code, log.status);
            break;
        case LOG_LSEEK:
            len = snprintf(buffer, sizeof(buffer), "[%ld.%03ld] %s: fd=%d, req_offset=%ld, whence=%d, res_offset=%ld, status=%d\n",
                    sec, msec,
                    log.function, log.lseek.file_descriptor,
                    (long)log.lseek.requested_offset, log.lseek.whence,
                    (long)log.lseek.resulted_offset, log.status);
            break;
        case LOG_READ:
            len = snprintf(buffer, sizeof(buffer), "[%ld.%03ld] %s: fd=%d, buf=%p, count=%zu, bytes_read=%zd, status=%d\n",
                    sec, msec,
                    log.function, log.read.file_descriptor,
                    log.read.buffer_pointer, log.read.count, 
                    log.read.bytes_read, log.status);
            break;
        case LOG_WRITE:
            len = snprintf(buffer, sizeof(buffer), "[%ld.%03ld] %s: fd=%d, buf=%p, count=%zu, bytes_written=%zd, status=%d\n",
                    sec, msec,
                    log.function, log.write.file_descriptor,
                    log.write.buffer_pointer, log.write.count, 
                    log.write.bytes_written, log.status);
            break;
        case LOG_MALLOC:
            len = snprintf(buffer, sizeof(buffer), "[%ld.%03ld] %s: bytes=%zu, ptr=%p, status=%d\n",
                    sec, msec,
                    log.function, log.malloc.bytes_requested, 
                    log.malloc.new_mem_pointer, log.status);
            break;
        case LOG_REALLOC:
            len = snprintf(buffer, sizeof(buffer), "[%ld.%03ld] %s: bytes=%zu, old_ptr=%p, new_ptr=%p, status=%d\n",
                    sec, msec,
                    log.function, log.realloc.bytes_requested,
                    log.realloc.current_mem_pointer, log.realloc.new_mem_pointer,
                    log.status);
            break;
        case LOG_FREE:
            len = len = snprintf(buffer, sizeof(buffer), "[%ld.%03ld] %s: ptr=%p, status=%d\n",
                    sec, msec,
                    log.function, log.free.mem_pointer, log.status);
            break;
        default:
            len = len = snprintf(buffer, sizeof(buffer), "[%ld.%03ld] Unknown log type, status=%d\n", 
                    sec, msec, log.status);
    }
    if (len > 0) {
        syscall(SYS_write, STDERR_FILENO, buffer, len);
    }
    #endif
}
// Функия create_log об
void create_log(Status status, LogType logType, ...) {
    //Создаём наш лог объект
    struct LogEntry log;
    //Получаем время в UNIX формате
    log.logType = logType;
    clock_gettime(CLOCK_REALTIME, &log.timestamp);
    log.status = status;
    va_list args;
    va_start(args, logType);
    
    switch (logType) {
        case LOG_OPEN:
            //Записываем имя функции open
            strncpy(log.function, "open", sizeof(log.function));
            // Считываем из списка аргументов
            strncpy(log.open.filename, va_arg(args, const char*), sizeof(log.open.filename));
            log.open.flags = va_arg(args, int);
            log.open.file_descriptor = va_arg(args, int);
            break;
        case LOG_CLOSE:
            //Записываем имя функции close
            strncpy(log.function, "close", sizeof(log.function));
            log.close.file_descriptor = va_arg(args, int);
            log.close.return_code = va_arg(args, int);
            break;
        case LOG_LSEEK:
            //Записываем имя функции lseek        
            strncpy(log.function, "lseek", sizeof(log.function));
            log.lseek.file_descriptor = va_arg(args, int);
            log.lseek.requested_offset = va_arg(args, off_t);
            log.lseek.whence = va_arg(args, int);
            log.lseek.resulted_offset = va_arg(args, off_t);
            break;
        case LOG_READ:
            //Записываем имя функции read
            strncpy(log.function, "read", sizeof(log.function));
            log.read.file_descriptor = va_arg(args, int);
            log.read.buffer_pointer = va_arg(args, void*);
            log.read.count = va_arg(args, size_t);
            log.read.bytes_read = va_arg(args, ssize_t);
            break;
        case LOG_WRITE:
            //Записываем имя функции write
            strncpy(log.function, "write", sizeof(log.function));
            log.write.file_descriptor = va_arg(args, int);
            log.write.buffer_pointer = va_arg(args, void*);
            log.write.count = va_arg(args, size_t);
            log.write.bytes_written = va_arg(args, ssize_t);
            break;
        case LOG_MALLOC:
            //Записываем имя функции malloc
            strncpy(log.function, "malloc", sizeof(log.function));
            log.malloc.bytes_requested = va_arg(args, size_t);
            log.malloc.new_mem_pointer = va_arg(args, void *);
            break;
        case LOG_REALLOC:
            //Записываем имя функции realloc
            strncpy(log.function, "realloc", sizeof(log.function));
            log.realloc.bytes_requested = va_arg(args, size_t);
            log.realloc.current_mem_pointer = va_arg(args, void *);
            log.realloc.new_mem_pointer = va_arg(args, void*);
            break;
        case LOG_FREE:
            //Записываем имя функции free
            strncpy(log.function, "free", sizeof(log.function));
            log.free.mem_pointer = va_arg(args, void *);
            break;
    }
    va_end(args);
    //На этом месте будет вызов функции которая работает с разделяемой памятью
    write_log(log);
}
// Обёртка для malloc
void* malloc(size_t bytes_requested) {
    // Создаём указатель на функцию с сигнатурой функции malloc
    static void* (*original_malloc)(size_t) = NULL;

    // Получаем указатель на оригинальную функцию malloc
    if (!original_malloc) {
        // RTLD_NEXT значит: получить malloc который был бы вызван,
        // если бы не было обёртки
        original_malloc = dlsym(RTLD_NEXT, "malloc");
        if (!original_malloc) {
            create_log(FAIL_MALLOC, LOG_MALLOC, bytes_requested, NULL);
            exit(1);
        }
    }

    // Вызываем оригинальную функцию malloc
    void* ptr = original_malloc(bytes_requested);
    // Выводим информацию о вызове malloc
    create_log(SUCCESS, LOG_MALLOC, bytes_requested, ptr);
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
            create_log(FAIL_FREE, LOG_FREE, NULL);
            exit(1);
        }
    }

    // Выводим информацию о вызове free
    create_log(SUCCESS, LOG_FREE, ptr);

    // Вызываем оригинальную функцию free
    original_free(ptr);
}

// Обёртка для realloc
void* realloc(void* ptr, size_t bytes_requested) {
    // Создаём указатель на функцию с сигнатурой функции realloc
    static void* (*original_realloc)(void*, size_t) = NULL;

    // Получаем указатель на оригинальную функцию realloc
    if (!original_realloc) {
        // RTLD_NEXT значит: получить realloc который был бы вызван,
        // если бы не было обёртки
        original_realloc = dlsym(RTLD_NEXT, "realloc");
        if (!original_realloc) {
            create_log(FAIL_REALLOC, LOG_REALLOC, bytes_requested, ptr, NULL);
            exit(1);
        }
    }

    // Вызываем оригинальную функцию realloc
    void* new_ptr = original_realloc(ptr, bytes_requested);
    // Выводим информацию о вызове realloc
    create_log(SUCCESS, LOG_REALLOC, bytes_requested, ptr, new_ptr);
    return new_ptr;
}

// Обёртка для open
int open(const char* filename, int flags, ...) {
    // Создаём указатель на функцию с сигнатурой функции open
    static int (*original_open)(const char*, int, ...) = NULL;
    // Получаем указатель на оригинальную функцию open,
    if (!original_open) {
        // RTLD_NEXT значит: получить open который был бы вызван,
        // если бы не было обёртки
        original_open = dlsym(RTLD_NEXT, "open");
        if (!original_open) {
            create_log(FAIL_OPEN, LOG_OPEN, filename, flags, 1);
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


    // Вызываем оригинальную функцию open
    // Параметр mode не передаётся, т.к. в требованиях не указано
    // конструкция if позволяет реализовать эту логику прям на месте (c небольшими правками в create_log и LogEntry)
    int file_discriptor = -1;
    if (flags & O_CREAT) {
        file_discriptor = original_open(filename, flags, mode); 
    } else {
        file_discriptor = original_open(filename, flags);
    }
    create_log(SUCCESS, LOG_OPEN, filename, flags, file_discriptor);
    return file_discriptor;
}

// Обёртка для close
int close(int file_descriptor) {
     // Создаём указатель на функцию с сигнатурой функции close
    static int (*original_close)(int) = NULL;
    // Получаем указатель на оригинальную функцию close
    if (!original_close) {
        // RTLD_NEXT значит: получить close который был бы вызван,
        // если бы не было обёртки
        original_close = dlsym(RTLD_NEXT, "close");
        if (!original_close) {
            create_log(FAIL_CLOSE, LOG_CLOSE, file_descriptor, -1);
            exit(1);
        }
    }

    // Вызываем оригинальную функцию close
    int return_code = original_close(file_descriptor);

    // Выводим информацию о вызове close
    create_log(SUCCESS, LOG_CLOSE, file_descriptor, return_code);
    

    return return_code;
}

// Обёртка для write
ssize_t write(int file_descriptor, const void* buf, size_t count) {
    // Получаем указатель на оригинальную функцию write
    static ssize_t (*original_write)(int, const void*, size_t) = NULL;
    // Получаем указатель на оригинальную функцию write
    // RTLD_NEXT значит: получить write который был бы вызван,
        // если бы не было обёртки
    if (!original_write) {
        original_write = dlsym(RTLD_NEXT, "write");
        if (!original_write) {
            create_log(FAIL_WRITE, LOG_WRITE, file_descriptor, buf, count, -1);
            exit(1);
        }
    }

    
    // Вызываем оригинальную функцию write
    int return_code = original_write(file_descriptor, buf, count);

    // Выводим информацию о вызове write
    create_log(SUCCESS, LOG_WRITE, file_descriptor, buf, count, return_code);

    return return_code;
}

// Обёртка для read
ssize_t read(int file_descriptor, void* buf, size_t count) {
    // Получаем указатель на оригинальную функцию read
    static ssize_t (*original_read)(int, void*, size_t) = NULL;
    // Получаем указатель на оригинальную функцию read
    if (!original_read) {
        // RTLD_NEXT значит: получить read который был бы вызван,
        // если бы не было обёртки
        original_read = dlsym(RTLD_NEXT, "read");
        if (!original_read) {
            create_log(FAIL_READ, LOG_READ, file_descriptor, buf, count,-1);
            exit(1);
        }
    }


    // Вызываем оригинальную функцию read
    int bytes_read =  original_read(file_descriptor, buf, count);

    // Выводим информацию о вызове read
    create_log(SUCCESS, LOG_READ, file_descriptor, buf, count, bytes_read);

    return bytes_read;
}

// Обёртка для lseek
off_t lseek(int file_descriptor, off_t offset, int whence) {
    // Получаем указатель на оригинальную функцию lseek
    static off_t (*original_lseek)(int, off_t, int) = NULL;
    // Получаем указатель на оригинальную функцию lseej
    if (!original_lseek) {
        // RTLD_NEXT значит: получить lseek который был бы вызван,
        // если бы не было обёртки
        original_lseek = dlsym(RTLD_NEXT, "lseek");
        if (!original_lseek) {
            create_log(FAIL_LSEEK, LOG_LSEEK, file_descriptor, offset, whence, -1);
            exit(1);
        }
    }
    // Вызываем оригинальную функцию lseek
    off_t resulted_offset = original_lseek(file_descriptor, offset, whence);

    create_log(SUCCESS, LOG_LSEEK, file_descriptor, offset, whence, resulted_offset);

    return resulted_offset;

}