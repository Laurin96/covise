using(FFMPEG)

IF(NOT (${OPENSCENEGRAPH_VERSION} VERSION_LESS "2.9"))
    SET(SDL_BUILDING_LIBRARY ON)
    COVISE_FIND_PACKAGE(SDL)
    IF(SDL_FOUND)
       ADD_DEFINITIONS(-DHAVE_SDL)
       INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
       SET(EXTRA_LIBS ${EXTRA_LIBS} ${SDL_LIBRARY})
    ENDIF()
ENDIF()

SET(HEADERS  StereoVideoPlayer.h FFMPEGVideoPlayer.h )
SET(SOURCES StereoVideoPlayer.cpp FFMPEGVideoPlayer.cpp )

cover_add_plugin(StereoVideoPlayer ${HEADERS} ${SOURCES})

IF(MSVC AND NOT MSVC10)
  ADD_COVISE_COMPILE_FLAGS(StereoVideoPlayer "/Zc:wchar_t-")
ENDIF()

