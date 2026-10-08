add_executable(quanta_tests
  test_types.cpp
  test_orders.cpp
  test_traders.cpp
  test_order_book.cpp
  test_matching_engine.cpp
  test_risk.cpp
  test_exchange.cpp
)
target_link_libraries(quanta_tests PRIVATE quanta_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(quanta_tests)