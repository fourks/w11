-- $Id: sys_conf_ba3_bram.vhd 1069 2018-11-16 17:11:30Z mueller $
--
-- Copyright 2018- by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
--
-- This program is free software; you may redistribute and/or modify it under
-- the terms of the GNU General Public License as published by the Free
-- Software Foundation, either version 3, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful, but
-- WITHOUT ANY WARRANTY, without even the implied warranty of MERCHANTABILITY
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
-- for complete details.
--
------------------------------------------------------------------------------
-- Package Name:   sys_conf_ba4_msim
-- Description:    Definitions for tb_sramif2migui_core (bawidth=3;btyp=bram)
--
-- Dependencies:   -
-- Tool versions:  viv 2017.2; ghdl 0.34
-- Revision History: 
-- Date         Rev Version  Comment
-- 2018-11-16  1069   1.0    Initial version
------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

package sys_conf is

  -- define constants --------------------------------------------------------
  constant c_btyp_msim : string := "MSIM";
  constant c_btyp_bram : string := "BRAM";
  
  -- configure ---------------------------------------------------------------
  constant sys_conf_mawidth : positive  := 28;
  constant sys_conf_bawidth : positive  :=  3;    --  64 bit data path 
  constant sys_conf_sawidth : positive  := 19;    -- msim memory size 
  constant sys_conf_rawidth : positive  := 19;    -- bram memory size
  constant sys_conf_rdelay  : positive  :=  1;    -- bram read delay
  constant sys_conf_btyp    : string    := c_btyp_bram;
 
end package sys_conf;