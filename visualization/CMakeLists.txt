file(GLOB_RECURSE source_files *.cpp *.h *.hpp)
QT5_WRAP_CPP(MOC_GENERATED interfacegui/mainwindow.h interfacegui/widgetopengl.h)
QT5_WRAP_UI(UI_GENERATED interfacegui/mainwindow.ui)

# Target to build
add_executable(FiberVisualizer ${source_files} ${UI_GENERATED} ${MOC_GENERATED})
target_compile_features(FiberVisualizer PUBLIC cxx_std_11)
target_include_directories(FiberVisualizer
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR} # ui & moc files will be generated here
)
target_link_libraries(FiberVisualizer
	PUBLIC qfib
	PUBLIC glad
	PUBLIC OpenGL::GL
	PUBLIC OpenGL::GLU
	PUBLIC Qt5::Widgets
	PUBLIC Qt5::OpenGL
	PUBLIC ${CMAKE_THREAD_LIBS_INIT}
)
set_target_properties(FiberVisualizer PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_LIST_DIR}/../bin>)

# Additional flags
if(NOT MSVC)
	target_compile_options(FiberVisualizer PUBLIC -Wall -Wextra)
endif()
