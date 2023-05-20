project(cpuloanpool)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( cpuloanpool cpuloanpool cpuloanpool.cpp )
target_include_directories( cpuloanpool PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( cpuloanpool ${CMAKE_SOURCE_DIR}/../ricardian )