///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2005, 2006, 2007, 2008, 2009
// University of Rochester
// Department of Computer Science
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the University of Rochester nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef __POLKARUPTION_H__
#define __POLKARUPTION_H__

#include "Polka.hpp"

namespace stm
{
  /**
   * combine Polka with Eruption
   * NB: we're re-rolling the Eruption part by hand
   */
  class Polkaruption: public Polka
  {
    private:
      int prio_transferred;

      // every time we Open something, reset the try counter, increase
      // the karma
      void onOpen()
      {
          priority++;
          prio_transferred = 0;
          tries = 1;
      }

      // request permission to abort enemy tx
      ConflictResolutions shouldAbort(ContentionManager* enemy)
      {
          Polkaruption* p;

          if (!enemy)
              return AbortOther;

          if ((tries + priority) > enemy->getPriority()) {
              return AbortOther;
          }
          else {
              p = static_cast<Polkaruption*>(enemy);

              if (p) {
                  p->givePrio(priority - prio_transferred);
                  prio_transferred = priority;
              }

              return Wait;
          }
      }

    public:
      // simple constructor
      Polkaruption() : prio_transferred(0) { }


      // transfer priority
      void givePrio(int prio)
      {
          priority += prio;
      }

      // event methods
      virtual void onOpenRead() { onOpen(); }
      virtual void onOpenWrite() { onOpen(); }

      virtual void onTransactionCommitted()
      {
          priority = 0;
          prio_transferred = 0;
      }

      virtual ConflictResolutions onRAW(ContentionManager* enemy)
      {
          return shouldAbort(enemy);
      }
      virtual ConflictResolutions onWAR(ContentionManager* enemy)
      {
          return AbortOther;
      }
      virtual ConflictResolutions onWAW(ContentionManager* enemy)
      {
          return shouldAbort(enemy);
      }
  };
}; // namespace stm

#endif // __POLKARUPTION_H__
