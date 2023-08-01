find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED fuse)

function(target_link_fuse exe_name)
  target_link_libraries(
      ${exe_name}
      PRIVATE
        ${FUSE_LDFLAGS}
  )

  target_compile_options(
      ${exe_name}
      PRIVATE
        ${FUSE_CFLAGS}
        -Wall -Wextra -Werror
  )
endfunction()

add_executable(vsfs
    vsfs.c
    vsfs.h
    fs_ctx.c
    fs_ctx.h
    options.h
    options.c
    bitmap.h
    bitmap.c
    map.h
    map.c
)
target_link_fuse(vsfs)

add_executable(mkfs.vsfs
    mkfs.c
    bitmap.h
    bitmap.c
    map.h
    map.c
)

target_compile_options(
  mkfs.vsfs
  PRIVATE
    -Wall -Wextra -Werror
)
