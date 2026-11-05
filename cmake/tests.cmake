if (IS_DESKTOP)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
endif ()

set(GRN_PIPELINE_SCRIPTS "${CMAKE_SOURCE_DIR}/scripts/test")


function(grn_pipeline_prepare_command name out_var)
    set(test_dir "${CMAKE_SOURCE_DIR}/tests/${name}")

    if (GRNGAME_EMBED_ASSETS)
        set(${out_var}
                ${Python3_EXECUTABLE} "${GRN_PIPELINE_SCRIPTS}/embedded_pipeline.py"
                "${test_dir}"
                "${CMAKE_SOURCE_DIR}"
                "${GRNGAME_OUTPUT_DIR}/embedded/$<LOWER_CASE:$<CONFIG>>"
                "$<TARGET_FILE:Runtime>"
                PARENT_SCOPE)
    else ()
        set(${out_var}
                ${Python3_EXECUTABLE} "${GRN_PIPELINE_SCRIPTS}/dev_pipeline.py"
                "prepare"
                "${test_dir}"
                "${CMAKE_SOURCE_DIR}"
                "$<TARGET_FILE:Runtime>"
                PARENT_SCOPE)
    endif ()
endfunction()

function(grn_pipeline_clean_command name out_var)
    set(test_dir "${CMAKE_SOURCE_DIR}/tests/${name}")

    if (GRNGAME_EMBED_ASSETS)
        set(${out_var}
                ${Python3_EXECUTABLE} "${GRN_PIPELINE_SCRIPTS}/embedded_clean.py"
                "${test_dir}"
                "$<TARGET_FILE:Runtime>"
                PARENT_SCOPE)
    else ()
        set(${out_var}
                ${Python3_EXECUTABLE} "${GRN_PIPELINE_SCRIPTS}/dev_pipeline.py"
                "clean"
                "${test_dir}"
                "$<TARGET_FILE:Runtime>"
                PARENT_SCOPE)
    endif ()
endfunction()

function(grn_cmd_quote out)
    set(s "")
    foreach (a IN LISTS ARGN)
        string(APPEND s "\"${a}\" ")
    endforeach ()
    string(STRIP "${s}" s)
    set(${out} "${s}" PARENT_SCOPE)
endfunction()

function(grn_add_test_target name)
    set(tgt "tests_${name}")

    # test only on desktop
    if (NOT IS_DESKTOP)
        message(STATUS "You need to be on desktop to run tests!")
        return()
    endif ()

    set(test_src_dir "${CMAKE_SOURCE_DIR}/tests/${name}")

    grn_pipeline_prepare_command(${name} prepare_command)
    grn_pipeline_clean_command(${name} clean_command)

    grn_cmd_quote(pre_str ${prepare_command})
    set_property(GLOBAL APPEND PROPERTY GRN_PRE_TEST_CMDS "${pre_str}")
    if (clean_command)
        grn_cmd_quote(post_str ${clean_command})
        set_property(GLOBAL APPEND PROPERTY GRN_POST_TEST_CMDS "${post_str}")
    endif ()

    add_test(NAME ${tgt}_run
            COMMAND "${test_src_dir}/$<TARGET_FILE_NAME:Runtime>"
            WORKING_DIRECTORY "${test_src_dir}")
endfunction()

function(grn_setup_tests)
    if (NOT IS_DESKTOP)
        return()
    endif ()
    cmake_parse_arguments(ARG "" "" "TARGET_TESTS;AUTO_TESTS" ${ARGN})

    set(deps Runtime)
    if (TARGET Embedded)
        list(APPEND deps Embedded)
    endif ()

    if (NOT TARGET build_test_deps)
        add_custom_target(build_test_deps DEPENDS ${deps})
    endif ()

    set(all_tests ${ARG_TARGET_TESTS} ${ARG_AUTO_TESTS})
    if (all_tests)
        list(REMOVE_DUPLICATES all_tests)
    endif ()

    foreach (name IN LISTS all_tests)
        grn_add_test_target(${name})

        set(test_labels "")
        if ("${name}" IN_LIST ARG_TARGET_TESTS)
            list(APPEND test_labels "target_tests")
        endif ()
        if ("${name}" IN_LIST ARG_AUTO_TESTS)
            list(APPEND test_labels "auto_tests")
        endif ()

        set_tests_properties(tests_${name}_run PROPERTIES LABELS "${test_labels}")
    endforeach ()

    get_property(pre_cmds GLOBAL PROPERTY GRN_PRE_TEST_CMDS)
    get_property(post_cmds GLOBAL PROPERTY GRN_POST_TEST_CMDS)

    set(content "set(CTEST_CUSTOM_PRE_TEST\n")
    foreach (c IN LISTS pre_cmds)
        string(APPEND content "  [==[${c}]==]\n")
    endforeach ()
    string(APPEND content ")\n\nset(CTEST_CUSTOM_POST_TEST\n")
    foreach (c IN LISTS post_cmds)
        string(APPEND content "  [==[${c}]==]\n")
    endforeach ()
    string(APPEND content ")\n")
    
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/CTestCustom-$<CONFIG>.cmake" CONTENT "${content}")

    if (CMAKE_CONFIGURATION_TYPES)
        set(ctest_config [=[$ENV{CMAKE_CONFIG_TYPE}]=])
    else ()
        set(ctest_config "${CMAKE_BUILD_TYPE}")
    endif ()
    file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
            "include(\"${CMAKE_BINARY_DIR}/CTestCustom-${ctest_config}.cmake\" OPTIONAL)\n")

    add_custom_target(run_tests COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "target_tests|auto_tests" DEPENDS build_test_deps)
    add_custom_target(run_auto_tests COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "auto_tests" DEPENDS build_test_deps)
endfunction()