#include <stdio.h>
#include <stdlib.h> // atoi()
#include <fcntl.h> 
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define SHM_NAME "/log_shm"
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


// Перечисление для типов логов
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
    int head;
    int tail;
    struct LogEntry logs[100];
} LogShm;

int running = 1;

void format_log_entry(struct LogEntry *log, char *buffer, size_t buf_size) {
    long sec = log->timestamp.tv_sec;
    long msec = log->timestamp.tv_nsec / 1000000;

    switch (log->logType) {
        case LOG_OPEN:
            snprintf(buffer, buf_size, "[%ld.%03ld] %s: filename='%s', flags=%d, fd=%d, status=%d\n",
                    sec, msec,
                    log->function, log->open.filename, log->open.flags, 
                    log->open.file_descriptor, log->status);
            break;
        case LOG_CLOSE:
            snprintf(buffer, buf_size, "[%ld.%03ld] %s: fd=%d, ret=%d, status=%d\n",
                    sec, msec,
                    log->function, log->close.file_descriptor, 
                    log->close.return_code, log->status);
            break;
        case LOG_LSEEK:
            snprintf(buffer, buf_size, "[%ld.%03ld] %s: fd=%d, req_offset=%ld, whence=%d, res_offset=%ld, status=%d\n",
                    sec, msec,
                    log->function, log->lseek.file_descriptor,
                    (long)log->lseek.requested_offset, log->lseek.whence,
                    (long)log->lseek.resulted_offset, log->status);
            break;
        case LOG_READ:
            snprintf(buffer, buf_size, "[%ld.%03ld] %s: fd=%d, buf=%p, count=%zu, bytes_read=%zd, status=%d\n",
                    sec, msec,
                    log->function, log->read.file_descriptor,
                    log->read.buffer_pointer, log->read.count, 
                    log->read.bytes_read, log->status);
            break;
        case LOG_WRITE:
            snprintf(buffer, buf_size, "[%ld.%03ld] %s: fd=%d, buf=%p, count=%zu, bytes_written=%zd, status=%d\n",
                    sec, msec,
                    log->function, log->write.file_descriptor,
                    log->write.buffer_pointer, log->write.count, 
                    log->write.bytes_written, log->status);
            break;
        case LOG_MALLOC:
            snprintf(buffer, buf_size, "[%ld.%03ld] %s: bytes=%zu, ptr=%p, status=%d\n",
                    sec, msec,
                    log->function, log->malloc.bytes_requested, 
                    log->malloc.new_mem_pointer, log->status);
            break;
        case LOG_REALLOC:
            snprintf(buffer, buf_size, "[%ld.%03ld] %s: bytes=%zu, old_ptr=%p, new_ptr=%p, status=%d\n",
                    sec, msec,
                    log->function, log->realloc.bytes_requested,
                    log->realloc.current_mem_pointer, log->realloc.new_mem_pointer,
                    log->status);
            break;
        case LOG_FREE:
            snprintf(buffer, buf_size, "[%ld.%03ld] %s: ptr=%p, status=%d\n",
                    sec, msec,
                    log->function, log->free.mem_pointer, log->status);
            break;
        default:
            snprintf(buffer, buf_size, "[%ld.%03ld] Unknown log type, status=%d\n", 
                    sec, msec, log->status);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <output_file> <ms>\n", argv[0]);
        return 1;
    }
    shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Установить размер и инициализировать структуру
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    LogShm *shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    // Инициализация структуры
    shm->head = 0;
    shm->tail = 0;

    // Открываем файл для записи
    FILE *output = fopen(argv[1], "a");
    if (!output) {
        perror("fopen");
        munmap(shm, SHM_SIZE);
        close(shm_fd);
        return 1;
    }

    printf("Loger demon start %s\n", argv[1]);

    // Создаем поток для обработки ввода

    char buffer[1024];
    int last_pos = shm->tail;

    // Основной цикл демона
    while (running) {
        // Если есть новые логи
        while (last_pos != shm->head && running) {
            format_log_entry(&shm->logs[last_pos], buffer, sizeof(buffer));
            
            // Записываем в файл
            fputs(buffer, output);
            fflush(output);
            
            // Переходим к следующей записи
            last_pos = (last_pos + 1) % 100;
        }
        
        // Небольшая пауза, чтобы не нагружать CPU
        usleep(atoi(argv[2]) * 1000); // argv[2] ms
    }
    

    // Завершаем работу
    fclose(output);
    munmap(shm, SHM_SIZE);
    close(shm_fd);
    // Полностью очищаем SHM
    shm_unlink(SHM_NAME);
    
    printf("Demon loger final\n");
    return 0;
}