file(GLOB_RECURSE UI_HEADERS "Include/*.h")
file(GLOB_RECURSE UI_SOURCES "Source/*.cpp")

# Exclude main.cpp from static library sources
list(FILTER UI_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# 1. Create static library for embeddable UI panel
add_library(SpriteSheetStudioUI STATIC ${UI_HEADERS} ${UI_SOURCES})

target_include_directories(SpriteSheetStudioUI PUBLIC Include)
target_link_libraries(SpriteSheetStudioUI PUBLIC 
    StudioCore 
    sfml-graphics 
    sfml-window 
    sfml-system
)

# 2. Standalone executable
add_executable(SpriteSheetStudioApp Source/main.cpp)
target_link_libraries(SpriteSheetStudioApp PRIVATE SpriteSheetStudioUI)