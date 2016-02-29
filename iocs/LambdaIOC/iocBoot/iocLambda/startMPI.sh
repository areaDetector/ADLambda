#!/bin/tcsh
setenv PATH /local/mpich_exec/bin:/local/qt4.8/bin:${PATH}
cd /local/xpcsMPI/build-xpcsMPI-Desktop-Debug
/local/mpich-install/bin/mpiexec  -n 6 xterm -e ./xpcsMPI --cmd_out_pipe_name "/local/xpcscmdout" --cmd_in_pipe_name "/local/xpcscmdin" --xsize 1556 --ysize 516 --in_q_length 2000 --file_q_length 200
