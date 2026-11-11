execute_process(
    COMMAND "${PROGRAM}" --scenario "${SCENARIO}" --days 3
    OUTPUT_VARIABLE output
    ERROR_VARIABLE errors
    RESULT_VARIABLE status
    TIMEOUT 20
)
if(NOT status STREQUAL "0" OR NOT errors STREQUAL "")
    message(FATAL_ERROR "CLI failed (${status}): ${errors}")
endif()

string(REGEX MATCHALL "物価指数[^:]*: [0-9.]+" records "${output}")
set(values "")
foreach(record IN LISTS records)
    string(REGEX REPLACE ".*: " "" value "${record}")
    list(APPEND values "${value}")
endforeach()
if(SCENARIO STREQUAL "balanced")
    set(expected "100;100;100")
elseif(SCENARIO STREQUAL "shortage")
    set(expected "110;120;130")
elseif(SCENARIO STREQUAL "oversupply")
    set(expected "90;80;70")
else()
    message(FATAL_ERROR "Unknown test scenario")
endif()
if(NOT "${values}" STREQUAL "${expected}")
    message(FATAL_ERROR "Unexpected price indices: ${values}, expected ${expected}")
endif()

string(REGEX MATCH "開始時の総資金: ([0-9]+)コイン" initial_match "${output}")
set(initial "${CMAKE_MATCH_1}")
string(REGEX MATCH "終了時の総資金: ([0-9]+)コイン" final_match "${output}")
set(final "${CMAKE_MATCH_1}")
if(initial STREQUAL "" OR NOT initial STREQUAL final)
    message(FATAL_ERROR "Total money changed: ${initial} -> ${final}")
endif()
