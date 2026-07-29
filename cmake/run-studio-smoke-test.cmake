if(NOT DEFINED HEXLOOM_CLI OR
   NOT DEFINED HEXLOOM_GODOT OR
   NOT DEFINED HEXLOOM_STUDIO_PROJECT OR
   NOT DEFINED HEXLOOM_SPEC OR
   NOT DEFINED HEXLOOM_OUTPUT)
    message(FATAL_ERROR "Studio smoke test requires all Hexloom paths")
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
    file(REMOVE_RECURSE "${HEXLOOM_OUTPUT}")
    message(
        FATAL_ERROR
        "Could not prepare Studio artifact (${generation_result}):\n"
        "${generation_output}\n${generation_error}"
    )
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        -E
        env
        "HEXLOOM_ARTIFACT_DIRECTORY=${HEXLOOM_OUTPUT}"
        "${HEXLOOM_GODOT}"
        --headless
        --path
        "${HEXLOOM_STUDIO_PROJECT}"
        --
        --smoke-test
    RESULT_VARIABLE studio_result
    OUTPUT_VARIABLE studio_output
    ERROR_VARIABLE studio_error
    TIMEOUT 15
)

file(REMOVE_RECURSE "${HEXLOOM_OUTPUT}")

if(NOT studio_result EQUAL 0)
    message(
        FATAL_ERROR
        "Hexloom Studio smoke test failed (${studio_result}):\n"
        "${studio_output}\n${studio_error}"
    )
endif()

if(NOT studio_output MATCHES "HEXLOOM_STUDIO_READY")
    message(
        FATAL_ERROR
        "Hexloom Studio did not report readiness:\n${studio_output}"
    )
endif()

if(NOT studio_output MATCHES "HEXLOOM_STUDIO_ARTIFACT_LOADED")
    message(
        FATAL_ERROR
        "Hexloom Studio did not load the generated artifact:\n${studio_output}"
    )
endif()

if(NOT studio_output MATCHES "HEXLOOM_STUDIO_SELF_CHECKS_PASSED")
    message(
        FATAL_ERROR
        "Hexloom Studio self-checks did not complete:\n${studio_output}"
    )
endif()
