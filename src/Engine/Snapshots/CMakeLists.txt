cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(ENGINE_SERIALIZATION_SOURCES
        ../MM6/MM6Archive.cpp
        CompositeSnapshots.cpp
        EntitySnapshots.cpp
        EnumSnapshots.cpp
        MM6OutdoorSnapshot.cpp
        TableSerialization.cpp)

set(ENGINE_SERIALIZATION_HEADERS
        ../MM6/MM6Archive.h
        CompositeSnapshots.h
        EntitySnapshots.h
        EnumSnapshots.h
        MM6OutdoorSnapshot.h
        TableSerialization.h)

add_library(engine_serialization STATIC ${ENGINE_SERIALIZATION_SOURCES} ${ENGINE_SERIALIZATION_HEADERS})
target_link_libraries(engine_serialization PUBLIC engine library_binary library_snapshots)
target_check_style(engine_serialization)
