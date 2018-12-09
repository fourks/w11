// $Id: RlinkConnect.cpp 1076 2018-12-02 12:45:49Z mueller $
//
// Copyright 2011-2018 by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
//
// This program is free software; you may redistribute and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY, without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for complete details.
// 
// Revision History: 
// Date         Rev Version  Comment
// 2018-12-01  1076   2.7    use unique_ptr instead of scoped_ptr
// 2018-11-30  1075   2.6.4  use list-init; use range loop
// 2018-10-27  1059   2.6.3  coverity fixup (uncaught exception in dtor)
// 2017-04-22   883   2.6.2  add rbus monitor probe, add HasRbmon()
// 2017-04-07   868   2.6.1  Dump(): add detail arg
// 2017-02-20   854   2.6    use Rtime, drop TimeOfDayAsDouble
// 2016-04-02   758   2.5    add USR_ACCESS register support (RLUA0/RLUA1)
// 2016-03-20   748   2.4    add fTimeout,(Set)Timeout();
// 2015-05-10   678   2.3.1  WaitAttn(): BUGFIX: return 0. (not -1.) if poll
// 2015-04-12   666   2.3    add LinkInit,LinkInitDone; transfer xon
// 2015-04-02   661   2.2    expect logic: stat expect in Command, invert mask
// 2015-01-06   631   2.1    full rlink v4 implementation
// 2014-12-10   611   2.0    re-organize for rlink v4
// 2014-08-26   587   1.5    start accept rlink v4 protocol (partially...)
// 2014-08-15   583   1.4    rb_mreq addr now 16 bit
// 2014-07-27   575   1.3.3  ExecPart(): increase packet tout from 5 to 15 sec
// 2013-04-21   509   1.3.2  add SndAttn() method
// 2013-03-01   493   1.3.1  add Server(Active..|SignalAttn)() methods
// 2013-02-23   492   1.3    use scoped_ptr for Port; Close allways allowed
//                           use RlinkContext, add Context(), Exec(..., cntx)
// 2013-02-22   491   1.2    use new RlogFile/RlogMsg interfaces
// 2013-02-03   481   1.1.2  use Rexception
// 2013-01-13   474   1.1.1  add PollAttn() method
// 2011-04-25   380   1.1    use boost::(mutex&lock), implement Lockable IF
// 2011-04-22   379   1.0.1  add Lock(), Unlock(), lock connect in Exec()
// 2011-04-02   375   1.0    Initial version
// 2011-01-15   356   0.1    First draft
// ---------------------------------------------------------------------------

/*!
  \file
  \brief   Implemenation of RlinkConnect.
*/

#include <iostream>

#include "boost/thread/locks.hpp"

#include "RlinkPortFactory.hpp"
#include "librtools/RosFill.hpp"
#include "librtools/RosPrintf.hpp"
#include "librtools/RosPrintBvi.hpp"
#include "librtools/Rtools.hpp"
#include "librtools/Rexception.hpp"
#include "librtools/RlogMsg.hpp"
#include "RlinkServer.hpp"

#include "RlinkConnect.hpp"

using namespace std;

/*!
  \class Retro::RlinkConnect
  \brief FIXME_docs
*/

// all method definitions in namespace Retro
namespace Retro {

//------------------------------------------+-----------------------------------
// constants definitions

const uint16_t RlinkConnect::kRbaddr_RLCNTL;
const uint16_t RlinkConnect::kRbaddr_RLSTAT;
const uint16_t RlinkConnect::kRbaddr_RLID1;
const uint16_t RlinkConnect::kRbaddr_RLID0;
const uint16_t RlinkConnect::kRbaddr_RLUA1;
const uint16_t RlinkConnect::kRbaddr_RLUA0;
const uint16_t RlinkConnect::kRbaddr_RMBASE;

const uint16_t RlinkConnect::kRLCNTL_M_AnEna;
const uint16_t RlinkConnect::kRLCNTL_M_AtoEna;
const uint16_t RlinkConnect::kRLCNTL_M_AtoVal;

const uint16_t RlinkConnect::kRLSTAT_V_LCmd;
const uint16_t RlinkConnect::kRLSTAT_B_LCmd;
const uint16_t RlinkConnect::kRLSTAT_M_BAbo;
const uint16_t RlinkConnect::kRLSTAT_M_RBSize;

const uint16_t RlinkConnect::kSBCNTL_V_RLMON;
const uint16_t RlinkConnect::kSBCNTL_V_RLBMON;
const uint16_t RlinkConnect::kSBCNTL_V_RBMON;  

const uint16_t RlinkConnect::kRbufBlkDelta;
const uint16_t RlinkConnect::kRbufPrudentDelta;

//------------------------------------------+-----------------------------------
//! Default constructor

RlinkConnect::RlinkConnect()
  : fupPort(),
    fLinkInitDeferred(false),
    fLinkInitDone(false),
    fpServ(nullptr),
    fSeqNumber{},
    fSndPkt(),
    fRcvPkt(),
    fContext(),
    fAddrMap(),
    fStats(),
    fLogBaseAddr(16),                       // addr default radix: hex
    fLogBaseData(16),                       // data default radix: hex
    fLogBaseStat(16),                       // stat default radix: hex
    fPrintLevel(2),                         // default print: error and checks
    fDumpLevel(0),                          // default dump: no
    fTraceLevel(0),                         // default trace: no
    fTimeout(10.),                          // default timeout: 10 sec
    fspLog(new RlogFile(&cout)),
    fConnectMutex(),
    fAttnNotiPatt(0),
    fTsLastAttnNoti(),
    fSysId(0xffffffff),
    fUsrAcc(0x00000000),
    fRbufSize(2048),
    fHasRbmon(false)
{
  fContext.SetStatus(0, RlinkCommand::kStat_M_RbTout |
                        RlinkCommand::kStat_M_RbNak  |
                        RlinkCommand::kStat_M_RbErr);

  // Statistic setup
  fStats.Define(kStatNExec,     "NExec",     "Exec() calls");
  fStats.Define(kStatNExecPart, "NExecPart", "ExecPart() calls");
  fStats.Define(kStatNCmd,      "NCmd",      "commands executed");
  fStats.Define(kStatNRreg,     "NRreg",     "rreg commands");
  fStats.Define(kStatNRblk,     "NRblk",     "rblk commands");
  fStats.Define(kStatNWreg,     "NWreg",     "wreg commands");
  fStats.Define(kStatNWblk,     "NWblk",     "wblk commands");
  fStats.Define(kStatNLabo,     "NLabo",     "labo commands");
  fStats.Define(kStatNAttn,     "NAttn",     "attn commands");
  fStats.Define(kStatNInit,     "NInit",     "init commands");
  fStats.Define(kStatNRblkWord, "NRblkWord", "words rcvd with rblk");
  fStats.Define(kStatNWblkWord, "NWblkWord", "words send with wblk");
  fStats.Define(kStatNExpData,  "NExpData",  "expect for data defined");
  fStats.Define(kStatNExpDone,  "NExpDone",  "expect for done defined");
  fStats.Define(kStatNExpStat,  "NExpStat",  "expect for stat explicit");
  fStats.Define(kStatNNoExpStat,"NNoExpStat","no expect for stat");
  fStats.Define(kStatNChkData,  "NChkData",  "expect data failed");
  fStats.Define(kStatNChkDone,  "NChkDone",  "expect done failed");
  fStats.Define(kStatNChkStat,  "NChkStat",  "expect stat failed");
  fStats.Define(kStatNSndOob,   "NSndOob",   "SndOob() calls");
  fStats.Define(kStatNErrMiss,  "NErrMiss",  "decode: missing data");
  fStats.Define(kStatNErrCmd,   "NErrCmd",   "decode: command mismatch");
  fStats.Define(kStatNErrLen,   "NErrLen",   "decode: length mismatch");
  fStats.Define(kStatNErrCrc,   "NErrCrc",   "decode: crc mismatch");
}

//------------------------------------------+-----------------------------------
//! Destructor

RlinkConnect::~RlinkConnect()
{
  Rtools::Catch2Cerr(__func__, [this](){ Close(); } );
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool RlinkConnect::Open(const std::string& name, RerrMsg& emsg)
{
  Close();

  fupPort = move(RlinkPortFactory::Open(name, emsg));
  if (!fupPort) return false;

  fSndPkt.SetXonEscape(fupPort->XonEnable()); // transfer XON enable

  fupPort->SetLogFile(fspLog);
  fupPort->SetTraceLevel(fTraceLevel);

  fLinkInitDone = false;
  fRbufSize = 2048;                         // use minimum (2kB) as startup
  fSysId    = 0xffffffff;
  fUsrAcc   = 0x00000000;
  
  if (! fupPort->Url().FindOpt("noinit")) {
    if (!LinkInit(emsg)) {
      Close();
      return false;
    }
  }

  return true;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::Close()
{
  if (!fupPort) return;

  if (fpServ) fpServ->Stop();               // stop server in case still running

  if (fupPort->Url().FindOpt("keep")) {
    RerrMsg emsg;
    fSndPkt.SndKeep(fupPort.get(), emsg);
  }

  fupPort.reset();
    
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool RlinkConnect::LinkInit(RerrMsg& emsg)
{
  if (fLinkInitDone) return true;
  
  RlinkCommandList clist;
  int ista = clist.AddRreg(kRbaddr_RLSTAT);
  int iid1 = clist.AddRreg(kRbaddr_RLID1);
  int iid0 = clist.AddRreg(kRbaddr_RLID0);

  // RLUA0/1 are optional registers, available for 7Series and higher
  int iua1 = clist.AddRreg(kRbaddr_RLUA1);
  clist.SetLastExpectStatus(0,0);       // disable stat check
  int iua0 = clist.AddRreg(kRbaddr_RLUA0);
  clist.SetLastExpectStatus(0,0);       // disable stat check
  // probe for rbus monitor
  int irbm = clist.AddRreg(kRbaddr_RMBASE);
  clist.SetLastExpectStatus(0,0);       // disable stat check

  if (!Exec(clist, emsg)) return false;

  fLinkInitDone = true;

  // handle rlink core registers: setup mappings, keep data
  AddrMapInsert("rl.cntl", kRbaddr_RLCNTL);
  AddrMapInsert("rl.stat", kRbaddr_RLSTAT);
  AddrMapInsert("rl.id1",  kRbaddr_RLID1);
  AddrMapInsert("rl.id0",  kRbaddr_RLID0);

  uint16_t rlstat = clist[ista].Data();
  uint16_t rlid1  = clist[iid1].Data();
  uint16_t rlid0  = clist[iid0].Data();

  fRbufSize = size_t(1) << (10 + (rlstat & kRLSTAT_M_RBSize));
  fSysId    = uint32_t(rlid1)<<16 | uint32_t(rlid0);

  // handle rlink optional registers: USR_ACCESS and rbus monitor probe
  const uint8_t staterr = RlinkCommand::kStat_M_RbTout |
                          RlinkCommand::kStat_M_RbNak  |
                          RlinkCommand::kStat_M_RbErr;
  if ((clist[iua1].Status() & staterr) == 0 &&     // RLUA1 ok
      (clist[iua0].Status() & staterr) == 0) {     // RLUA0 ok

    AddrMapInsert("rl.ua1",  kRbaddr_RLUA1);
    AddrMapInsert("rl.ua0",  kRbaddr_RLUA0);

    uint16_t rlua1  = clist[3].Data();
    uint16_t rlua0  = clist[4].Data();
    fUsrAcc   = uint32_t(rlua1)<<16 | uint32_t(rlua0);
  }
  
  if ((clist[irbm].Status() & staterr) == 0) {     // rbus monitor found
    fHasRbmon = true;                                // just remember 
  }
  
  return true;
}
  
//------------------------------------------+-----------------------------------
//! Indicates whether server is active.
/*!
  \returns \c true if server active.
 */


bool RlinkConnect::ServerActive() const
{
  return fpServ && fpServ->IsActive();
}

//------------------------------------------+-----------------------------------
//! Indicates whether server is active and caller is inside server thread.
/*!
  \returns \c true if server active and method is called from server thread.
 */

bool RlinkConnect::ServerActiveInside() const
{
  return fpServ && fpServ->IsActiveInside();
}

//------------------------------------------+-----------------------------------
//! Indicates whether server is active and caller is outside server thread.
/*!
  \returns \c true if server active and method is called from a thread
           other than the server thread.
 */

bool RlinkConnect::ServerActiveOutside() const
{
  return fpServ && fpServ->IsActiveOutside();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::lock()
{
  fConnectMutex.lock();
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool RlinkConnect::try_lock()
{
  return fConnectMutex.try_lock();
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::unlock()
{
  fConnectMutex.unlock();
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool RlinkConnect::Exec(RlinkCommandList& clist, RlinkContext& cntx, 
                        RerrMsg& emsg)
{  
  if (clist.Size() == 0)
    throw Rexception("RlinkConnect::Exec()", "Bad state: clist empty");
  if (! IsOpen())
    throw Rexception("RlinkConnect::Exec()", "Bad state: port not open");

  boost::lock_guard<RlinkConnect> lock(*this);

  fStats.Inc(kStatNExec);

  clist.ClearLaboIndex();

  uint8_t defstatval = cntx.StatusValue();
  uint8_t defstatmsk = cntx.StatusMask();
  size_t size = clist.Size();

  for (size_t i=0; i<size; i++) {
    RlinkCommand& cmd = clist[i];
   if (!cmd.TestFlagAny(RlinkCommand::kFlagInit))
     throw Rexception("RlinkConnect::Exec()", 
                      "BugCheck: command not initialized");
    if (cmd.Command() > RlinkCommand::kCmdInit)
      throw Rexception("RlinkConnect::Exec()", 
                       "BugCheck: invalid command code");
    // trap attn command when server running and outside server thread
    if (cmd.Command() == RlinkCommand::kCmdAttn && ServerActiveOutside())
      throw Rexception("RlinkConnect::Exec()", 
                       "attn command not allowed outside active server");
    
    cmd.ClearFlagBit(RlinkCommand::kFlagSend   | 
                     RlinkCommand::kFlagDone   |
                     RlinkCommand::kFlagLabo   |
                     RlinkCommand::kFlagPktBeg | 
                     RlinkCommand::kFlagPktEnd |
                     RlinkCommand::kFlagErrNak | 
                     RlinkCommand::kFlagErrDec);
    
    // setup default status check unless explicit check defined
    if (!cmd.ExpectStatusSet()) {
      cmd.SetExpectStatusDefault(defstatval, defstatmsk);
    }
  }
  
  // old split volative logic. Currently dormant
  // may be later used for rtbuf size prot
#ifdef NEVER
  while (ibeg < size) {
    size_t iend = ibeg;
    for (size_t i=ibeg; i<size; i++) {
      iend = i;
      if (clist[i].TestFlagAll(RlinkCommand::kFlagVol)) {
        fStats.Inc(kStatNSplitVol);
        break;
      }
    }
    bool rc = ExecPart(clist, ibeg, iend, emsg);
    if (!rc) return rc;
    ibeg = iend+1;
  }
#endif

  bool rc = ExecPart(clist, 0, size-1, emsg);
  if (!rc) return rc;

  bool checkseen = false;
  bool errorseen = false;

  for (size_t i=0; i<size; i++) {
    RlinkCommand& cmd = clist[i];
    
    bool checkfound = cmd.TestFlagAny(RlinkCommand::kFlagChkStat | 
                                      RlinkCommand::kFlagChkData |
                                      RlinkCommand::kFlagChkDone);
    bool errorfound = cmd.TestFlagAny(RlinkCommand::kFlagErrNak | 
                                      RlinkCommand::kFlagErrDec);
    checkseen |= checkfound;
    errorseen |= errorfound;
    if (checkfound | errorfound) cntx.IncErrorCount();
  }

  size_t loglevel = 3;
  if (checkseen) loglevel = 2;
  if (errorseen) loglevel = 1;
  if (loglevel <= fPrintLevel) {
    RlogMsg lmsg(*fspLog);
    clist.Print(lmsg(), &AddrMap(), fLogBaseAddr, fLogBaseData, fLogBaseStat);
  }
  if (loglevel <= fDumpLevel) {
    RlogMsg lmsg(*fspLog);
    clist.Dump(lmsg(), 0);
  }
  
  return true;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::Exec(RlinkCommandList& clist, RlinkContext& cntx)
{
  RerrMsg emsg;
  bool rc = Exec(clist, cntx, emsg);
  if (!rc) {
    RlogMsg lmsg(*fspLog, 'F');
    lmsg << emsg << endl;
    lmsg << "Dump of failed clist:" << endl;
    clist.Dump(lmsg(), 0);    
  }
  if (!rc)
    throw Rexception("RlinkConnect::Exec", "Exec() failed: ", emsg);
  return;
}

//------------------------------------------+-----------------------------------
//! Wait for an attention notify.
/*!
  First checks whether there are received and not yet harvested notifies.
  In that case the cummulative pattern of these pending notifies is returned
  in \a apat, and a 0 return value.

  If a positive \a timeout is specified the method waits this long for a
  valid and non-zero attention notify.

  \param      timeout  maximal time to wait. Must be non-negative.
                       A zero \a timeout can be used to only harvest pending
                       notifies without waiting for new ones.
  \param[out] twait    wait time
  \param[out] apat     cummulative attention pattern
  \param[out] emsg     contains error description (mainly from port layer)

  \returns >=0 on success or a negative value indicating an error:
    -  0  if there was already a received and not yet harvested notify
    -  1  finite duration wait
    - -1  indicates timeout (\a apat will be 0)
    - -2  indicates port IO error (\a emsg will contain information)

  \throws Rexception if called outside of an active server

  \pre ServerActiveOutside() must be \c false.

 */

int RlinkConnect::WaitAttn(const Rtime& timeout, Rtime& twait, 
                           uint16_t& apat, RerrMsg& emsg)
{
  if (ServerActiveOutside())
    throw Rexception("RlinkConnect::WaitAttn()", 
                     "not allowed outside active server");

  apat = 0;
  twait.Clear();

  boost::lock_guard<RlinkConnect> lock(*this);

  // harvest pending notifiers
  if (fAttnNotiPatt != 0) {
    apat = fAttnNotiPatt;
    fAttnNotiPatt = 0;
    return 0;
  }

  // quit if poll only (zero timeout) 
  if (!timeout.IsPositive()) return 0;

  // wait for new notifier
  Rtime tnow(CLOCK_MONOTONIC);
  Rtime tend = tnow + timeout;
  Rtime tbeg = tnow;

  while (tnow < tend) {
    int irc = fRcvPkt.ReadData(fupPort.get(), tend-tnow, emsg);
    if (irc == RlinkPort::kTout) return -1;
    if (irc == RlinkPort::kErr)  return -2;
    tnow.GetClock(CLOCK_MONOTONIC);    
    while (fRcvPkt.ProcessData()) {
      int irc = fRcvPkt.PacketState();
      if (irc == RlinkPacketBufRcv::kPktPend) break;
      if (irc == RlinkPacketBufRcv::kPktAttn) {
        ProcessAttnNotify();
        if (fAttnNotiPatt != 0) {
          apat = fAttnNotiPatt;
          fAttnNotiPatt = 0;
          twait = tnow - tbeg;
          return 1;
        }
      } else {
        RlogMsg lmsg(*fspLog, 'E');
        lmsg << "WaitAttn: dropped spurious packet";
        fRcvPkt.AcceptPacket();
      }

    } // while (fRcvPkt.ProcessData())
  } // while (tnow < tend)

  return -1;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool RlinkConnect::SndOob(uint16_t addr, uint16_t data, RerrMsg& emsg)
{
  boost::lock_guard<RlinkConnect> lock(*this);
  fStats.Inc(kStatNSndOob);
  return fSndPkt.SndOob(fupPort.get(), addr, data, emsg);
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool RlinkConnect::SndAttn(RerrMsg& emsg)
{
  boost::lock_guard<RlinkConnect> lock(*this);
  return fSndPkt.SndAttn(fupPort.get(), emsg);
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::SetLogBaseAddr(uint32_t base)
{
  if (base!=2 && base!=8 && base!=16)
    throw Rexception("RlinkConnect::SetLogBaseAddr()",
                     "Bad args: base != 2,8,16");
  fLogBaseAddr = base;
  return;
}
  
//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::SetLogBaseData(uint32_t base)
{
  if (base!=2 && base!=8 && base!=16)
    throw Rexception("RlinkConnect::SetLogBaseData()",
                     "Bad args: base != 2,8,16");
  fLogBaseData = base;
  return;
}
  
//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::SetLogBaseStat(uint32_t base)
{
  if (base!=2 && base!=8 && base!=16)
    throw Rexception("RlinkConnect::SetLogBaseStat()",
                     "Bad args: base != 2,8,16");
  fLogBaseStat = base;
  return;
}
  
//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::SetPrintLevel(uint32_t lvl)
{
  fPrintLevel = lvl;
  return;
}
  
//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::SetDumpLevel(uint32_t lvl)
{
  fDumpLevel = lvl;
  return;
}
  
//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::SetTraceLevel(uint32_t lvl)
{
  fTraceLevel = lvl;
  if (fupPort) fupPort->SetTraceLevel(lvl);
  return;
}  

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::SetTimeout(const Rtime& timeout)
{
  if (!timeout.IsPositive())
    throw Rexception("RlinkConnect::SetTimeout()",
                     "Bad args: timeout <= 0");
  fTimeout = timeout;
  return;
}
  
//------------------------------------------+-----------------------------------
//! FIXME_docs

bool RlinkConnect::LogOpen(const std::string& name, RerrMsg& emsg)
{
  if (!fspLog->Open(name, emsg)) {
    fspLog->UseStream(&cout);
    return false;
  }
  return true;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::LogUseStream(std::ostream* pstr, const std::string& name)
{
  fspLog->UseStream(pstr, name);
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::SetLogFileName(const std::string& name)
{
  RerrMsg emsg;
  if (!LogOpen(name, emsg)) {
    throw Rexception("RlinkConnect::SetLogFile", 
                     emsg.Text() + "', using stdout");
  }  
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::Print(std::ostream& os) const
{
  os << "RlinkConnect::Print(std::ostream& os)" << endl;
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::Dump(std::ostream& os, int ind, const char* text,
                        int detail) const
{
  RosFill bl(ind);
  os << bl << (text?text:"--") << "RlinkConnect @ " << this << endl;

  if (fupPort) {
    fupPort->Dump(os, ind+2, "fupPort: ", detail);
  } else {
    os << bl << "  fupPort:         " <<  fupPort.get() << endl;
  }

  os << bl << "  fLinkInitDone:   " << fLinkInitDone << endl;

  os << bl << "  fpServ:          " << fpServ << endl;
  os << bl << "  fSeqNumber:      ";
  for (auto& o: fSeqNumber) os << RosPrintBvi(o,16) << " ";
  os << endl;
  
  fSndPkt.Dump(os, ind+2, "fSndPkt: ", detail);
  fRcvPkt.Dump(os, ind+2, "fRcvPkt: ", detail);
  fContext.Dump(os, ind+2, "fContext: ", detail);
  fAddrMap.Dump(os, ind+2, "fAddrMap: ", detail-1);
  fStats.Dump(os, ind+2, "fStats: ", detail-1);
  os << bl << "  fLogBaseAddr:    " << fLogBaseAddr << endl;
  os << bl << "  fLogBaseData:    " << fLogBaseData << endl;
  os << bl << "  fLogBaseStat:    " << fLogBaseStat << endl;
  os << bl << "  fPrintLevel:     " << fPrintLevel << endl;
  os << bl << "  fDumpLevel       " << fDumpLevel << endl;
  os << bl << "  fTraceLevel      " << fTraceLevel << endl;
  fspLog->Dump(os, ind+2, "fspLog: ");
  os << bl << "  fAttnNotiPatt:   " << RosPrintBvi(fAttnNotiPatt,16) << endl;
  os << bl << "  fTsLastAttnNoti: " << fTsLastAttnNoti << endl;
  os << bl << "  fSysId:          " << RosPrintBvi(fSysId,16) << endl;
  os << bl << "  fUsrAcc:         " << RosPrintBvi(fUsrAcc,16) << endl;
  os << bl << "  fRbufSize:       " << RosPrintf(fRbufSize,"d",6) << endl;

  return;
}

//------------------------------------------+-----------------------------------
//! Handle unsolicited data from port.
/*!
  Called by RlinkServer to process unsolicited input data. Will read all
  pending data from input port and process it with ProcessUnsolicitedData().

  \throws Rexception if not called from inside of an active server

  \pre ServerActiveInside() must be \c true.
 */

void RlinkConnect::HandleUnsolicitedData()
{
  if (!ServerActiveInside())
    throw Rexception("RlinkConnect::HandleUnsolicitedData()", 
                     "only allowed inside active server");

  boost::lock_guard<RlinkConnect> lock(*this);
  RerrMsg emsg;
  int irc = fRcvPkt.ReadData(fupPort.get(), Rtime(), emsg);
  if (irc == 0) return;
  if (irc < 0) {
    RlogMsg lmsg(*fspLog, 'E');
    lmsg << "HandleUnsolicitedData: IO error: " << emsg;
  }
  ProcessUnsolicitedData();
  return;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool RlinkConnect::ExecPart(RlinkCommandList& clist, size_t ibeg, size_t iend,
                            RerrMsg& emsg)
{  
  if (ibeg>iend || iend>=clist.Size())
    throw Rexception("RlinkConnect::ExecPart()",
                     "Bad args: ibeg or iend invalid");
  if (!IsOpen())
    throw Rexception("RlinkConnect::ExecPart()","Bad state: port not open");

  fStats.Inc(kStatNExecPart);
  EncodeRequest(clist, ibeg, iend);

  // FIXME_code: handle send fail properly;
  if (!fSndPkt.SndPacket(fupPort.get(), emsg)) return false;

  // FIXME_code: handle recoveries
  // FIXME_code: use proper value for timeout (rest time for Exec ?)
  bool ok = ReadResponse(fTimeout, emsg);
  if (!ok) Rexception("RlinkConnect::ExecPart()","faulty response");

  int ncmd = DecodeResponse(clist, ibeg, iend);
  if (ncmd != int(iend-ibeg+1)) {
    clist.Dump(cout);
    throw Rexception("RlinkConnect::ExecPart()","incomplete response");
  }

  AcceptResponse();

  return true;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void RlinkConnect::EncodeRequest(RlinkCommandList& clist, size_t ibeg, 
                                 size_t iend)
{
  fSndPkt.Init();

  for (size_t i=ibeg; i<=iend; i++) {
    RlinkCommand& cmd = clist[i];
    uint8_t   ccode = cmd.Command();
    size_t    ndata = cmd.BlockSize();
    uint16_t* pdata = cmd.BlockPointer();

    fStats.Inc(kStatNCmd);

    cmd.SetSeqNumber(fSeqNumber[ccode]++);
    cmd.ClearFlagBit(RlinkCommand::kFlagPktBeg | RlinkCommand::kFlagPktEnd);

    fSndPkt.PutWithCrc(cmd.Request());

    switch (ccode) {
      case RlinkCommand::kCmdRreg:          // rreg command ---------------
        fStats.Inc(kStatNRreg);
        cmd.SetRcvSize(1+2+1+2);            // rcv: cmd+data+stat+crc
        fSndPkt.PutWithCrc(cmd.Address());
        break;

      case RlinkCommand::kCmdRblk:          // rblk command ---------------
        fStats.Inc(kStatNRblk);
        fStats.Inc(kStatNRblkWord, (double) ndata);
        cmd.SetRcvSize(1+2+2*ndata+2+1+2); // rcv: cmd+cnt+n*data+dcnt+stat+crc
        fSndPkt.PutWithCrc(cmd.Address());
        fSndPkt.PutWithCrc((uint16_t)ndata);
        break;

      case RlinkCommand::kCmdWreg:          // wreg command ---------------
        fStats.Inc(kStatNWreg);
        cmd.SetRcvSize(1+1+2);              // rcv: cmd+stat+crc
        fSndPkt.PutWithCrc(cmd.Address());
        fSndPkt.PutWithCrc(cmd.Data());
        break;

      case RlinkCommand::kCmdWblk:          // wblk command ---------------
        fStats.Inc(kStatNWblk);
        fStats.Inc(kStatNWblkWord, (double) ndata);
        cmd.SetRcvSize(1+2+1+2);              // rcv: cmd+dcnt+stat+crc
        fSndPkt.PutWithCrc(cmd.Address());
        fSndPkt.PutWithCrc((uint16_t)ndata);
        fSndPkt.PutCrc();
        fSndPkt.PutWithCrc(pdata, ndata);
        break;

      case RlinkCommand::kCmdLabo:          // labo command ---------------
        fStats.Inc(kStatNLabo);
        cmd.SetRcvSize(1+1+1+2);            // rcv: cmd+babo+stat+crc
        break;
      case RlinkCommand::kCmdAttn:          // attn command ---------------
        fStats.Inc(kStatNAttn);
        cmd.SetRcvSize(1+2+1+2);            // rcv: cmd+data+stat+crc
        break;

      case RlinkCommand::kCmdInit:          // init command ---------------
        fStats.Inc(kStatNInit);
        cmd.SetRcvSize(1+1+2);              // rcv: cmd+stat+crc
        fSndPkt.PutWithCrc(cmd.Address());
        fSndPkt.PutWithCrc(cmd.Data());
        break;

      default:
        throw Rexception("RlinkConnect::Exec()", "BugCheck: invalid command");
    } // switch (ccode)

    fSndPkt.PutCrc();
    cmd.SetFlagBit(RlinkCommand::kFlagSend);
  } // for (size_t i=ibeg; i<=iend; i++)

  // FIXME_code: do we still need kFlagPktBeg,kFlagPktEnd ?
  clist[ibeg].SetFlagBit(RlinkCommand::kFlagPktBeg);
  clist[iend].SetFlagBit(RlinkCommand::kFlagPktEnd);

  return;
}
  
//------------------------------------------+-----------------------------------
//! FIXME_docs

int RlinkConnect::DecodeResponse(RlinkCommandList& clist, size_t ibeg,
                                 size_t iend)
{
  size_t ncmd = 0;
  
  for (size_t i=ibeg; i<=iend; i++) {
    RlinkCommand& cmd = clist[i];
    uint8_t   ccode = cmd.Command();
    uint16_t  rdata;
    uint8_t   rdata8;

    // handle commands after an active labo
    if (clist.LaboActive()) {
      ncmd += 1;
      cmd.SetFlagBit(RlinkCommand::kFlagDone|RlinkCommand::kFlagLabo);
      continue;
    }

    // FIXME_code: handle NAK properly !!

    if (!fRcvPkt.CheckSize(cmd.RcvSize())) {   // not enough data for cmd
      cmd.SetFlagBit(RlinkCommand::kFlagErrDec);
      fStats.Inc(kStatNErrMiss);
      RlogMsg lmsg(*fspLog, 'E');
      lmsg << "DecodeResponse: not enough data for cmd";
      return -1;
    }
    
    fRcvPkt.GetWithCrc(rdata8);
    if (rdata8 != cmd.Request()) { // command mismatch
      cmd.SetFlagBit(RlinkCommand::kFlagErrDec);
      fStats.Inc(kStatNErrCmd);
      RlogMsg lmsg(*fspLog, 'E');
      lmsg << "DecodeResponse: command mismatch";
      return -1;
    }

    switch (ccode) {
      case RlinkCommand::kCmdRreg:          // rreg command ---------------
        fRcvPkt.GetWithCrc(rdata);
        cmd.SetData(rdata);
        break;

      case RlinkCommand::kCmdRblk:          // rblk command ---------------
        fRcvPkt.GetWithCrc(rdata);
        if (rdata != (uint16_t)cmd.BlockSize()) {    // length mismatch
          cmd.SetFlagBit(RlinkCommand::kFlagErrDec);
          fStats.Inc(kStatNErrLen);
          RlogMsg lmsg(*fspLog, 'E');
          lmsg << "DecodeResponse: rblk length mismatch";
          return -1;
        }
        fRcvPkt.GetWithCrc(cmd.BlockPointer(), cmd.BlockSize());
        fRcvPkt.GetWithCrc(rdata);
        cmd.SetBlockDone(rdata);
       break;

      case RlinkCommand::kCmdWreg:          // wreg command ---------------
        break;

      case RlinkCommand::kCmdWblk:          // wblk command ---------------
        fRcvPkt.GetWithCrc(rdata);
        cmd.SetBlockDone(rdata);
       break;

      case RlinkCommand::kCmdLabo:          // labo command ---------------
        fRcvPkt.GetWithCrc(rdata8);
        cmd.SetData((uint16_t)rdata8);
        break;

      case RlinkCommand::kCmdAttn:          // attn command ---------------
        fRcvPkt.GetWithCrc(rdata);
        cmd.SetData(rdata);
        break;

      case RlinkCommand::kCmdInit:          // init command ---------------
        break;
    } // switch (ccode)

    // crc handling
    fRcvPkt.GetWithCrc(rdata8);
    cmd.SetStatus(rdata8);
    if (!fRcvPkt.CheckCrc()) {              // crc mismatch
      cmd.SetFlagBit(RlinkCommand::kFlagErrDec);
      fStats.Inc(kStatNErrCrc);
      RlogMsg lmsg(*fspLog, 'E');
      lmsg << "DecodeResponse: crc mismatch";
      return -1;
    }

    ncmd += 1;
    cmd.SetFlagBit(RlinkCommand::kFlagDone);

    // handle active labo command, here we know that crc is ok
    if (ccode==RlinkCommand::kCmdLabo && cmd.Data()) {    // labo active ?
      clist.SetLaboIndex(i);                  // set index
    }

    // expect handling
    if (cmd.HasExpect()) {                    // expect object attached ?
      auto expect = cmd.Expect();
      if (ccode==RlinkCommand::kCmdRblk || 
          ccode==RlinkCommand::kCmdWblk) {
        if (expect.BlockValue().size()>0) fStats.Inc(kStatNExpData);
        if (expect.DoneIsChecked()) fStats.Inc(kStatNExpDone);
      } else {
        if (expect.DataIsChecked()) fStats.Inc(kStatNExpData);
      }

      if (ccode==RlinkCommand::kCmdRreg || 
          ccode==RlinkCommand::kCmdLabo ||
          ccode==RlinkCommand::kCmdAttn) {
        if (!expect.DataCheck(cmd.Data())) {
          fStats.Inc(kStatNChkData);
          cmd.SetFlagBit(RlinkCommand::kFlagChkData);
        }
      } else if (ccode==RlinkCommand::kCmdRblk) {
        size_t nerr = expect.BlockCheck(cmd.BlockPointer(), cmd.BlockSize());
        if (nerr != 0) {
          fStats.Inc(kStatNChkData);
          cmd.SetFlagBit(RlinkCommand::kFlagChkData);
        }
      }
      if (ccode==RlinkCommand::kCmdRblk || 
          ccode==RlinkCommand::kCmdWblk) {        
        if (!expect.DoneCheck(cmd.BlockDone())) {
          fStats.Inc(kStatNChkDone);
          cmd.SetFlagBit(RlinkCommand::kFlagChkDone);
        }
      }

    } // if (cmd.Expect())

    // status check, now independent of Expect object
    if (cmd.ExpectStatusSet())  fStats.Inc(kStatNExpStat);
    if (!cmd.StatusIsChecked()) fStats.Inc(kStatNNoExpStat);
    if (!cmd.StatusCheck()) {
      fStats.Inc(kStatNChkStat);
      cmd.SetFlagBit(RlinkCommand::kFlagChkStat);
    }

  } // for (size_t i=ibeg; i<=iend; i++)

  // FIXME_code: check that all data is consumed !!

  return ncmd;
}

//------------------------------------------+-----------------------------------
//! Decodes an attention notify packet.
/*!
  \param[out]  apat  attention pattern, can be zero
  \returns  \c true if decode without errors, \c false otherwise
 */

bool RlinkConnect::DecodeAttnNotify(uint16_t& apat)
{
  apat = 0;
  
  if (!fRcvPkt.CheckSize(2+2)) {            // not enough data for data+crc
    fStats.Inc(kStatNErrMiss); 
    RlogMsg lmsg(*fspLog, 'E');
    lmsg << "DecodeAttnNotify: not enough data for data+crc";
    return false;
  }

  fRcvPkt.GetWithCrc(apat);

  if (!fRcvPkt.CheckCrc()) {                // crc mismatch
    fStats.Inc(kStatNErrCrc);
    RlogMsg lmsg(*fspLog, 'E');
    lmsg << "DecodeAttnNotify: crc mismatch";
    return false;
  }

  // FIXME_code: check for extra data

  return true;
}

//------------------------------------------+-----------------------------------
//! Read data from port until complete response packet seen.
/*!
  Any spurious data or corrupt packages, e.g. with framing errors, 
  are logged and discarded.

  If an attention notify packet is detected it will handled with
  ProcessAttnNotify().

  The method returns \c true if a complete response packet is received.
  The caller will usually use DecodeResponse() and must accept the packet
  with AcceptResponse() afterwards.

  The method returns \c false if
    - no valid response packet is seen within the time given by \a timeout
    - an IO error occurred
    .
  An appropriate log message is generated, any partial input packet discarded.

  \param      timeout  maximal time to wait for input in sec. Must be > 0.
  \param[out] emsg     contains error description (mainly from port layer)

  \returns \c true if complete response packet received

  \pre a previous response must have been accepted with AcceptResponse().
 */ 

bool RlinkConnect::ReadResponse(const Rtime& timeout, RerrMsg& emsg)
{
  Rtime tnow(CLOCK_MONOTONIC);
  Rtime tend = tnow + timeout;

  while (tnow < tend) {
    int irc = fRcvPkt.ReadData(fupPort.get(), tend-tnow, emsg);
    if (irc <= 0) {
      RlogMsg lmsg(*fspLog, 'E');
      lmsg << "ReadResponse: IO error or timeout: " << emsg;
      return false;
    }

    while (fRcvPkt.ProcessData()) {
      int irc = fRcvPkt.PacketState();
      if (irc == RlinkPacketBufRcv::kPktPend) break;
      if (irc == RlinkPacketBufRcv::kPktAttn) {
        ProcessAttnNotify();
      } else if (irc == RlinkPacketBufRcv::kPktResp) {
        return true;
      } else {
        RlogMsg lmsg(*fspLog, 'E');
        lmsg << "ReadResponse: dropped spurious packet";
        fRcvPkt.AcceptPacket();
      }
    } //while (fRcvPkt.ProcessData())

    tnow.GetClock(CLOCK_MONOTONIC);

  } // while (tnow < tend)

  { 
    RlogMsg lmsg(*fspLog, 'E');
    lmsg << "ReadResponse: timeout";
  }
  fRcvPkt.AcceptPacket();

  return false;
}

//------------------------------------------+-----------------------------------
//! Accept response packet received with ReadResponse().
/*!
  The packet buffer is cleared, and any still buffered input data is processed
  with ProcessUnsolicitedData(). 
 */

void RlinkConnect::AcceptResponse()
{
  fRcvPkt.AcceptPacket();
  ProcessUnsolicitedData();
  return;
}
  
//------------------------------------------+-----------------------------------
//! Process data still pending in the input buffer.
/*!
  If an attention notify packet is detected it will handled with
  ProcessAttnNotify(). If a response or corrupted packet is seen
  it will be logged and discarded.
 */

void RlinkConnect::ProcessUnsolicitedData()
{
  while (fRcvPkt.ProcessData()) {
    int irc = fRcvPkt.PacketState();
    if (irc == RlinkPacketBufRcv::kPktPend) break;
    if (irc == RlinkPacketBufRcv::kPktAttn) {
      ProcessAttnNotify();
    } else {
      fRcvPkt.AcceptPacket();
      RlogMsg lmsg(*fspLog, 'E');
      lmsg << "ProcessUnsolicitedData: dropped spurious packet";
    }
  }  
  return;
}

//------------------------------------------+-----------------------------------
//! Process attention notify packets.
/*!
  The packets is decoded with DecodeAttnNotify(). If the packet is valid and 
  contains a non-zero attention pattern the pattern is ored to the attention 
  notify pattern which can later be inquired with HarvestAttnNotifies().
  Corrupt packets are logged and discarded. Notifies with a zero pattern
  are silently ignored.
 */

void RlinkConnect::ProcessAttnNotify()
{
  uint16_t apat;
  bool ok = DecodeAttnNotify(apat);
  fRcvPkt.AcceptPacket();

  if (ok) {
    if (apat) {
      if (ServerActive()) {                   // if server active
        fpServ->SignalAttnNotify(apat);       //   handle in RlinkServer
      } else {                                // otherwise
        fAttnNotiPatt |= apat;                //   handle in RlinkConnect
      }
    } else {
      RlogMsg lmsg(*fspLog, 'W');
      lmsg << "ProcessAttnNotify: zero attn notify received";
    }
  }

  if (ok && fPrintLevel == 3) {
     RlogMsg lmsg(*fspLog, 'I');
     lmsg << "ATTN notify apat = " << RosPrintf(apat,"x0",4)
          << "  lams =";
     if (apat) {
       char sep = ' ';
       for (int i=15; i>=0; i--) {
         if (apat & (uint16_t(1)<<i) ) {
           lmsg << sep << i;
           sep = ',';
         }
       }
     } else {
       lmsg << " !NONE!";
     }
     Rtime tnow(CLOCK_MONOTONIC);
     if (fTsLastAttnNoti.IsPositive()) 
       lmsg << "  dt=" << RosPrintf(double(tnow-fTsLastAttnNoti),"f",8,6);
     fTsLastAttnNoti = tnow;
  }
  return;
}

} // end namespace Retro
