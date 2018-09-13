cmake_minimum_required(VERSION 2.8)

project(embr-lib)

set(ROOT_DIR ..)
set(EXT_DIR ${ROOT_DIR}/ext)
set(ESTDLIB_DIR ${EXT_DIR}/estdlib/src)

include_directories(${ESTDLIB_DIR})

# libs don't add subdirectories themselves, since it collides
# with an add_subdirectory that a consuming app may issue
#add_subdirectory(${ESTDLIB_DIR} estdlib)

set(SOURCE_FILES
    main.cpp

    embr/dataport.h
    embr/dataport.hpp
    embr/datapump.h
    embr/datapump.hpp
    embr/events.h

    embr/exp/netbuf-alloc.h

    embr/netbuf.h
    embr/netbuf-static.h
    embr/netbuf-dynamic.h
    embr/netbuf-reader.h
    embr/netbuf-writer.h

    embr/observer.h


    embr/streambuf.h
    embr/transport-descriptor.h
    )

add_library(${PROJECT_NAME} ${SOURCE_FILES})
