set(LIBRARY_NAME HDBSCAN)
Set(LINKDEF ${LIBRARY_NAME}LinkDef.h)

set(SRCS
#Put here your sourcefiles
Distance/EuclideanDistance.cpp
Distance/IDistanceCalculator.cpp
Distance/ManhattanDistance.cpp

Hdbscan/hdbscan.cpp

HdbscanStar/cluster.cpp
HdbscanStar/hdbscanAlgorithm.cpp
HdbscanStar/hdbscanConstraint.cpp
HdbscanStar/outlierScore.cpp
HdbscanStar/undirectedGraph.cpp

Runner/hdbscanRunner.cpp
Runner/hdbscanResult.cpp
Runner/hdbscanParameters.cpp

Utils/bitSet.cpp
)

Set(DEPENDENCIES
  FairRoot::FairTools
  ROOT::Core
  ROOT::HistPainter
  ROOT::Eve
  ROOT::Graf
  ROOT::Graf3d
  ROOT::Gui
  ROOT::Hist
)

Set(INCLUDE_DIR
  ${CMAKE_SOURCE_DIR}/HDBSCAN-CPP/Distance
  ${CMAKE_SOURCE_DIR}/HDBSCAN-CPP/HdbscanStar
  ${CMAKE_SOURCE_DIR}/HDBSCAN-CPP/Runner
  ${CMAKE_SOURCE_DIR}/HDBSCAN-CPP/Utils
  ${CMAKE_SOURCE_DIR}/HDBSCAN-CPP/Hdbscan
)

generate_target_and_root_library(${LIBRARY_NAME}
  LINKDEF ${LINKDEF}
  SRCS ${SRCS}
  DEPS_PUBLIC ${DEPENDENCIES}
  INCLUDE_DIR ${INCLUDE_DIR}
  EXT_CPP .cpp
  EXT_H .hpp
  )
