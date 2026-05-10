cmake_minimum_required(VERSION 3.21)
project(SCEditor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(SCEditor
    src/main.cpp
    src/ScParser.h
    src/ScParser.cpp
    src/ScFile.h
    src/Decompressor.h
    src/Decompressor.cpp
    src/ScCombiner.h
    src/ScCombiner.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ViewerWidget.h
    src/ViewerWidget.cpp
    src/EditorWidget.h
    src/EditorWidget.cpp
    src/PreviewWidget.h
    src/PreviewWidget.cpp
)

target_link_libraries(SCEditor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(ANDROID)
    set_target_properties(SCEditor PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_VERSION_NAME       "1.0.0"
        QT_ANDROID_VERSION_CODE       1
        QT_ANDROID_MIN_SDK_VERSION    26
        QT_ANDROID_TARGET_SDK_VERSION 35
    )

    # Force Qt to deploy all required libraries into the APK
    qt_android_add_apk_target(SCEditor)

    # Enable deployment of Qt libs
    set_property(TARGET SCEditor PROPERTY
        QT_ANDROID_DEPLOYMENT_DEPENDENCIES
        "Qt6Core Qt6Gui Qt6Widgets"
    )
endif()

# Install rules
install(TARGETS SCEditor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION VERSION_GREATER_EQUAL "6.3")
    qt_finalize_executable(SCEditor)
endif()
