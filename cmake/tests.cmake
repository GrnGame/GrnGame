if (IS_DESKTOP)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
endif ()

# one executable per test, built into build/tests/<name>, with its files prepared by a python script
function(grn_add_test_target name)
    set(tgt "tests_${name}")
    if (TARGET ${tgt})
        return() # same test listed in both lists
    endif ()

    set(scripts "${CMAKE_SOURCE_DIR}/scripts/test")
    set(build_dir "${CMAKE_BINARY_DIR}/tests/${name}")

    # same sources as the runtime
    add_executable(${tgt} "${CMAKE_SOURCE_DIR}/runtime/main.c")
    target_link_libraries(${tgt} PRIVATE grngame)
    set_target_properties(${tgt} PROPERTIES
            OUTPUT_NAME "Runtime-${GRNGAME_TAG}"
            RUNTIME_OUTPUT_DIRECTORY "${build_dir}")

    # prepare the test folder before each build of the test:
    #  - dev: link assets, std and data
    #  - embedded: build Assets.pak with the Embedded generator, which is NOT a target of this
    #    configuration (it is built by the dev configuration into GRNGAME_OUTPUT_DIR/embedded)
    if (GRNGAME_EMBED_ASSETS)
        set(prepare_cmd ${Python3_EXECUTABLE} "${scripts}/embedded_pipeline.py"
                "tests/${name}" "${build_dir}" "${CMAKE_SOURCE_DIR}"
                "${GRNGAME_OUTPUT_DIR}/embedded/$<LOWER_CASE:$<CONFIG>>")
    else ()
        set(prepare_cmd ${Python3_EXECUTABLE} "${scripts}/dev_pipeline.py"
                "tests/${name}" "${build_dir}")
    endif ()

    add_custom_target(${tgt}_prepare COMMAND ${prepare_cmd} VERBATIM)
    add_dependencies(${tgt} ${tgt}_prepare)

    add_test(NAME ${tgt} COMMAND "$<TARGET_FILE:${tgt}>" WORKING_DIRECTORY "${build_dir}")
endfunction()

# runs the given tests one after the other, in listed order
function(grn_add_group_target group)
    set(commands)
    set(exes)
    foreach (name IN LISTS ARGN)
        list(APPEND exes tests_${name})
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E chdir
                "${CMAKE_BINARY_DIR}/tests/${name}" "$<TARGET_FILE:tests_${name}>")
    endforeach ()

    if (commands)
        add_custom_target(${group} ${commands} USES_TERMINAL VERBATIM)
        add_dependencies(${group} ${exes})
    else ()
        add_custom_target(${group} COMMAND ${CMAKE_COMMAND} -E echo "No tests in ${group}")
    endif ()
endfunction()

function(grn_setup_tests)
    if (NOT IS_DESKTOP)
        return()
    endif ()
    cmake_parse_arguments(ARG "" "" "TARGET_TESTS;AUTO_TESTS" ${ARGN})

    foreach (name IN LISTS ARG_TARGET_TESTS ARG_AUTO_TESTS)
        grn_add_test_target(${name})
    endforeach ()

    # ctest labels, a test listed in both lists gets both (ctest -L auto_tests)
    foreach (name IN LISTS ARG_TARGET_TESTS)
        set_property(TEST tests_${name} APPEND PROPERTY LABELS "target_tests")
    endforeach ()
    foreach (name IN LISTS ARG_AUTO_TESTS)
        set_property(TEST tests_${name} APPEND PROPERTY LABELS "auto_tests")
    endforeach ()

    grn_add_group_target(tests ${ARG_TARGET_TESTS})
    grn_add_group_target(auto_tests ${ARG_AUTO_TESTS})

    # delete every generated test folder (they are recreated by the prepare step)
    add_custom_target(tests_clean_all
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/tests")
endfunction()