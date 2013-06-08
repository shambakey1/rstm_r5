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

#include <iostream>

#include "ContentionManager.hpp"
#include "Polka.hpp"
#include "Karma.hpp"
#include "Eruption.hpp"
#include "Polite.hpp"
#include "Timestamp.hpp"
#include "Polkaruption.hpp"
#include "Killblocked.hpp"
#include "Aggressive.hpp"
#include "Highlander.hpp"
#include "Whpolka.hpp"
#include "Greedy.hpp"
#include "Polkavis.hpp"
#include "Serializer.hpp"
#include "Justice.hpp"
#include "Timid.hpp"
#include "Reincarnate.hpp"
#include "Gladiator.hpp"
#include "AggressiveR.hpp"
#include "TimidR.hpp"
#include "PoliteR.hpp"
/************************* SH-START **********************************/
#include "ECM.hpp"
#include "RCM.hpp"
#include "LCM.hpp"
#include "FIFO.hpp"
#include "PNF.hpp"
#include "FBLT.hpp"

stm::ContentionManager* no_cm=NULL;	//Used to return NULL pointer if CM does not exist. This replaces the default CM (Polka)
unsigned long long new_tx_null=0;	//Initilizer value for new_tx_released and new_tx_committed
unsigned long long* stm::new_tx_released=&new_tx_null;	//Holds address of a released transaction to be used in pnf_helper
unsigned int stm::new_tx_checking=0;	//If 1, then pnf_main should check Txs in n_set
unsigned long long* stm::new_tx_committed=&new_tx_null;	//Holds address of a committed transaction to be used in pnf_helper

unsigned long long stm::m_set_bits=0;	//Holds bit representation of all objects held in m_set in PNF
										//Due to current implementation, m_set_bits cannot exceed 64 objects
pthread_t stm::pnf_main_th;				//pnf_main service thread
pthread_attr_t stm::pnf_th_attr;		//Attributes for pnf_main service thread
struct sched_param stm::pnf_main_param;	//scheduling parameters for pnf_main service
unsigned long stm::pnf_main_th_init=0;	//If 1, then pnf_main_th has been created
/************************* SH-END **********************************/

// create the appropriate cm based on the input string
stm::ContentionManager* stm::Factory(std::string cm_type)
{
    if (cm_type == "Eruption")
        return new Eruption();
    else if (cm_type == "Serializer")
        return new Serializer();
    else if (cm_type == "Justice")
        return new Justice();
    else if (cm_type == "Karma")
        return new Karma();
    else if (cm_type == "Killblocked")
        return new Killblocked();
    else if (cm_type == "Polite")
        return new Polite();
    else if (cm_type == "Polka")
        return new Polka();
    else if (cm_type == "Polkaruption")
        return new Polkaruption();
    else if (cm_type == "Timestamp")
        return new Timestamp();
    else if (cm_type == "Aggressive")
        return new Aggressive();
    else if (cm_type == "Highlander")
        return new Highlander();
    else if (cm_type == "Whpolka")
        return new Whpolka();
    else if (cm_type == "Greedy")
        return new Greedy();
    else if (cm_type == "Polkavis")
        return new Polkavis();
    else if (cm_type == "Timid")
        return new Timid();
    else if (cm_type == "Reincarnate")
        return new Reincarnate();
    else if (cm_type == "Gladiator")
        return new Gladiator();
    else if (cm_type == "PoliteR")
        return new PoliteR();
    else if (cm_type == "TimidR")
        return new TimidR();
    else if (cm_type == "AggressiveR")
        return new AggressiveR();
/*************************** SH-START ***********************************************/
// As FIFO does not expect any input arguments, so it is positioned here
// not with the others (ECM, RCM, ... etc)
    else if (cm_type == "FIFO")
	return new FIFO();
    else if (cm_type=="ECM")
	return new ECM();
    else if (cm_type=="LCM")
	return new LCM();
    else if (cm_type=="PNF")
	return new PNF();
    else if (cm_type=="RCM")
	return new RCM();
    else if (cm_type=="FBLT")
        return new FBLT();
/*************************** SH-END ***********************************************/
    else {
        std::cerr << "*** Warning: unknown contention manager "
                  << cm_type << std::endl;
        std::cerr << "*** Defaulting to NULL" << std::endl;
        return no_cm;
    }
}

/********************************* SH-START **********************************************/
// Modified Factory method in include real-time parameters
// and calling only real-time CMs (i. e., ECM, RCM, LCM)
// Timestamp might work like FIFO CM
stm::ContentionManager* stm::Factory(std::string cm_type, void* t_args)
{
//    if (cm_type == "RCM")
//        return new RCM(t_args);
    if (cm_type == "LCM")
        return new LCM(t_args);
    else {
        std::cerr << "*** Warning: unknown contention manager "
                  << cm_type << std::endl;
        std::cerr << "*** Defaulting to NULL" << std::endl;
        return no_cm;
    }
}
/********************************* SH-END **********************************************/

// provide backing for static variables in different CMs
volatile unsigned long stm::Greedy::timeCounter = 0;
volatile unsigned long stm::Serializer::timeCounter = 0;
volatile unsigned long stm::Reincarnate::timeCounter = 0;

/********************* SH-START *****************************/
bool stm::compTimeSpec(struct timespec* f_ts,struct timespec* s_ts){
	/*
	 * "true" if f_ts timespec is smaller than s_ts. "false" otherwise
	 */
	if((f_ts->tv_sec < s_ts->tv_sec) || ((f_ts->tv_sec == s_ts->tv_sec)&&(f_ts->tv_nsec < s_ts->tv_nsec))){
		return true;
	}
	else{
		return false;
	}
}

int stm::nSetPos(void* cm_ptr){
	/*
	 * Returns position of cm_ptr within n_set
	 */
	unsigned int n_set_size=n_set.size();
	for(unsigned int i=0;i<n_set_size;i++){
		if(cm_ptr==n_set[i]){
			//Found required position
			return i;
		}
	}
	//cm_ptr does not exist in n_set
	return -1;
}

void stm::addTxNset(void* cm_th){
	/*
	 * Add cm_th to n_set according to its priority
	 */
	unsigned int n_set_size=n_set.size();
	for(unsigned int i=0;i<n_set_size;i++){
		if(compTimeSpec(((ContentionManager*)(cm_th))->getTimeParam(),((ContentionManager*)(n_set[i]))->getTimeParam())){
			n_set.insert(n_set.begin()+i,cm_th);	//Found right position. Put cm_th there
			return;
		}
	}
	//cm_th is the last Tx in n_set
	n_set.push_back(cm_th);
}

void* stm::pnf_main(void* arg){
	/*
	 * Main (centralized) service of PNF. It continues execution until all tasks finish. It is invoked
	 * by Txs to execute some tasks.
	 */
	unsigned int next_tx;	//Tx index in n_set

	while(!cm_stop){
		if(new_tx_released!=&new_tx_null){
			//A new Tx is released. Check there is no conflict with current executing Txs
			if(m_set_bits & ((ContentionManager*)new_tx_released)->curr_objs_bits){
				//AND is not 0. There is a conflict. m_set is already false
				addTxNset((void*)new_tx_released);	//Put the new Tx in n_set
				((ContentionManager*)new_tx_released)->cur_state=retrying;	//Identify the new Tx as retrying
				((ContentionManager*)new_tx_released)->param.sched_priority=PNF_N_PRIO;
				sched_setparam(((ContentionManager*)new_tx_released)->th, &(((ContentionManager*)new_tx_released)->param));	//Reduce priority to lowest value
			}
			else{
				//AND is 0. There is no conflict.
				((ContentionManager*)new_tx_released)->m_set=true;
				((ContentionManager*)new_tx_released)->cur_state=executing;
				//Modify m_set_bits to include the new accessed objects' bits
				m_set_bits |= (((ContentionManager*)new_tx_released)->curr_objs_bits);
				((ContentionManager*)new_tx_released)->param.sched_priority=PNF_M_PRIO;
				sched_setparam(((ContentionManager*)new_tx_released)->th, &(((ContentionManager*)new_tx_released)->param));	//Increase priority to highest value as Tx is a non-preemptive Tx
			}
			((ContentionManager*)new_tx_released)->go_on=true;	//Tell released Tx to continue execution
			new_tx_released=&new_tx_null;	//reset to be used by another Tx
		}
		if(new_tx_checking){
			//Check Txs in n_set. Checking is done after a Tx commits
			next_tx=0;	//Index of first Tx in n_set
			while(next_tx<n_set.size()){
				//Check if there is conflict with m_set_bits
				if(m_set_bits & ((ContentionManager*)(n_set[next_tx]))->curr_objs_bits){
					//AND is not 0. There is a conflict
					next_tx++;	//Move to next Tx in n_set
				}
				else{
					//AND is 0. There is no conflict. next_tx does not need to be modified here because
					//current Tx will be removed from n_set
					((ContentionManager*)(n_set[next_tx]))->m_set=true;
					((ContentionManager*)(n_set[next_tx]))->cur_state=executing;
					m_set_bits |= (((ContentionManager*)(n_set[next_tx]))->curr_objs_bits);	//Modify m_set_bits to include the new accessed objects' bits
					n_set.erase(n_set.begin()+next_tx);	//Remove checking Tx from n_set
					((ContentionManager*)(n_set[next_tx]))->param.sched_priority=PNF_M_PRIO;
					sched_setparam(((ContentionManager*)(n_set[next_tx]))->th, &(((ContentionManager*)(n_set[next_tx]))->param));	//Increase priority to highest value as Tx is a non-preemptive Tx
				}
			}
			new_tx_checking=0;	//reset to be used after another Tx commits
		}
		if(new_tx_committed!=&new_tx_null){
			//A Tx has committed
			if(((ContentionManager*)new_tx_committed)->cur_state==retrying){
				/*
				 * Current Tx has committed while retrying (it happens because no other
				 * transaction conflicts with it). Current Tx is in n_set. Remove it from there
				 */
                n_set.erase(n_set.begin()+nSetPos((void*)new_tx_committed));
            }
			else{
				//Tx was executing and has committed
                //Remove accessed objects from m_set_bits
				m_set_bits &= (~(((ContentionManager*)new_tx_committed)->curr_objs_bits));
				new_tx_checking=1;	//To start checking Txs in n_set
			}
            //Restore default values for m_set and cur_state. Otherwise, the next Tx
            //will go on with the last values for these variables
			((ContentionManager*)new_tx_committed)->m_set=false;
			((ContentionManager*)new_tx_committed)->cur_state=released;
			sched_setparam(((ContentionManager*)new_tx_committed)->th, &(((ContentionManager*)new_tx_committed)->orig_param));	//Restore priority of current Tx to its original real-time priority
			((ContentionManager*)new_tx_committed)->go_on=true;
			new_tx_committed=&new_tx_null;	//reset to be used by another Tx
		}
	}
	cm_stop=false;	//reset pnf_stop to be used next time
	pnf_main_th_init=0;	//reset to be used next time
	return NULL;
}
/********************* SH-END ********************************/
