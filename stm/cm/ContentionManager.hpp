//////////////////////////////////////////////////////////////////////////////
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

#ifndef __CONTENTIONMANAGER_H__
#define __CONTENTIONMANAGER_H__

#include <string>
#include "../support/atomic_ops.h"
#include "../support/hrtime.h"
/*********************************** SH-START ******************************************/
#include <chronos/chronos.h>
#include <chronos/chronos_utils.h>
#include <iostream>
#include <vector>
#include <time.h>
#include <rstm_hlp.hpp>
#include <pthread.h>
#include <string>
#include <sstream>
using namespace std;
/*********************************** SH-END ******************************************/


namespace stm
{
  enum ConflictResolutions { AbortSelf, AbortOther, Wait };
  enum tx_state {released,retrying,executing};	//Different states for transaction
  extern unsigned long long *new_tx_released;	//Holds address of a released transaction to be used in pnf_helper
  extern unsigned int new_tx_checking;	//If 1, then pnf_main should check Txs in n_set
  extern unsigned long long *new_tx_committed;	//Holds address of a committed transaction to be used in pnf_helper
  extern unsigned long long m_set_bits;	//Holds bit representation of all objects held in m_set in PNF
  										//Due to current implementation, m_set_bits cannot exceed 64 objects
  extern pthread_t pnf_main_th;				//pnf_main service thread
  extern pthread_attr_t pnf_th_attr;			//Attributes for pnf_main service thread
  extern struct sched_param pnf_main_param;	//scheduling parameters for pnf_main service
  extern unsigned long pnf_main_th_init;	//If 1, then pnf_main_th has been created

  extern bool compTimeSpec(struct timespec* f_ts,struct timespec* s_ts);	//"true" if f_ts timespec is smaller than s_ts. "false" otherwise
  extern int nSetPos(void* cm_ptr);	//Returns position of cm_ptr within n_set
  extern void addTxNset(void* cm_th);	//Add cm_th to n_set according to its priority
  extern void* pnf_main(void* arg);	//Main (centralized) service of PNF. It continues execution until all tasks finish. It is invoked by Txs to execute some tasks

  class ContentionManager
  {
    protected:
      int priority;
    public:
      /********************************** SH-START *******************************/
      struct timespec time_param;			//input parameter for ECM which should be cast to deadline
                                        		//In case of RCM, it holds the period
	unsigned long length;				//specific for LCM
	double psy;					//specific for LCM
	struct timespec stamp, tra_abort;		//stamp records the beginning of the transaction, while tra_abort records when the transaction is aborted
	unsigned long long total_abort_duration;	//Holds the total abort time of all instances of the thread during the whole run time of experimennt
	vector<double> acc_obj;                         //list of accessed objects by current tx
        int th;				//ptr to current thread
        /************************ Debug 1 start ****************************/
	//vector<timespec> tra_start, tra_abr;	//Record transaction start, transaction abort.
	//vector<unsigned long long> tra_int;	//Records difference between transaction start and transaction abort so we can calculate total time of abortion for transactions in each thread
	/************************ Debug 1 end ****************************/
        /********************* Debug 1 start ******************/
        vector<string> rec;	//Holds intermediate records for logging
	//vector<string> rec_final;	//Holds final logging for current transaction
        stringstream rec_in;
        struct timespec rec_time;
        struct sched_param param;
        struct sched_param param_tmp;
        int policy;
        int task_run_prio;
        int task_end_prio;
        int task_util;
        struct timespec* task_deadline;
        struct timespec* task_period;
        unsigned long task_unlocked;
        unsigned long task_locked;
        int eta;            //Defines maximum number of times each transaction can be aborted
                            //Used in FBLT
	bool new_tx;	//if true, it is first time to begin transaction. Otherwise, tx has already begun and it is just
			//an abort and retry
	unsigned long long curr_objs_bits;	//Indecies of accessed objects by current Tx. If bit at position X=1, then
						//current Tx accesses object number X. It should be identified at Tx_begin.
						//As curr_objs is represented with unsigned long long, than maximum
						//number of accessed objects cannot exceed 64
	bool go_on;	//If true, pnf_main tells current Tx to complete execution. It is used to
			//synchronize execution between pnf_main and current Tx
	tx_state cur_state;			//Holds state of current transaction
	struct sched_param orig_param;	//records original sched_param for current thread when a Tx starts
	bool m_set;				//if "true", tx is an executing tx. Otherwise, tx is a retrying one.
        /********************* Debug 1 end ******************/
        /******************* Debug 6 start *****************/	
        vector<string> getRec(){
            return rec;
        }
        
        void newInst(){
            //Declares a new instance of the current thread
            //Define things that need to be renewed in the new instance
            rec=vector<string> ();
        }
          /******************* Debug 6 end *****************/
      struct timespec* getTimeParam(){
          return &time_param;
      }

	/******************** LCM functions start ********************/

	void setLength(unsigned long in_length){
		length=in_length;
	}

	unsigned long getLength(){
		return length;
	}

	void setPsy(double in_psy){
		psy=in_psy;
	}

	double getPsy(){
		return psy;
	}
	/******************** LCM functions end ********************/

	/******************** SH-START *******************/
        void setAccObj(vector<double> in_acc_obj){
	//Set list of accessed objects by current transaction
		acc_obj=in_acc_obj;
	}

	vector<double> getAccObj(){
	//get list of accessed objects by current transaction
		return acc_obj;
	}
        
        void setCurThr(int in_th){
	//set value to thread inititing current transaction
		th=in_th;
	}

	int getCurThr(){
	//get value of thread initiating current transaction
		return th;
	}

      ContentionManager() : priority(0),total_abort_duration(0){
	/******************************* SH-START **********************************/
	tra_abort.tv_sec=0;
	tra_abort.tv_nsec=0;
	new_tx=true;
	go_on=false;
	/******************************* SH-END **********************************/
      }
      int getPriority() { return priority; }

      ////////////////////////////////////////
      // Transaction-level events
      virtual void onBeginTransaction() {
	  try{
		clock_gettime(CLOCK_REALTIME, &stamp);
		/********************************* Debug 5 start ********************************/
		//tra_start.push_back(stamp);
		/********************************* Debug 5 end ********************************/
	    }catch(exception e){
		cout<<"onBeginTransaction exception: "<<e.what()<<endl;
	    }
     }
	virtual void onTryCommitTransaction() { }
	virtual void onTransactionCommitted() {
		priority = 0;	//This step is not part of PNF. It comes from original CM
		new_tx=true;
	}
      virtual void onTransactionAborted() {
	  try{
		clock_gettime(CLOCK_REALTIME, &tra_abort);
		total_abort_duration+=subtract_ts_mod(&stamp,&tra_abort);
		/*************************** Debug 6 start ********************************/
		//tra_abr.push_back(tra_abort);
		//tra_int.push_back(subtract_ts_mod(&stamp,&tra_abort));
		/*************************** Debug 6 end ********************************/
	    }catch(exception e){
		cout<<"onTransactionAborted exception: "<<e.what()<<endl;
	    }

      }

      ////////////////////////////////////////
      // Object-level events
      virtual void onContention() { }
      virtual void onOpenRead() { }
      virtual void onOpenWrite() { }
      virtual void onReOpen() { }

      ////////////////////////////////////////
      // Conflict Event methods
      virtual ConflictResolutions onRAW(ContentionManager* enemy) = 0;
      virtual ConflictResolutions onWAR(ContentionManager* enemy) = 0;
      virtual ConflictResolutions onWAW(ContentionManager* enemy) = 0;

      virtual ~ContentionManager() { }
  };

  // create a contention manager
  ContentionManager* Factory(std::string cm_type);

/************************* SH-START **********************************/
	// Redefine factory to pass real-time arguments to the real-time CM
  ContentionManager* Factory(std::string cm_type, void* t_args);
/************************* SH-END **********************************/

  // wait by executing a bunch of nops in a loop (not preemption
  // tolerant)
  inline void nano_sleep(unsigned long nops)
  {
      for (unsigned i = 0; i < nops; i++)
          nop();
  }
} // namespace stm

#endif  // __CONTENTIONMANAGER_H__
