# $Id: 211bsd_rp_boot.tcl 689 2015-06-05 14:33:18Z mueller $
#
# Setup file for 211bsd RP06 based system
#
# Usage:
#   
# console_starter -d DL0 &
# console_starter -d DL1 &
# ti_w11 -xxx @211bsd_rp_boot.tcl        ( -xxx depends on sim or fpga connect)
#

# setup w11 cpu
rutil::dohook "preinithook"
puts [rlw]

# setup tt,lp (211bsd uses parity -> use 7 bit mode)
rw11::setup_tt "cpu0" {to7bit 1}
rw11::setup_lp 

# mount disks
cpu0rpa0 set type rp06
cpu0rpa1 set type rp06

cpu0rpa0 att 211bsd_rp.dsk

# and boot
rutil::dohook "preboothook"
rw11::cpumon
rw11::cpucons
cpu0 boot rpa0
