
### PLEASE SET PATH OF libgofuncs.so ###
set(libgofuncs_path $ENV{HOME}/prefix/lib/libgofuncs.so)
########################################

set(prerequisites_available "FALSE")

if((EXISTS "${libgofuncs_path}")
    AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gofuncs.h")
    AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gofuncs.api"))
  set(prerequisites_available "TRUE")
else()
  message(STATUS "One or more of nl_solver prerequisites were not found. Utility nl_solver will not be built")
endif()

if(prerequisites_available)
  message(STATUS "Tool nl_solver will be built..")
  set(SOURCE_FILES
      nl_solver.cpp
      GOFuncsMap.h
      ${CMAKE_SOURCE_DIR}/src/Optimizer/NLoptOptimizer.cpp
      )
  add_library(libgofuncs SHARED IMPORTED)
  set_target_properties(libgofuncs PROPERTIES IMPORTED_LOCATION ${libgofuncs_path})
  add_executable(nl_solver ${SOURCE_FILES})
  target_link_libraries(nl_solver libnlopt libgofuncs)
endif()