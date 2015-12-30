# $Id: rt11-40_rk_boot.tcl 704 2015-07-25 14:18:03Z mueller $
#
# Setup file for RT-11 V4.0 RK05 based system
#
# Usage:
#   
# console_starter -d DL0 &
# ti_w11 -xxx @rt11-40_rk_boot.tcl     ( -xxx depends on sim or fpga connect)
#

# setup w11 cpu
rutil::dohook "preinithook"
puts [rlw]

# setup tt,lp,pp (single console; enable rx rate limiter on old DEC OS)
rw11::setup_tt "cpu0" ndl 1 dlrlim 5
rw11::setup_lp 
rw11::setup_pp

# mount disks
cpu0rka0 att rtv4_rk.dsk

# and boot
rutil::dohook "preboothook"
rw11::cpumon
rw11::cpucons
cpu0 boot rka0
