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
include(GoogleTest)
gtest_discover_tests(WebSocketCppTest DISCOVERY_MODE PRE_TEST)
