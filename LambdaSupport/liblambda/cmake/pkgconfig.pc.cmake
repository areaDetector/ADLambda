prefix = @CMAKE_INSTALL_PREFIX@
includedir = ${prefix}/include
libdir = ${prefix}/lib

Name: @PROJECT_NAME@
Description:
URL:
Version: @LIBRARY_VERSION@
Requires: fsdetcore
Libs: -L${libdir}  -l@PROJECT_NAME@ @PKG_LIBS@
Cflags: -I${includedir}
