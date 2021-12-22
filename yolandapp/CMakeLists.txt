file(GLOB sources "*.cpp" "*.c")
file(GLOB headers "*.h")

add_library(yolandapp SHARED ${sources} ${headers})
target_compile_features(yolandapp PUBLIC cxx_std_20)
target_link_options(yolandapp 
    PUBLIC 
        -pthread
)
target_compile_options(yolandapp 
    PUBLIC
        -Wall -Wextra -pedantic
)
target_include_directories(yolandapp PUBLIC ..)

add_subdirectory(chap-27)
add_subdirectory(chap-34)
