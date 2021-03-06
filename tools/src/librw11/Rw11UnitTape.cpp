// $Id: Rw11UnitTape.cpp 1186 2019-07-12 17:49:59Z mueller $
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2015-2018 by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
// 
// Revision History: 
// Date         Rev Version  Comment
// 2018-12-19  1090   1.0.3  use RosPrintf(bool)
// 2018-12-09  1080   1.0.2  use HasVirt(); Virt() returns ref
// 2017-04-07   868   1.0.1  Dump(): add detail arg
// 2015-06-04   686   1.0    Initial version
// 2015-05-17   683   0.1    First draft
// ---------------------------------------------------------------------------

/*!
  \brief   Implemenation of Rw11UnitTape.
*/

#include "librtools/Rexception.hpp"

#include "librtools/RosFill.hpp"
#include "librtools/RosPrintf.hpp"

#include "Rw11UnitTape.hpp"

using namespace std;

/*!
  \class Retro::Rw11UnitTape
  \brief FIXME_docs
*/

// all method definitions in namespace Retro
namespace Retro {

//------------------------------------------+-----------------------------------
//! Constructor

Rw11UnitTape::Rw11UnitTape(Rw11Cntl* pcntl, size_t index)
  : Rw11UnitVirt<Rw11VirtTape>(pcntl, index),
    fType(),
    fEnabled(false),
    fWProt(false),
    fCapacity(0)
{}

//------------------------------------------+-----------------------------------
//! Destructor

Rw11UnitTape::~Rw11UnitTape()
{}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTape::SetType(const std::string& /*type*/)
{
  throw Rexception("Rw11UnitTape::SetType", 
                   string("Bad args: only type '") + fType + "' supported");
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTape::SetWProt(bool wprot)
{
  if (HasVirt()) throw Rexception("Rw11UnitTape::SetWProt",
                                  "not allowed when tape attached");
  fWProt = wprot;
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTape::SetCapacity(size_t nbyte)
{
  if (HasVirt()) throw Rexception("Rw11UnitTape::SetCapacity",
                                  "not allowed when tape attached");
  fCapacity = nbyte;
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTape::SetPosFile(int posfile)
{
  if (!HasVirt()) throw Rexception("Rw11UnitTape::SetPosFile", 
                                   "no tape attached");
  Virt().SetPosFile(posfile);
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTape::SetPosRecord(int posrec)
{
  if (!HasVirt()) throw Rexception("Rw11UnitTape::SetPosRecord", 
                                   "no tape attached");
  Virt().SetPosRecord(posrec);
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTape::Bot() const
{
  if (!HasVirt()) return false;
  return Virt().Bot();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTape::Eot() const
{
  if (!HasVirt()) return false;
  return Virt().Eot();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTape::Eom() const
{
  if (!HasVirt()) return false;
  return Virt().Eom();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

int Rw11UnitTape::PosFile() const
{
  if (!HasVirt()) return -1;
  return Virt().PosFile();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

int Rw11UnitTape::PosRecord() const
{
  if (!HasVirt()) return -1;
  return Virt().PosRecord();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTape::VirtReadRecord(size_t nbyte, uint8_t* data, size_t& ndone, 
                                  int& opcode, RerrMsg& emsg)
{
  if (!HasVirt()) {
    emsg.Init("Rw11UnitTape::VirtReadRecord", "no tape attached");
    return false;
  }
  return Virt().ReadRecord(nbyte, data, ndone, opcode, emsg);
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTape::VirtWriteRecord(size_t nbyte, const uint8_t* data, 
                                   int& opcode, RerrMsg& emsg)
{
  if (!HasVirt()) {
    emsg.Init("Rw11UnitTape::VirtWriteRecord", "no tape attached");
    return false;
  }
  return Virt().WriteRecord(nbyte, data, opcode, emsg);
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTape::VirtWriteEof(RerrMsg& emsg)
{
  if (!HasVirt()) {
    emsg.Init("Rw11UnitTape::VirtWriteEof", "no tape attached");
    return false;
  }
  return Virt().WriteEof(emsg);
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTape::VirtSpaceForw(size_t nrec, size_t& ndone, 
                                 int& opcode, RerrMsg& emsg)
{
  if (!HasVirt()) {
    emsg.Init("Rw11UnitTape::VirtSpaceForw", "no tape attached");
    return false;
  }
  return Virt().SpaceForw(nrec, ndone, opcode, emsg);
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTape::VirtSpaceBack(size_t nrec, size_t& ndone, 
                                 int& opcode, RerrMsg& emsg)
{
  if (!HasVirt()) {
    emsg.Init("Rw11UnitTape::VirtSpaceBack", "no tape attached");
    return false;
  }
  return Virt().SpaceBack(nrec, ndone, opcode, emsg);
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTape::VirtRewind(int& opcode, RerrMsg& emsg)
{
  if (!HasVirt()) {
    emsg.Init("Rw11UnitTape::VirtRewind", "no tape attached");
    return false;
  }
  return Virt().Rewind(opcode, emsg);
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTape::Dump(std::ostream& os, int ind, const char* text,
                        int detail) const
{
  RosFill bl(ind);
  os << bl << (text?text:"--") << "Rw11UnitTape @ " << this << endl;
  os << bl << "  fType:           " << fType  << endl;
  os << bl << "  fEnabled:        " << RosPrintf(fEnabled) << endl;
  os << bl << "  fWProt:          " << RosPrintf(fWProt) << endl;
  os << bl << "  fCapacity:       " << fCapacity << endl;

  Rw11UnitVirt<Rw11VirtTape>::Dump(os, ind, " ^", detail);
  return;
} 


} // end namespace Retro
