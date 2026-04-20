# Terminal Module CMakeLists.txt
# Builds CSI 2026 Synchronized Output support

cmake_minimum_required(VERSION 3.16)

# Find Qt5 or Qt6
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# Terminal module sources
set(TERMINAL_SOURCES
    TerminalMode.cpp
    SynchronizedBuffer.cpp
    CSIParser.cpp
    TerminalHandler.cpp
)

set(TERMINAL_HEADERS
    TerminalMode.h
    SynchronizedBuffer.h
    CSIParser.h
    TerminalHandler.h
    ../Utility/Spin.h
    ../Utility/CircularBuffer.h
)

# Create static library
add_library(Terminal STATIC ${TERMINAL_SOURCES} ${TERMINAL_HEADERS})

# Set include directories
target_include_directories(Terminal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link Qt
target_link_libraries(Terminal PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)

# Set C++ standard
set_target_properties(Terminal PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Windows specific
if(WIN32)
    target_compile_definitions(Terminal PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Example executable (optional)
option(BUILD_CSI2026_EXAMPLE "Build CSI 2026 example" ON)

if(BUILD_CSI2026_EXAMPLE)
    add_executable(csi2026_example
        example_csi2026.cpp
    )

    target_link_libraries(csi2026_example PRIVATE
        Terminal
        Qt${QT_VERSION_MAJOR}::Core
    )

    set_target_properties(csi2026_example PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
endif()

# Installation
install(TARGETS Terminal
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${TERMINAL_HEADERS}
    DESTINATION include/Terminal
)
