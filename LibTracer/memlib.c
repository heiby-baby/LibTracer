#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

// Безопасная функция вывода, не использующая malloc
void safe_print(const char* format, ...) {
    char buffer[128];  // Статический буфер на стеке
    va_list args;

    // Инициализируем va_list
    va_start(args, format);

    // Форматируем строку в буфер
    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    // Завершаем работу с va_list
    va_end(args);

    // Выводим содержимое буфера в stderr (файловый дескриптор 2)
    if (len > 0) {
        write(STDERR_FILENO, buffer, len);
    }
}

// Наша переопределенная функция malloc
void* malloc(size_t size) {
    static void* (*original_malloc)(size_t) = NULL;

    // Получаем указатель на оригинальную функцию malloc
    if (!original_malloc) {
        original_malloc = dlsym(RTLD_NEXT, "malloc");
        if (!original_malloc) {
            safe_print("Ошибка при получении оригинального malloc\n");
            exit(1);
        }
    }

    // Выводим информацию о вызове malloc
    safe_print("Вызван malloc с размером: %zu\n", size);

    // Вызываем оригинальную функцию malloc
    return original_malloc(size);
}

// Наша переопределенная функция free
void free(void* ptr) {
    static void (*original_free)(void*) = NULL;

    // Получаем указатель на оригинальную функцию free
    if (!original_free) {
        original_free = dlsym(RTLD_NEXT, "free");
        if (!original_free) {
            safe_print("Ошибка при получении оригинального free\n");
            exit(1);
        }
    }

    // Выводим информацию о вызове free
    safe_print("Вызван free с адресом: %p\n", ptr);

    // Вызываем оригинальную функцию free
    original_free(ptr);
}

// Наша переопределенная функция realloc
void* realloc(void* ptr, size_t size) {
    static void* (*original_realloc)(void*, size_t) = NULL;

    // Получаем указатель на оригинальную функцию realloc
    if (!original_realloc) {
        original_realloc = dlsym(RTLD_NEXT, "realloc");
        if (!original_realloc) {
            safe_print("Ошибка при получении оригинального realloc\n");
            exit(1);
        }
    }

    // Выводим информацию о вызове realloc
    safe_print("Вызван realloc с адресом: %p и размером: %zu\n", ptr, size);

    // Вызываем оригинальную функцию realloc
    return original_realloc(ptr, size);
}

// Наша переопределенная функция calloc
void* calloc(size_t num, size_t size) {
    static void* (*original_calloc)(size_t, size_t) = NULL;
    // Получаем указатель на оригинальную функцию calloc
    if (!original_calloc) {
        original_calloc = dlsym(RTLD_NEXT, "calloc");
        if (!original_calloc) {
            safe_print("Ошибка при получении оригинального calloc\n");
            exit(1);
        }
    }

    // Выводим информацию о вызове calloc
    safe_print("Вызван calloc с количеством: %zu и размером: %zu\n", num, size);

    // Вызываем оригинальную функцию calloc
    return original_calloc(num, size);
}

