set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
)
FetchContent_MakeAvailable(Catch2)

include(Catch)