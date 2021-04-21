add_library(stb STATIC 
    stb_impl.cpp
    include/stb_image_write.h
    include/stb_image.h
)

target_compile_features(stb PRIVATE cxx_std_11)

target_include_directories(stb PUBLIC include)