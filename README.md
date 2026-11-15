# SCAPMS - Smart Clinic Appointment & Patient Management System

COS202 Programming II lab project. A C++17 / Qt 6 desktop application for managing
patients, appointments, a waiting queue, searching, sorting, undo and reports.

## Build (Windows, MSYS2 UCRT64)

```
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
build\scapms.exe
```

In the app choose **File > Load** and open `sample_clinic_data.csv`.

## Structure

- `model/` - patient classes, appointments, database
- `controller/` - ClinicManager, sorting, searching, queue, cache, undo, reports
- `utils/` - validation, file handling, IDs, exceptions
- `gui/` - Qt windows and panels