add_library(qfib INTERFACE)
target_include_directories(qfib
	INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_compile_features(qfib INTERFACE cxx_std_11)
target_link_libraries(qfib INTERFACE glm)#add glm
