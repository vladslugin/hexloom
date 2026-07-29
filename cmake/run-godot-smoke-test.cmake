if(NOT DEFINED HEXLOOM_CLI OR
   NOT DEFINED HEXLOOM_GODOT OR
   NOT DEFINED HEXLOOM_PROJECT OR
   NOT DEFINED HEXLOOM_SPEC OR
   NOT DEFINED HEXLOOM_OUTPUT)
    message(FATAL_ERROR "Godot smoke test requires all Hexloom paths")
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
        "Could not prepare Godot artifact (${generation_result}):\n"
        "${generation_output}\n${generation_error}"
    )
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        -E
        env
        "HEXLOOM_SPEC_PATH=${HEXLOOM_SPEC}"
        "HEXLOOM_ARTIFACT_DIRECTORY=${HEXLOOM_OUTPUT}"
        "${HEXLOOM_GODOT}"
        --headless
        --path
        "${HEXLOOM_PROJECT}"
        --quit-after
        2
    RESULT_VARIABLE godot_result
    OUTPUT_VARIABLE godot_output
    ERROR_VARIABLE godot_error
)

file(REMOVE_RECURSE "${HEXLOOM_OUTPUT}")

if(NOT godot_result EQUAL 0)
    message(
        FATAL_ERROR
        "Godot artifact smoke test failed (${godot_result}):\n"
        "${godot_output}\n${godot_error}"
    )
endif()

if(NOT godot_output MATCHES "Hexloom texture artifact loaded")
    message(
        FATAL_ERROR
        "Godot did not confirm texture loading:\n${godot_output}"
    )
endif()
