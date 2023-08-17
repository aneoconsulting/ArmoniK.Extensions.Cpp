function(setup_options project_name_param)

    file(READ /etc/issue ${PROJECT_NAME}_ETCISSUE_CONTENT)
    string(FIND "${${project_name_param}_ETCISSUE_CONTENT}" "Alpine" IS_ALPINE)

    if(${ARGC} GREATER 1)
        set(extra_param ${ARGV1})
    endif()

    if(MSVC)
        target_compile_options(${project_name_param} PRIVATE /W4)
    else()
        if(CMAKE_BUILD_TYPE MATCHES DEBUG AND IS_ALPINE EQUAL -1)
            target_compile_options(${project_name_param} PRIVATE -Wall -Wextra -Wpedantic -fsanitize=undefined,address ${extra_param})
        else ()
            target_compile_options(${project_name_param} PRIVATE -Wall -Wextra -Wpedantic ${extra_param})
        endif()
    endif()

    if(CMAKE_BUILD_TYPE MATCHES DEBUG AND IS_ALPINE EQUAL -1)
        target_link_options(${project_name_param} PRIVATE -fsanitize=undefined,address ${extra_param})
    endif()
endfunction()
