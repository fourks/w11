// $Id: Rw11Unit.ipp 1186 2019-07-12 17:49:59Z mueller $
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2013-2019 by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
// 
// Revision History: 
// Date         Rev Version  Comment
// 2019-06-07  1160   1.0.2  Stats() not longer const
// 2013-05-01   513   1.0.1  add fAttachOpts, (Set)AttachOpts()
// 2013-03-06   495   1.0    Initial version
// 2013-02-13   488   0.1    First draft
// ---------------------------------------------------------------------------

/*!
  \brief   Implemenation (inline) of Rw11Unit.
*/

// all method definitions in namespace Retro
namespace Retro {

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline size_t Rw11Unit::Index() const
{
  return fIndex;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline std::string Rw11Unit::Name() const
{
  return fpCntlBase->UnitName(fIndex);
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline void Rw11Unit::SetAttachOpts(const std::string& opts)
{
  fAttachOpts = opts;
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline const std::string& Rw11Unit::AttachOpts() const
{
  return fAttachOpts;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline Rw11Cntl& Rw11Unit::CntlBase() const
{
  return *fpCntlBase;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline Rw11Cpu& Rw11Unit::Cpu() const
{
  return fpCntlBase->Cpu();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline Rw11& Rw11Unit::W11() const
{
  return fpCntlBase->W11();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline RlinkServer& Rw11Unit::Server() const
{
  return fpCntlBase->Server();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline RlinkConnect& Rw11Unit::Connect() const
{
  return fpCntlBase->Connect();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline RlogFile& Rw11Unit::LogFile() const
{
  return fpCntlBase->LogFile();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

inline Rstats& Rw11Unit::Stats()
{
  return fStats;
}

} // end namespace Retro
