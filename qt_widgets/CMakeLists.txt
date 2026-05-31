cmake_minimum_required(VERSION 3.21)

project(library_management_system_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(POSTGRESQL_ROOT "C:/Program Files/PostgreSQL/10" CACHE PATH "PostgreSQL installation root")
find_path(POSTGRESQL_INCLUDE_DIR libpq-fe.h PATHS "${POSTGRESQL_ROOT}/include" REQUIRED)
find_library(POSTGRESQL_LIBRARY libpq PATHS "${POSTGRESQL_ROOT}/lib" REQUIRED)

set(APP_SOURCES
    main.cpp
    MainWindow.cpp
    LibraryRepository.cpp
    QtDbSupport.cpp
    ../DBUtil.cpp
    ../UserDao.cpp
    ../OperationLogDao.cpp
    ../BookDao.cpp
    ../BorrowDao.cpp
)

set(APP_HEADERS
    MainWindow.h
    LibraryRepository.h
    QtDbSupport.h
)

add_executable(library_management_system_qt
    ${APP_SOURCES}
    ${APP_HEADERS}
)

target_include_directories(library_management_system_qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${POSTGRESQL_INCLUDE_DIR}
)

target_link_libraries(library_management_system_qt PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${POSTGRESQL_LIBRARY}
)

if(WIN32)
    add_custom_command(TARGET library_management_system_qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${POSTGRESQL_ROOT}/bin"
            "$<TARGET_FILE_DIR:library_management_system_qt>"
    )
endif()
