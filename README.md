# Тестовое задание C3D Labs

## Требования

- Windows 10/11 x64
- Visual Studio 2019 или новее с компонентом C++ (MSVC, x64)
- CMake 3.25 или новее
- Qt 5.15.2, сборка `msvc2019_64` (модули Core, Gui, Widgets, OpenGL)
- SDK C3D Modeler (Kernel) и C3D Vision с действующим лицензионным ключом
- Папка `Dependencies` (см. ниже)

## Папка `Dependencies`

**Без папки `Dependencies` в корне репозитория проект не соберётся.**
В ней лежат заголовки, библиотеки импорта и DLL из SDK C3D, а также исходники VisionKit.


Для сборки и запуска проекта необходимо создать папку Dependencies в корне со следующей структурой:

### Структура

```
Dependencies/
├── Modeler/                    
│   ├── Include/               
│   ├── Debug/
│   │   ├── c3d.lib             
│   │   └── c3d.dll             
│   └── Release/
│       ├── c3d.lib             
│       └── c3d.dll             
│
└── Vision/                     
    ├── Include/                
    ├── Debug/
    │   ├── c3dvision.lib       
    │   └── c3dvision.dll       
    ├── Release/
    │   ├── c3dvision.lib       
    │   └── c3dvision.dll       
    ├── Shared/
    │   ├── readwritelic.h      
    │   └── readwritelic.cpp
    └── VisionKit/              
        ├── CMakeLists.txt
        ├── MbVision/
        └── QtVision/           
```
