cmake_minimum_required(VERSION 3.16)

project(ReminderApp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем AUTOMOC и AUTOUIC для обработки макроса Q_OBJECT и .ui файлов
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Добавляем компонент Sql
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    resources.qrc
    app.rc
)

if(WIN32)
    add_executable(ReminderApp WIN32 ${PROJECT_SOURCES})
else()
    add_executable(ReminderApp ${PROJECT_SOURCES})
endif()

# Линкуем библиотеки Widgets и Sql
target_link_libraries(ReminderApp PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
)
