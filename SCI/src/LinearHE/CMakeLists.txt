add_library(SCI-LinearHE
    conv-field.cpp
    fc-field.cpp
    elemwise-prod-field.cpp
    utils-HE.cpp
    utils-vec.cpp
)

target_link_libraries(SCI-LinearHE
    PUBLIC
    SCI-utils
    SEAL::seal
    OpenMP::OpenMP_CXX
    Eigen3::Eigen
)

target_compile_definitions(SCI-LinearHE
    PUBLIC
    HE_DEBUG=${TRACK_HE_NOISE}
)
