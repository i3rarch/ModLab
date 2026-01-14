# ModLab

[![Build](https://github.com/i3rarch/ModLab/actions/workflows/build.yml/badge.svg)](https://github.com/i3rarch/ModLab/actions/workflows/build.yml)

Программное обеспечение для управления учебным стендом «Методы цифровой модуляции». Позволяет настраивать параметры передатчика, приемника и генератора помех, анализировать качество радиоканала в реальном времени.

## Скачать

**Готовые сборки доступны в [Releases](https://github.com/i3rarch/ModLab/releases)**

- **Windows:** Скачайте `ModLab-Windows-x64.zip`, распакуйте и запустите `diplom_v0.exe`
- **Linux:** Скачайте `ModLab-Linux-x64.tar.gz`, распакуйте и запустите `./run.sh`

## Возможности

- **Цифровая модуляция:** 2-FSK, GFSK, ASK/OOK, MSK с настраиваемыми параметрами
- **Управление передатчиком:**
  - Настройка несущей частоты, скорости передачи, девиации
  - Регулировка выходной мощности
  - Режимы пакетов: фиксированная/переменная длина
  - Источники данных: текст, счетчик, файл
  - Циклическая передача с настраиваемым периодом
- **Эмуляция помех:** белый шум, синусоидальная помеха с регулировкой мощности
- **Анализ качества связи:**
  - Метрики: RSSI, LQI
  - Подсчет пакетов и ошибок CRC
  - Расчет PER (Packet Error Rate)
- **Логирование** всех событий с сохранением в файл

## Требования

- **Qt 6.2+** (Core, Widgets, SerialPort)
- **C++17** компилятор:
  - Windows: MSVC 2019+, MinGW, Clang
  - Linux: GCC 7+, Clang 5+
  - macOS: Xcode Clang

## Сборка

### Visual Studio (Windows)

```bash
# Откройте diplom_v0.sln
# Нажмите F7 для сборки, F5 для запуска
```

**Требуется:** Qt VS Tools с настроенным путем к Qt

### VS Code (Windows)

```bash
# Проект уже настроен
Ctrl+Shift+B  # Сборка
F5            # Запуск с отладкой
```

### CMake (универсально)

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt6
cmake --build . --config Release
./diplom_v0
```

<details>
<summary>CMakeLists.txt</summary>

```cmake
cmake_minimum_required(VERSION 3.16)
project(diplom_v0 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets SerialPort)

add_executable(${PROJECT_NAME} WIN32
    diplom_v0/main.cpp
    diplom_v0/diplom_v0.cpp
    diplom_v0/diplom_v0.h
    diplom_v0/diplom_v0.ui
    diplom_v0/diplom_v0.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core Qt6::Widgets Qt6::SerialPort
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()
```
</details>

### qmake

```bash
qmake diplom_v0.pro
make  # или nmake/mingw32-make на Windows
./diplom_v0
```

<details>
<summary>diplom_v0.pro</summary>

```qmake
QT += core gui widgets serialport
CONFIG += c++17

TARGET = diplom_v0
TEMPLATE = app

SOURCES += diplom_v0/main.cpp diplom_v0/diplom_v0.cpp
HEADERS += diplom_v0/diplom_v0.h
FORMS += diplom_v0/diplom_v0.ui
RESOURCES += diplom_v0/diplom_v0.qrc
```
</details>

## Структура проекта

```
ModLab/
├── diplom_v0.sln          # Visual Studio solution
├── diplom_v0/             # Исходный код
│   ├── main.cpp
│   ├── diplom_v0.{h,cpp,ui,qrc}
│   └── diplom_v0.vcxproj
├── .vscode/               # Конфигурация VS Code
└── x64/{Debug,Release}/   # Результаты сборки
```

## Использование

1. Подключите устройства (ПРД, ПРМ, генератор помех) к COM-портам
2. Выберите порты в интерфейсе и нажмите "Подключить"
3. Настройте параметры модуляции и частоты
4. Управляйте передачей данных и анализируйте результаты

## Устранение неполадок

**Отсутствуют Qt библиотеки (при запуске собранного вручную):**
```bash
# Windows: используйте windeployqt
windeployqt diplom_v0.exe

# Linux: убедитесь, что Qt в LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/qt/lib:$LD_LIBRARY_PATH
```

**MSBuild не найден (VS Code):**
```bash
# Запустите VS Code из Developer Command Prompt
"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
code .
```

**Qt не найден (CMake):**
```bash
# Укажите путь к Qt
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt6
```

## CI/CD

Проект использует GitHub Actions для автоматической сборки:

- При каждом push в `main`/`master` собираются артефакты для Windows и Linux
- При создании тега `v*` (например, `v1.0.0`) создается релиз с готовыми сборками
- Артефакты доступны в разделе Actions → выбрать workflow → Artifacts

**Создать релиз:**
```bash
git tag v1.0.0
git push origin v1.0.0
```