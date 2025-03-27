# LibTracer

Библиотека для трассировки вызовов стандартных функций libc через механизм `LD_PRELOAD` с использованием разделяемой памяти.

## Содержание
- [Описание](#описание)
- [Особенности](#особенности)
- [Сборка](#сборка)
- [Использование](#использование)
- [Примеры](#примеры)
- [Ограничения](#ограничения)
- [Структура проекта](#структура-проекта)

## Описание

Проект состоит из двух компонентов:
1. **Инжектируемая библиотека** (`injectLib.so`) - перехватывает вызовы функций:
   - Работа с памятью: `malloc`, `realloc`, `free`
   - Работа с файлами: `open`, `close`, `lseek`, `read`, `write`
2. **Демон-логгер** (`logdemon`) - записывает перехваченные вызовы в лог-файл

Компоненты общаются через разделяемую память (shared memory).

## Особенности

- Поддержка выборочного логирования (память/файлы/всё)
- Минимальное влияние на производительность трассируемой программы
- Точные временные метки для каждого вызова
- Кольцевой буфер в разделяемой памяти для эффективной передачи данных

## Сборка

Требования:
- GCC
- Make
- Linux (тестировалось на ядрах 5.x+)

```bash
mkdir build
make
```

## Использование

Запуск демона

```bash
./logdemon [OPTIONS] <log_file> <check_interval_ms>
```

Опции:
1. -m или --memory - логировать только операции с памятью
2. -f или --file - логировать только операции с файлами
3. Без флагов - логировать всё

Примеры запуска

Демон

```bash
cd build
```

```bash
# Логировать всё с интервалом проверки 10мс
./logdemon full.log 10
```
Трассируемая программа

```bash
LD_PRELOAD=./injectLib.so ls
```

## Примеры

Пример лог-файла
```log
[1743058311.433] malloc: bytes=1024, ptr=0x5b48b38fd2a0, status=0
[1743058311.433] malloc: bytes=14, ptr=0x5b48b38fd6b0, status=0
[1743058311.433] open: filename='testfile.txt', flags=577, fd=4, status=0
[1743058311.433] write: fd=4, buf=0x5b48b0f9b015, count=13, bytes_written=13, status=0
[1743058311.433] close: fd=4, ret=0, status=0
[1743058311.433] open: filename='testfile.txt', flags=0, fd=4, status=0
[1743058311.433] read: fd=4, buf=0x5b48b38fd6b0, count=13, bytes_read=13, status=0
[1743058311.433] lseek: fd=4, req_offset=0, whence=0, res_offset=0, status=0
[1743058311.433] read: fd=4, buf=0x5b48b38fd6d0, count=13, bytes_read=13, status=0
[1743058311.433] close: fd=4, ret=0, status=0
[1743058312.433] realloc: bytes=16, old_ptr=0x5b48b38fd6b0, new_ptr=0x5b48b38fd6b0, status=0
[1743058312.433] free: ptr=0x5b48b38fd6b0, status=0
[1743058312.433] free: ptr=0x5b48b38fd6d0, status=0
```

## Ограничения

1) Работает только на Linux
2) Не поддерживается многопоточность в полном объеме

## Структура проекта

```bash
LibTracer/
├── build/          # Собранные файлы
├── src/
│   ├── injectLib.c # Код инжектируемой библиотеки
│   └── logdemon.c  # Код демона-логгера
├── Makefile        # Файл сборки
└── README.md       # Этот файл
```
