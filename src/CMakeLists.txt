set(INSTALL_DEFAULT_BINDIR "bin" CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")

add_executable(renderfall renderfall.c)
install(TARGETS renderfall RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

find_library(FFTW_LIBRARY NAMES fftw3)
set(FFTW_LIBRARIES "${FFTW_LIBRARY}")

find_path(FFTW_INCLUDE_DIR fftw3.h)
include_directories(${FFTW_INCLUDE_DIR})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

LIST(APPEND TOOLS_LINK_LIBS ${FFTW_LIBRARIES} ${PNG_LIBRARY} m)

target_link_libraries(renderfall ${TOOLS_LINK_LIBS})
