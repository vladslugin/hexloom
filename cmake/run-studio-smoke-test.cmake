if(NOT DEFINED HEXLOOM_GODOT OR NOT DEFINED HEXLOOM_STUDIO_PROJECT)
    message(FATAL_ERROR "Studio smoke test requires Godot and project paths")
endif()

execute_process(
    COMMAND
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
