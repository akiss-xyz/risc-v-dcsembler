if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES ".*Clang")
	option(ENABLE_COVERAGE "Enable coverage reporting for gcc/clang" FALSE)
endif()

if(ENABLE_COVERAGE)
	message("Enabled coverage reporting.")
	target_compile_options(${PROJECT_NAME} PUBLIC --coverage -O0 -g)
	target_link_libraries(${PROJECT_NAME} PUBLIC --coverage)
endif()

set(SANITIZERS "")

option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" FALSE)
if(ENABLE_SANITIZER_ADDRESS)
	message("Enabled address sanitizer.")
	list(APPEND SANITIZERS "address")
endif()

option(ENABLE_SANITIZER_LEAK "Enable leak sanitizer" FALSE)
if(ENABLE_SANITIZER_LEAK)
	message("Enabled leak sanitizer.")
	list(APPEND SANITIZERS "leak")
endif()

option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" FALSE)
if(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
	message("Enabled UB sanitizer.")
	list(APPEND SANITIZERS "undefined")
endif()

option(ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" FALSE)
if(ENABLE_SANITIZER_MEMORY)
	if(CMAKE_C_COMPILER_ID MATCHES ".*Clang")
		message("Enabled memory sanitizer.")
		if("address" IN_LIST SANITIZERS
		   OR "leak" IN_LIST SANITIZERS)
			message(WARNING "Memory sanitizer does not work with Address and Leak sanitizer enabled")
		else()
			list(APPEND SANITIZERS "memory")
		endif()
	else()
		message(WARNING "Memory sanitizer requested but the C compiler ID doesn't match Clang."
		                " Ignoring. See cmake/Sanitizers.cmake for more info.")
	endif()
endif()

list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)

if(NOT "${LIST_OF_SANITIZERS}" STREQUAL "")
	target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=${LIST_OF_SANITIZERS})
	target_link_libraries(${PROJECT_NAME} PUBLIC -fsanitize=${LIST_OF_SANITIZERS})
endif()
