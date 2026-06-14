cmake_minimum_required(VERSION 3.19)
project(stm32_uart_reader LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets SerialPort Charts)

qt_standard_project_setup()

qt_add_executable(stm32_uart_reader
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_link_libraries(stm32_uart_reader
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::SerialPort
        Qt6::Charts
)

include(GNUInstallDirs)

install(TARGETS stm32_uart_reader
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET stm32_uart_reader
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
