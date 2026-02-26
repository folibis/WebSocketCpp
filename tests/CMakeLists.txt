include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.0
    GIT_SHALLOW TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(WebSocketCppTest websocketcpp_test.cpp)
add_test(NAME WebSocketCppTest COMMAND WebSocketCppTest)
target_link_libraries(WebSocketCppTest PRIVATE websocketcpp gtest_main)

add_executable(WebSocketCppUrlTest websocketcpp_url_test.cpp)
add_test(NAME WebSocketCppUrlTest COMMAND WebSocketCppUrlTest)
target_link_libraries(WebSocketCppUrlTest PRIVATE websocketcpp gtest_main)

add_executable(WebSocketCppRequestTest websocketcpp_request_test.cpp)
add_test(NAME WebSocketCppRequestTest COMMAND WebSocketCppRequestTest)
target_link_libraries(WebSocketCppRequestTest PRIVATE websocketcpp gtest_main)

add_executable(WebSocketCppRouteTest websocketcpp_route_test.cpp)
add_test(NAME WebSocketCppRouteTest COMMAND WebSocketCppRouteTest)
target_link_libraries(WebSocketCppRouteTest PRIVATE websocketcpp gtest_main)

