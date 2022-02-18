add_custom_Target(CopyCompileCommands ALL
	${CMAKE_COMMAND} -E copy_if_different
		${CMAKE_BINARY_DIR}/compile_commands.json
		${CMAKE_SOURCE_DIR}/compile_commands.json)
