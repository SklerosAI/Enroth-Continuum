cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(MEDIA_SOURCES
        AudioBaseDataSource.cpp
        AudioBufferDataSource.cpp
        FFmpegBlobIoContext.cpp
        FFmpegLogProxy.cpp
        FFmpegLogSource.cpp
        MediaPlayer.cpp)

set(MEDIA_HEADERS
        AudioBaseDataSource.h
        AudioBufferDataSource.h
        AudioDataSource.h
        AudioSample.h
        AudioTrack.h
        FFmpegBlobIoContext.h
        FFmpegLogProxy.h
        FFmpegLogSource.h
        MM6MovieLookup.h
        MediaPlayer.h
        Movie.h
        VideoDataSource.h)

add_library(media STATIC ${MEDIA_SOURCES} ${MEDIA_HEADERS})
target_link_libraries(media PRIVATE FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil FFmpeg::swscale FFmpeg::swresample)
target_link_libraries(media PUBLIC media_audio library_logger library_vid utility application)

target_check_style(media)

add_subdirectory(Audio)

if(OE_BUILD_TESTS)
    add_library(test_media_mm6_movie OBJECT Tests/MM6MovieLookup_ut.cpp)
    target_link_libraries(test_media_mm6_movie PUBLIC testing_unit library_vid)
    target_check_style(test_media_mm6_movie)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_media_mm6_movie)
endif()
