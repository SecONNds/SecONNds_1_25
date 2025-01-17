set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

macro (add_network_OT _name)
  add_executable(${_name}-SCI_OT "main_${_name}.cpp")
  target_link_libraries(${_name}-SCI_OT SCI-OT) 
  target_compile_definitions(${_name}-SCI_OT PUBLIC SCI_OT=1 USE_CHEETAH=0
    VERIFY_LAYERWISE=${VERIFY_LAYERWISE} HE_DEBUG=${TRACK_HE_NOISE} 
    MILL_PRINT_COMP=${MILL_PRINT_COMP} MILL_PRINT_COMM=${MILL_PRINT_COMM} MILL_PRINT_TIME=${MILL_PRINT_TIME}
    )
endmacro()

macro (add_network_HE _name)
  add_executable(${_name}-SCI_HE "main_${_name}.cpp")
  target_link_libraries(${_name}-SCI_HE SCI-HE) 
  message(STATUS "Adding ${_name}-SCI_HE with RUN_TRIP_TRIALS=${RUN_TRIP_TRIALS}")
  target_compile_definitions(${_name}-SCI_HE PUBLIC SCI-OT=0 USE_CHEETAH=0
    RUN_TRIP_TRIALS=${RUN_TRIP_TRIALS}
    VERIFY_LAYERWISE=${VERIFY_LAYERWISE} HE_DEBUG=${TRACK_HE_NOISE} 
    MILL_PRINT_COMP=${MILL_PRINT_COMP} MILL_PRINT_COMM=${MILL_PRINT_COMM} MILL_PRINT_TIME=${MILL_PRINT_TIME}
    )
endmacro()

# Networks
add_network_HE(sqnet)
add_network_HE(resnet50)
add_network_HE(densenet121)
