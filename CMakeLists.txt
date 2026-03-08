cmake_minimum_required(VERSION 3.16)

project(IC-Astronomy-Capture
    VERSION 0.1.0
    DESCRIPTION "Astronomy camera capture application using IC Imaging Control 4"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package( ic4 CONFIG REQUIRED ) 
set(QT_REQUIRED_TOP_LEVEL ${PROJECT_IS_TOP_LEVEL})

if( NOT TARGET qt6-dialogs )
    add_subdirectory(${IC4_EXAMPLES}common/qt6-dialogs ${CMAKE_BINARY_DIR}/demoapp-qt6-dialogs)
endif()

# find_package(cfitsio CONFIG REQUIRED)
link_directories(${CMAKE_SOURCE_DIR}/lib/cfitsio)


 include(./common/setup_qt.cmake )

# Find Qt
#find_package(Qt6 REQUIRED COMPONENTS
#    Core
#    Gui
#    Widgets
#    LinguistTools
#)


# Translation files
set(TRANSLATION_FILES
    translations/ic_astronomy_de.ts
    translations/ic_astronomy_en.ts
)

qt6_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TRANSLATION_FILES})

#qt_add_lupdate(IC-Astronomy-Capture
#    TS_FILES ${TRANSLATION_FILES}
#)

#qt_add_lrelease(IC-Astronomy-Capture
#    TS_FILES ${TRANSLATION_FILES} 
#    QM_FILES_OUTPUT_VARIABLE QM_FILES
#)

# Sources
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/optionsdialog.cpp
    src/videowidget.cpp
    fits/fitssaver.cpp
)

set(HEADERS
    include/mainwindow.h
    include/optionsdialog.h
    include/videowidget.h
    include/version.h
    fits/fitssaver.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

qt_add_executable(IC-Astronomy-Capture
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
    ${QM_FILES}
)

target_include_directories(IC-Astronomy-Capture PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/lib/cfitsio/include
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(IC-Astronomy-Capture PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ic4::core
    qt6-dialogs
    cfitsio
)


# Install
include(GNUInstallDirs)
install(TARGETS IC-Astronomy-Capture
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${QM_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/IC-Astronomy-Capture/translations
)

if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON )

    ic4_copy_runtime_to_target(${PROJECT_NAME})

    add_windeployqt_custom_command(${PROJECT_NAME}) # Adds a POST_BUILD call to windeployqt.exe 

endif()
