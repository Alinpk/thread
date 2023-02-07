function(git_download url local_dir)
    if(NOT EXISTS ${local_dir}/googletest)
        message("cmd is:")
        message("git clone ${url} -b ${branch} ${local_dir}")
        execute_process(COMMAND git clone ${url} ${local_dir}/googletest)
    endif()
endfunction()

set(url "https://github.com/google/googletest.git")
set(DEPEND_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependence")

git_download(${url} ${DEPEND_DIR})

add_subdirectory(${DEPEND_DIR}/googletest)