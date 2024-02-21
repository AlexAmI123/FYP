if(NOT EIGEN_NAN_INITIALIZATION_CONDITION)
  set(EIGEN_NAN_INITIALIZATION_CONDITION "$<CONFIG:Debug>")
endif()

add_library(Eigen::Eigen INTERFACE IMPORTED)
set_target_properties(Eigen::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/Eigen"
    INTERFACE_COMPILE_DEFINITIONS "$<${EIGEN_NAN_INITIALIZATION_CONDITION}:EIGEN_INITIALIZE_MATRICES_BY_NAN>")
if(NOT APPLE)
  set_target_properties(Eigen::Eigen PROPERTIES
      INTERFACE_SOURCES "${SIMROBOT_PREFIX}/Util/Eigen/debug/msvc/eigen.natvis")
endif()
