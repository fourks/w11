// $Id: Rw11VirtDisk.hpp 1076 2018-12-02 12:45:49Z mueller $
//
// Copyright 2013-2018 by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
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
// 2018-12-02  1076   1.3    use unique_ptr for New()
// 2018-10-27  1061   1.2    add fNCyl,fNHead,fNSect,NCylinder(),...
// 2017-04-07   868   1.1.1  Dump(): add detail arg
// 2017-04-02   866   1.1    add default scheme handling
// 2013-03-03   494   1.0    Initial version
// 2013-02-13   488   0.1    First draft
// ---------------------------------------------------------------------------


/*!
  \file
  \brief   Declaration of class Rw11VirtDisk.
*/

#ifndef included_Retro_Rw11VirtDisk
#define included_Retro_Rw11VirtDisk 1

#include <string>
#include <memory>

#include "Rw11Virt.hpp"

namespace Retro {

  class Rw11VirtDisk : public Rw11Virt {
    public:
      explicit      Rw11VirtDisk(Rw11Unit* punit);
                   ~Rw11VirtDisk();

      void          Setup(size_t blksize, size_t nblock,
                          size_t ncyl, size_t nhead, size_t nsect);
      size_t        BlockSize() const;
      size_t        NBlock() const;
      size_t        NCylinder() const;
      size_t        NHead() const;
      size_t        NSector() const;

      virtual bool  Read(size_t lba, size_t nblk, uint8_t* data, 
                         RerrMsg& emsg) = 0;
      virtual bool  Write(size_t lba, size_t nblk, const uint8_t* data, 
                          RerrMsg& emsg) = 0;

      virtual void  Dump(std::ostream& os, int ind=0, const char* text=0,
                         int detail=0) const;

      static std::unique_ptr<Rw11VirtDisk> New(const std::string& url,
                                               Rw11Unit* punit, RerrMsg& emsg);

      static const std::string& DefaultScheme();
      static void   SetDefaultScheme(const std::string& scheme);

    // statistics counter indices
      enum stats {
        kStatNVDRead = Rw11Virt::kDimStat,
        kStatNVDReadBlk,
        kStatNVDWrite,
        kStatNVDWriteBlk,
        kDimStat
      };    

    protected:
      size_t        fBlkSize;               //!< block size in byte
      size_t        fNBlock;                //!< disk size in blocks
      size_t        fNCyl;                  //!< # cylinder
      size_t        fNHead;                 //!< # heads (aka surfaces)
      size_t        fNSect;                 //!< # sectors

    protected:
      static std::string sDefaultScheme;     //!< default scheme
  };
  
} // end namespace Retro

#include "Rw11VirtDisk.ipp"

#endif
