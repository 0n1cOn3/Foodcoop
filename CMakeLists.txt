cmake_minimum_required(VERSION 3.16)
project(Foodcoop VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql Charts)
find_package(Qt6 QUIET COMPONENTS WebEngineCore WebEngineQuick)

if(TARGET Qt6::WebEngineCore AND TARGET Qt6::WebEngineQuick)
    set(HAVE_WEBENGINE TRUE)
else()
    set(HAVE_WEBENGINE FALSE)
endif()

add_executable(Foodcoop
    src/main.cpp
    src/PriceFetcher.cpp
    src/DatabaseManager.cpp
    src/TrendDetector.cpp
    src/PlotWindow.cpp
    src/FirstRunDialog.cpp
)

target_link_libraries(Foodcoop PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Charts
)
if(HAVE_WEBENGINE)
    target_link_libraries(Foodcoop PRIVATE Qt6::WebEngineCore Qt6::WebEngineQuick)
endif()
target_compile_definitions(Foodcoop PRIVATE HAVE_WEBENGINE=$<BOOL:${HAVE_WEBENGINE}>)

install(TARGETS Foodcoop DESTINATION bin)

if(HAVE_WEBENGINE)
    add_executable(fetch_test
        fetch_test.cpp
        src/PriceFetcher.cpp
        src/DatabaseManager.cpp
    )

    target_include_directories(fetch_test PRIVATE src)

    target_link_libraries(fetch_test PRIVATE
        Qt6::Network
        Qt6::Sql
        Qt6::WebEngineCore
        Qt6::WebEngineQuick
    )
    target_compile_definitions(fetch_test PRIVATE HAVE_WEBENGINE)

    install(TARGETS fetch_test DESTINATION bin)
endif()
