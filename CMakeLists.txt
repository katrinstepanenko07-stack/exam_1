cmake_minimum_required(VERSION 3.15)
project(OnlineStore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Используем pkg-config для поиска libpqxx
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

# Ищем libpq (клиентская библиотека PostgreSQL)
find_library(PQ_LIBRARY NAMES pq
        PATHS /opt/homebrew/lib /usr/local/lib)

# Исходные файлы
set(SOURCES
        src/main.cpp
        src/DatabaseConnection.cpp
        src/User.cpp
        src/Order.cpp
        src/Payment.cpp
)

# Создаем исполняемый файл
add_executable(OnlineStore ${SOURCES})

# Подключаем библиотеки
target_link_libraries(OnlineStore
        PRIVATE
        ${LIBPQXX_LIBRARIES}
        ${PQ_LIBRARY}
        pthread
)

# Добавляем пути для заголовков
target_include_directories(OnlineStore
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LIBPQXX_INCLUDE_DIRS}
        /opt/homebrew/include
)

# Для macOS
if(APPLE)
    target_include_directories(OnlineStore PRIVATE /opt/homebrew/opt/libpqxx/include)
    target_link_directories(OnlineStore PRIVATE /opt/homebrew/opt/libpqxx/lib)
endif()



#cmake_minimum_required(VERSION 3.15)
#project(OnlineStore)
#
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)
#
#find_package(PostgreSQL REQUIRED)
#find_package(libpqxx REQUIRED)
#
## Исходные файлы
#set(SOURCES
#        src/main.cpp
#        src/User.cpp
#        src/Order.cpp
#        src/Payment.cpp
#)
#
## Заголовочные файлы
#set(HEADERS
#        include/DatabaseConnection.h
#        include/User.h
#        include/Order.h
#        include/Payment.h
#)
#
#add_executable(OnlineStore ${SOURCES} ${HEADERS})
#
#target_link_libraries(OnlineStore
#        ${PostgreSQL_LIBRARIES}
#        ${libpqxx_LIBRARIES}
#        pthread
#)
#
#target_include_directories(OnlineStore
#        PRIVATE
#        include
#        ${PostgreSQL_INCLUDE_DIRS}
#        ${libpqxx_INCLUDE_DIRS}
#)
#
#if (APPLE)
#    target_include_directories(OnlineStore
#            PRIVATE
#            /usr/local/include
#    )
#    target_link_directories(OnlineStore
#            PRIVATE
#            /usr/local/lib
#    )
#endif()