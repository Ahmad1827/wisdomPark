file(GLOB_RECURSE CORE_HEADERS "Include/*.h")
file(GLOB_RECURSE CORE_SOURCES "Source/*.cpp")

add_library(StudioCore STATIC ${CORE_HEADERS} ${CORE_SOURCES})

target_include_directories(StudioCore PUBLIC Include)

# Add SFML linkage so StudioCore can find SFML/Graphics/Image.hpp
target_link_libraries(StudioCore PUBLIC 
    sfml-graphics 
    sfml-window 
    sfml-system
)