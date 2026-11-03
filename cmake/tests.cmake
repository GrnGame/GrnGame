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

function(grn_add_test_target name)
    set(tgt "tests_${name}")
    
    #test only on desktop 
    if (TARGET ${tgt}_prepare OR NOT IS_DESKTOP)
        message(STATUS "You need to be on desktop to run tests!")
        return()
    endif ()

    set(test_src_dir "${CMAKE_SOURCE_DIR}/tests/${name}")

    set(deps Runtime)
    if (TARGET Embedded)
        list(APPEND deps Embedded)
    endif ()

    grn_pipeline_prepare_command(${name} prepare_command)
    grn_pipeline_clean_command(${name} clean_command)


    add_custom_target(${tgt}_prepare
            COMMAND ${prepare_command}
            COMMENT "Preparing tests/${name}"
            VERBATIM)
    add_dependencies(${tgt}_prepare ${deps})

    add_custom_target(${tgt}_run
            COMMAND ${CMAKE_COMMAND} -E chdir "${test_src_dir}" "${test_src_dir}/$<TARGET_FILE_NAME:Runtime>"
            COMMENT "Running test: ${name}"
            VERBATIM)
    
    add_dependencies(${tgt}_run ${tgt}_prepare)

    if (clean_command)
        add_custom_target(${tgt}_clean
                COMMAND ${clean_command}
                COMMENT "Cleaning tests/${name}"
                VERBATIM)
        add_dependencies(${tgt}_clean ${tgt}_run)
        
        add_custom_target(${tgt} ALL DEPENDS ${tgt}_clean)
        set_target_properties(${tgt} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    else()
        add_custom_target(${tgt} ALL DEPENDS ${tgt}_run)
        set_target_properties(${tgt} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()

endfunction()

function(grn_add_run_group_target group)
    set(tests ${ARGN})
    set(test_targets)

    foreach (name IN LISTS tests)
        list(APPEND test_targets tests_${name})
    endforeach ()

    if (test_targets)
        add_custom_target(${group} DEPENDS ${test_targets})
    else ()
        add_custom_target(${group} COMMAND ${CMAKE_COMMAND} -E echo "No tests in ${group}")
    endif ()
endfunction()

function(grn_setup_tests)
    cmake_parse_arguments(ARG "" "" "TARGET_TESTS;AUTO_TESTS" ${ARGN})

    foreach (name IN LISTS ARG_TARGET_TESTS ARG_AUTO_TESTS)
        grn_add_test_target(${name})
    endforeach ()

    add_custom_target(tests)
    foreach (name IN LISTS ARG_TARGET_TESTS)
        if (TARGET tests_${name})
            add_dependencies(tests tests_${name})
        endif ()
    endforeach ()

    add_custom_target(auto_tests)
    foreach (name IN LISTS ARG_AUTO_TESTS)
        if (TARGET tests_${name})
            add_dependencies(auto_tests tests_${name})
        endif ()
    endforeach ()

    grn_add_run_group_target(run_tests      ${ARG_TARGET_TESTS})
    grn_add_run_group_target(run_auto_tests ${ARG_AUTO_TESTS})
endfunction()