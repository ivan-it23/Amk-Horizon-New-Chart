// header.h: включаемый файл для стандартных системных включаемых файлов
// или включаемые файлы для конкретного проекта
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
#ifndef NOMINMAX
#define NOMINMAX                        // Отключаем макросы min/max из Windows
#endif
// Файлы заголовков Windows
#include <windows.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
// Файлы заголовков среды выполнения C
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
