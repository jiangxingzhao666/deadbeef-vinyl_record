# ================================================================
#  CMake 构建配置 —— DeaDBeeF Vinyl Record 插件
#
#  输出: vinyl_record.dll (Windows) / vinyl_record.so (Linux)
#  依赖: GTK+ 3, Cairo, GdkPixbuf
# ================================================================

cmake_minimum_required(VERSION 3.20)
project(vinyl_record C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---- 头文件搜索路径 ----
include_directories(${CMAKE_SOURCE_DIR}/include)

# ---- 查找 GTK3（含 Cairo/GdkPixbuf 等依赖） ----
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# ---- 插件源文件 ----
set(PLUGIN_SOURCES
        src/MyPlugin.c          # 插件主入口 + 设计模式注册
        src/vinyl_widget.c      # 黑胶唱片绘制控件
        src/album_art.c         # 专辑封面获取
)

add_library(vinyl_record SHARED ${PLUGIN_SOURCES})

# ================================================================
#  平台差异化配置
# ================================================================

if(WIN32)
    # ---- Windows: 输出 vinyl_record.dll ----
    set_target_properties(vinyl_record PROPERTIES
            PREFIX "" SUFFIX ".dll" OUTPUT_NAME "vinyl_record"
    )
    target_compile_definitions(vinyl_record PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_include_directories(vinyl_record PRIVATE ${GTK3_INCLUDE_DIRS})
    # ws2_32: Winsock2（某些 GTK 组件需要）
    # ole32:   COM/OLE（GdkPixbuf 加载某些格式时需要）
    target_link_libraries(vinyl_record ${GTK3_LIBRARIES} ws2_32 ole32)
else()
    # ---- Linux/BSD: 输出 vinyl_record.so ----
    set_target_properties(vinyl_record PROPERTIES
            PREFIX "" SUFFIX ".so" OUTPUT_NAME "vinyl_record"
    )
    target_include_directories(vinyl_record PRIVATE ${GTK3_INCLUDE_DIRS})
    # m: 数学库（sin/cos 等，vinyl_widget 中使用 M_PI）
    target_link_libraries(vinyl_record ${GTK3_LIBRARIES} m)
    target_compile_options(vinyl_record PRIVATE -Wall -Wextra -O2)
endif()

# ---- CLion/IDE 运行占位目标（不参与插件功能） ----
add_executable(run_plugin src/dummy_main.c)
