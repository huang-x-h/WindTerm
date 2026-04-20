cmake_minimum_required(VERSION 3.16)
project(WindTermCSI2026 VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理 Qt 的 MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt5 或 Qt6
find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Core)
if (NOT QT_FOUND)
    message(STATUS "Qt not found, trying to find Qt5...")
    find_package(Qt5 QUIET COMPONENTS Core)
endif()

if (NOT QT_FOUND)
    message(FATAL_ERROR "Qt not found! Please install Qt5 or Qt6.")
endif()

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

message(STATUS "==========================================")
message(STATUS "WindTerm CSI 2026 Build")
message(STATUS "==========================================")
message(STATUS "Qt Version: ${QT_VERSION_MAJOR}.${QT${QT_VERSION_MAJOR}_VERSION_MINOR}.${Qt${QT_VERSION_MAJOR}_VERSION_PATCH}")
message(STATUS "Qt Path: ${Qt${QT_VERSION_MAJOR}_DIR}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "==========================================")

# 添加 Terminal 模块
add_subdirectory(src/Terminal)

# ===== 示例程序 =====
add_executable(WindTermCSI2026Demo
    src/Terminal/example_csi2026.cpp
)

target_link_libraries(WindTermCSI2026Demo PRIVATE
    Terminal
    Qt${QT_VERSION_MAJOR}::Core
)

set_target_properties(WindTermCSI2026Demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# ===== 测试程序 =====
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/Terminal/test_csi2026.cpp)
    add_executable(WindTermCSI2026Test
        src/Terminal/test_csi2026.cpp
    )

    target_link_libraries(WindTermCSI2026Test PRIVATE
        Terminal
        Qt${QT_VERSION_MAJOR}::Core
    )

    set_target_properties(WindTermCSI2026Test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
endif()

# Windows 特定配置
if(WIN32)
    # 控制台应用
    set_property(TARGET WindTermCSI2026Demo PROPERTY WIN32_EXECUTABLE FALSE)
    if(TARGET WindTermCSI2026Test)
        set_property(TARGET WindTermCSI2026Test PROPERTY WIN32_EXECUTABLE FALSE)
    endif()

    # 静态链接选项 (可选)
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

    # 部署帮助目标
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        PATHS ${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin)

    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")

        add_custom_target(deploy
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/deploy
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:WindTermCSI2026Demo> ${CMAKE_BINARY_DIR}/deploy/
            COMMAND ${WINDEPLOYQT_EXECUTABLE} ${CMAKE_BINARY_DIR}/deploy/WindTermCSI2026Demo.exe --release --no-translations
            COMMENT "Deploying Qt dependencies..."
        )
    endif()
endif()

# macOS 特定配置
if(APPLE)
    set_target_properties(WindTermCSI2026Demo PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    )

    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
        PATHS ${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin)

    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_target(deploy
            COMMAND ${MACDEPLOYQT_EXECUTABLE} $<TARGET_FILE:WindTermCSI2026Demo>.app
            COMMENT "Deploying Qt dependencies..."
        )
    endif()
endif()

# Linux 特定配置
if(UNIX AND NOT APPLE)
    # 设置 RPATH
    set_target_properties(WindTermCSI2026Demo PROPERTIES
        INSTALL_RPATH "$ORIGIN/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# 安装目标
install(TARGETS WindTermCSI2026Demo
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION . COMPONENT Runtime
)

if(TARGET WindTermCSI2026Test)
    install(TARGETS WindTermCSI2026Test
        RUNTIME DESTINATION bin
    )
endif()

# 安装文档
install(FILES
    README.md
    INTEGRATION_QUICK.md
    BUILD_GUIDE.md
    DESTINATION .
)

# 打印总结
message(STATUS "")
message(STATUS "Targets:")
message(STATUS "  - WindTermCSI2026Demo: Example application")
if(TARGET WindTermCSI2026Test)
    message(STATUS "  - WindTermCSI2026Test: Unit tests")
endif()
if(TARGET deploy)
    message(STATUS "  - deploy: Deploy Qt dependencies")
endif()
message(STATUS "")
