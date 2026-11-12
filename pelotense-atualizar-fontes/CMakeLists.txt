cmake_minimum_required(VERSION 3.16)

project(PelotenseAtualizarFontes VERSION 1.0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

include_directories(
    C:/obs-deps/include
    C:/obs-deps/obs-deps-2025-08-23-x64/include
    C:/obs-deps/obs-deps-qt6-2025-08-23-x64/include
    C:/obs-deps/obs-deps-qt6-2025-08-23-x64/include/QtCore
    C:/obs-deps/obs-deps-qt6-2025-08-23-x64/include/QtGui
    C:/obs-deps/obs-deps-qt6-2025-08-23-x64/include/QtWidgets
    C:/obs-deps/obs-deps-qt6-2025-08-23-x64/mkspecs/win32-msvc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(PelotenseAtualizarFontes MODULE
    src/plugin-main.c
    src/plugin-support.c
    src/pelotense-atualizar-fontes.cpp
)

target_link_libraries(PelotenseAtualizarFontes PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
    C:/obs-deps/lib/obs.lib
    C:/obs-deps/lib/obs-frontend-api.lib
    C:/obs-deps/lib/w32-pthreads.lib
    ole32.lib oleaut32.lib uuid.lib shell32.lib user32.lib gdi32.lib advapi32.lib
    mpr.lib userenv.lib d3d11.lib dxgi.lib dxguid.lib d3d12.lib kernel32.lib winspool.lib comdlg32.lib
)

set_target_properties(PelotenseAtualizarFontes PROPERTIES
    PREFIX ""
    OUTPUT_NAME PelotenseAtualizarFontes
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

if(MSVC)
    target_compile_options(PelotenseAtualizarFontes PRIVATE /utf-8 /EHsc /MD /O2 /Ob1)
    target_compile_definitions(PelotenseAtualizarFontes PRIVATE _UNICODE UNICODE WIN32 _WINDOWS HAVE_OBSCONFIG_H QT_CORE_LIB QT_NO_DEBUG _ENABLE_EXTENDED_ALIGNED_STORAGE WIN64 _WIN64 QT_WIDGETS_LIB QT_GUI_LIB NDEBUG)
    target_link_options(PelotenseAtualizarFontes PRIVATE /DLL /INCREMENTAL:NO /NOLOGO /MANIFEST /SUBSYSTEM:CONSOLE /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /MACHINE:X64)
endif()
