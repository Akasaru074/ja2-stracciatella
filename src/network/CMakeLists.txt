target_sources(${JA2_BINARY} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/TacticalNetwork.cc
)

add_executable(ReplayTest
    ${CMAKE_CURRENT_LIST_DIR}/ReplayTest.cc
)
