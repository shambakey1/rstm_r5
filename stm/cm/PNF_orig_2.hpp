// THE WHOLE FILE NEEDS TO BE WRITTEN BECAUSE IT CURRENTLY ACTS LIKE RCM
// SO THIS CODE IS NOT THE ACTUAL IMPLEMENTATION OF LCM, IT IS ONLY A PLACEHOLDER SO
// RSTM CAN BE COMBILED WITHOUT BUGS
#ifndef __PNF_H__
#define __PNF_H__

#ifdef _MSC_VER
#include <time.h>
#include <winsock.h>
#else
#include <sys/time.h>
#endif

#ifndef in_rstm
#define in_rstm
#endif

#include "ContentionManager.hpp"
#include <cmath>

using namespace std;


namespace stm
{

  /**
   *  Follow the paper to know how PNF works
   */


  class PNF: public ContentionManager
  {
    private:

        struct timespec now;
	

        ConflictResolutions shouldAbort(ContentionManager* enemy);

    public:

    PNF(){
		m_set=false;
		cur_state=released;
		if(!pnf_main_th){
			//pnf_main service has not been created yet. Print error message and exit
			cout<<"pnf_main service has not been created yet."<<endl;
			exit(0);
		}
    }

    timespec* GetTimestamp(){
    	return &stamp;
    }

	void setMset(bool in_m_set){
	//if "true", then current transaction becomes executing one. Otherwise, transaction is in n_set
		m_set=in_m_set;
	}

	bool getMset(){
	//get status of current transaction (i. e., executing or not)
		return m_set;
	}

	void setObjBits(){
		/*
		 * Transform accessed objects into binary form.
		 */
		unsigned int acc_obj_size=acc_obj.size();
		for(unsigned int i=0;i<acc_obj_size;i++){
			curr_objs_bits|=((unsigned long long)1)<<((int)(acc_obj[i]));
		}
	}

	virtual void onBeginTransaction() {
	  try{
		clock_gettime(CLOCK_REALTIME, &stamp);
		if(cur_state==released){
			go_on=true;
			sched_getparam(0,&orig_param);	//records original sched_param for current thread
			curr_objs_bits=0;	//Reset to set bits corresponding to objects of current Tx
			setObjBits();
			//Tx newely released. Try to set address of current CM into new_tx_released
			while(__sync_val_compare_and_swap(&new_tx_released,0,(unsigned long long)this));
			while(go_on){
				//Loop until pnf_main tell current Tx to continue
			}
		}
	  }catch(exception e){
	    	cout << "onBeginTransaction exception: " << e.what() << endl;
	    }
	}

	virtual void onTransactionCommitted() {
		try{
			//Tx commits. Try to set address of current CM into new_tx_committed
			go_on=true;
			while(__sync_val_compare_and_swap(&new_tx_committed,0,(unsigned long long)this));
			while(go_on){
				//Loop until pnf_main tell current Tx to continue
			}
		}catch(exception e){
			cout << "onTransactionCommitted exception: " << e.what() << endl;
		}
	}

    virtual void onTransactionAborted() {
	    try{
		  clock_gettime(CLOCK_REALTIME, &tra_abort);
		  total_abort_duration+=subtract_ts_mod(&stamp,&tra_abort);
	    }catch(exception e){
		  cout << "onTransactionAborted exception: " << e.what() << endl;
	    }
    }



/*
      time_t GetTimestamp() { return stamp; }
      void SetDefunct() { defunct = true; }
      bool GetDefunct() { return defunct; }

      virtual void onContention()
      {
          defunct = false;
          nano_sleep(INTERVAL);
          tries++;
      }

      virtual void onOpenRead() { onOpen(); }
      virtual void onOpenWrite() { onOpen(); }
      virtual void onTransactionCommitted() { defunct = false; }
      virtual void onTryCommitTransaction() { defunct = false; }
      virtual void onTransactionAborted() { defunct = false; }
      virtual void onBeginTransaction();

 */
      
      virtual ConflictResolutions onRAW(ContentionManager* e)
      {
          return shouldAbort(e);
      }
      virtual ConflictResolutions onWAR(ContentionManager* e)
      {
          return shouldAbort(e);
      }
      virtual ConflictResolutions onWAW(ContentionManager* e)
      {
          return shouldAbort(e);
      }
  };
}; // namespace stm

inline stm::ConflictResolutions
stm::PNF::shouldAbort(ContentionManager* enemy)
{
//	PNF* t=static_cast<PNF*>(enemy);
	//If current task is in m_set, abort other
	bool other_mset=((PNF*)enemy)->m_set;
	if(m_set && !other_mset){
		return AbortOther;
	}
	else if(!m_set && other_mset){
		return AbortSelf;
	}
	else if(m_set && other_mset){
		/*
		 * Precaution case due to timing issues
		 */
		if(compTimeSpec(&stamp,&(((PNF*)enemy)->stamp))){
			return AbortOther;
		}
		else{
			return AbortSelf;
		}
	}
	else{
		return AbortSelf;
	}	
}



#endif // __PNF_H__
