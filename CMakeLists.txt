cmake_minimum_required(VERSION 3.16)
project(FoodcoopPriceTracker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql Charts)

add_executable(price_tracker
    src/main.cpp
    src/PriceFetcher.cpp
    src/DatabaseManager.cpp
    src/TrendDetector.cpp
    src/PlotWindow.cpp
)

target_link_libraries(price_tracker PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Charts
)

install(TARGETS price_tracker DESTINATION bin)
