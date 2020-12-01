project(example_ff_dh)

include_directories(
  ../../../../src/main/cpp/
  )

add_executable(example_ff_dh
  main.cpp
  )

target_link_libraries(example_ff_dh
  fortissimo
  )

add_executable(dataowner_qs
  quicksort_network_test.cpp
  )
target_link_libraries(dataowner_qs
  fortissimo
  )
add_executable(dealer_qs
  quicksort_dealer.cpp
  )
target_link_libraries(dealer_qs
  fortissimo
  )


add_executable(dataowner_siso
  siso_sort_network_test.cpp
  )
target_link_libraries(dataowner_siso
  fortissimo
  )
add_executable(dealer_siso
  siso_sort_dealer.cpp
  )
target_link_libraries(dealer_siso
  fortissimo
  )
