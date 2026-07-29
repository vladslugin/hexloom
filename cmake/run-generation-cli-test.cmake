if(NOT DEFINED HEXLOOM_CLI OR
   NOT DEFINED HEXLOOM_SPEC OR
   NOT DEFINED HEXLOOM_OUTPUT)
    message(FATAL_ERROR "Texture CLI test requires CLI, spec, and output paths")
endif()

file(REMOVE_RECURSE "${HEXLOOM_OUTPUT}")

execute_process(
    COMMAND
        "${HEXLOOM_CLI}"
        generate-textures
        "${HEXLOOM_SPEC}"
        "${HEXLOOM_OUTPUT}"
        42
    RESULT_VARIABLE generation_result
    OUTPUT_VARIABLE generation_output
    ERROR_VARIABLE generation_error
)

if(NOT generation_result EQUAL 0)
    message(
        FATAL_ERROR
        "Texture CLI failed (${generation_result}):\n"
        "${generation_output}\n${generation_error}"
    )
endif()

if(NOT EXISTS "${HEXLOOM_OUTPUT}/artifact.yaml")
    message(FATAL_ERROR "Texture CLI did not create artifact.yaml")
endif()

file(REMOVE_RECURSE "${HEXLOOM_OUTPUT}")
