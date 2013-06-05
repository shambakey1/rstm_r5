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
#include <time.h>
#include <chronos/chronos.h>
#include <chronos/chronos_utils.h>
#include <rstm_hlp.hpp>
#include <cmath>
#include <vector>
#include <pthread.h>
#include <string>
#include <sstream>
/**************** Debug 2 start ****************/
#include <iostream>
/**************** Debug 2 end ****************/

using namespace std;


namespace stm
{

  /**
   *  Follow the paper to know how PNF works
   */

//vector<double> m_set_objs;		//Holds accessed objects by executing transactions
//vector<ContentionManager*> n_set;	//Holds non executing transactions
//pthread_mutex_t m_set_mutx = PTHREAD_MUTEX_INITIALIZER; //Mutex to check m_set for conflicting objects. Removal from m_set does not need mutex
enum tx_state {released,retrying,checking,executing};	//Different states for transaction


  class PNF: public ContentionManager
  {
    private:

        struct timespec now;
	

        ConflictResolutions shouldAbort(ContentionManager* enemy);

    public:

	
	bool m_set;				//if "true", tx is an executing tx. Otherwise, tx is a retrying one.
	tx_state cur_state;			//Holds state of current transaction
        

    PNF():m_set(false),cur_state(released)
    {
        //mu_init();
    }
      timespec* GetTimestamp() { return &stamp; }

        	

	void setMset(bool in_m_set){
	//if "true", then current transaction becomes executing one. Otherwise, transaction is in n_set
		m_set=in_m_set;
	}

	bool getMset(){
	//get status of current transaction (i. e., executing or not)
		return m_set;
	}

	int nSetPos(void* cm_ptr){
	//Returns position of cm_ptr within n_set
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

	bool checkMset(unsigned int tx_pos){
	
	//pos=> position of requesting Tx if it is in n_set. -1 otherwise.
	//Initially called when transaction begins. Afterwards, called by checking transactions
	//Check if requesting transaction conflicts with current executing transactions
	//Hold mutex first. Otherwise, two conflicting transactions might add the same objects at the same check time.
	//"true" if Tx added to m_set. "false" otherwise.
            /******************** Debug 3 start **********************/
/*
            clock_gettime(CLOCK_REALTIME, &rec_time);
            sched_getparam(0, &param);
            rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "start checkMset" << endl;
            rec.push_back(rec_in.str());
            rec_in.str("");
*/
            /******************** Debug 3 end **********************/
		unsigned int acc_obj_size=acc_obj.size();
		unsigned int m_set_objs_size=m_set_objs.size();
		//Traverse through m_set_objs for conflicting objects
		for(unsigned int i=0;i<m_set_objs_size;i++){
			for(unsigned int j=0;j<acc_obj_size;j++){
				if(acc_obj[j]==m_set_objs[i]){
					//conflict found. Add Tx to n_set if it is not already in n_set
                                    /******************** Debug 3 start **********************/                                    
/*
                                    clock_gettime(CLOCK_REALTIME, &rec_time);
                                    sched_getparam(0, &param);
                                    rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "conflict found" << endl;
                                    rec.push_back(rec_in.str());
                                    rec_in.str("");                                    
*/
                                    /******************** Debug 3 end **********************/
					if(cur_state==released){
						addTxNset((void*)this);
						m_set=false;
					}
					cur_state=retrying;
                                        /******************** Debug 3 start **********************/                                        
/*
                                        clock_gettime(CLOCK_REALTIME, &rec_time);
                                        sched_getparam(0, &param);
                                        rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "end check_mset with conflict" << endl;
                                        rec.push_back(rec_in.str());
                                        rec_in.str("");                                         
*/
                                        /******************** Debug 3 end **********************/
					return false;
				}
			}
		}
		//No conflict found. Add accessed objects to m_set
		for(unsigned int i=0;i<acc_obj_size;i++){
			m_set_objs.push_back(acc_obj[i]);
		}
		//if Tx was is in n_set, remove it from there.
		if(cur_state==checking){
			n_set.erase(n_set.begin()+tx_pos);
		}
		m_set=true;
		cur_state=executing;
                /******************** Debug 3 start **********************/                
/*
                clock_gettime(CLOCK_REALTIME, &rec_time);
                sched_getparam(0, &param);
                rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "end checkMset with no conflict" << endl;
                rec.push_back(rec_in.str());
                rec_in.str("");               
*/
                /******************** Debug 3 end **********************/
		return true;
	}

	void addTxNset(void* cm_th){
	//Add cm_th to n_set according to its priority
	//No mutex used here because it is called with m_set access (either add or remove)
		unsigned int n_set_size=n_set.size();
		for(unsigned int i=0;i<n_set_size;i++){
			if(compTimeSpec(((PNF*)(cm_th))->getTimeParam(),((PNF*)(n_set[i]))->getTimeParam())){
				n_set.insert(n_set.begin()+i,cm_th);
				//Found righ position. Leave n_set
                                /******************** Debug 3 start **********************/                                
/*
                                clock_gettime(CLOCK_REALTIME, &rec_time);
                                sched_getparam(0, &param);
                                rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "add to n_set at " << i << endl;
                                rec.push_back(rec_in.str());
                                rec_in.str("");                                
*/
                                /******************** Debug 3 end **********************/
				return;
			}
		}	
		//cm_th is the last Tx in n_set
		n_set.push_back(cm_th);
                /******************** Debug 3 start **********************/                
/*
                clock_gettime(CLOCK_REALTIME, &rec_time);
                sched_getparam(0, &param);
                rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "add to n_set at " << (n_set.size()-1) << endl;
                rec.push_back(rec_in.str());
                rec_in.str("");               
*/
                /******************** Debug 3 end **********************/
	}

	bool compTimeSpec(struct timespec* f_ts,struct timespec* s_ts){
	//"true" if f_ts timespec is smaller than s_ts. "false" otherwise
		if((f_ts->tv_sec < s_ts->tv_sec) || ((f_ts->tv_sec == s_ts->tv_sec)&&(f_ts->tv_nsec < s_ts->tv_nsec))){
			return true;
		}
		else{
			return false;
		}
	}

	virtual void onBeginTransaction() {
	  try{
                /******************** Debug 3 start **********************/              
/*  
                clock_gettime(CLOCK_REALTIME, &rec_time);
                sched_getparam(0, &param);
                rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "startBeginTx" << endl;
                rec.push_back(rec_in.str());
                rec_in.str("");              
*/
                /******************** Debug 3 end **********************/
		clock_gettime(CLOCK_REALTIME, &stamp);
		if(cur_state==released){
			//Tx newely released
                    param.sched_priority=PNF_M_PRIO;
                    sched_setparam(0,&param);	//Increase priority until checking m
                    /********************* Debug 4 start *****************/
/*
                    rec_in.str("");
                    clock_gettime(CLOCK_REALTIME, &rec_time);
                    sched_getparam(0, &param);
                    rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "released" << endl;
                    rec.push_back(rec_in.str());
                    rec_in.str("");
*/
		    /******************* Debug 4 end ********************************/
                    //pthread_mutex_lock(&m_set_mutx);
                    mu_lock();
                    bool checkMset_st=checkMset(-1);        //-1 is a dummy variable
                    //pthread_mutex_unlock(&m_set_mutx);
                    mu_unlock();
                    if(!checkMset_st){
                        //Tx added to n_set. Reduce priority to lowest value
                        //If Tx becomes executing tx, then leave its priority at PNF_M_PRIO
                        //pthread_setschedprio(*th, PNF_N_PRIO);
                        param.sched_priority=PNF_N_PRIO;
                        sched_setparam(0,&param);
                    }
		}
		else if(cur_state==checking){
			//Tx is in n_set. But an executing tx finished. So, Tx is checking m_set again
			//pthread_setschedprio(*th, PNF_M_PRIO);	//Increase priority until checking m
			if(param.sched_priority!=PNF_M_PRIO){
				//increase priority of current thread before locking
				param.sched_priority=PNF_M_PRIO;
				sched_setparam(0,&param);
			}
                        /********************* Debug 4 start *****************/
/*
                        clock_gettime(CLOCK_REALTIME, &rec_time);
                        sched_getparam(0, &param);
                        rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "checking" << endl;
                        rec.push_back(rec_in.str());
                        rec_in.str("");
*/
                        /******************* Debug 4 end ********************************/

                        //pthread_mutex_lock(&m_set_mutx);
            mu_lock();
			unsigned int next_tx=nSetPos((void*)this)+1;		//Identify what is next Tx. Should be checked before
								//further use. Otherwise, it might exceed n_set size
								//This step should be done before "checkMset". Otherwise, 
								//Tx might be removed from n_set before knowing its previous position
			bool check_mset=checkMset(next_tx-1);	//Store checking result
			if(check_mset){
				//current transaction is not in n_set any more
				//next_tx should be reduced by 1
				next_tx--;
			}
			if(next_tx<n_set.size()){		//Haven't finished n_set yet
			//Prepare the next Tx in n_set for checking
				((PNF*)(n_set[next_tx]))->cur_state=checking;
				//pthread_setschedprio(*(((PNF*)(n_set[next_tx]))->th), RUN_PRIO);
                                param_tmp.sched_priority=PNF_M_PRIO;
                                sched_setparam(((PNF*)(n_set[next_tx]))->th, &param_tmp);
			}
                        //pthread_mutex_unlock(&m_set_mutx);
            mu_unlock();
			if(!check_mset){			//Tx still conflicts with m_set
				//Keep Tx as it is in n_set
				//pthread_setschedprio(*th, PNF_N_PRIO);
				param.sched_priority=PNF_N_PRIO;
				sched_setparam(0,&param);
			}
		}
		else{
			//DO NOTHING. Tx is either executing or retrying
                    /********************* Debug 4 start *****************/
/*
                    clock_gettime(CLOCK_REALTIME, &rec_time);
                    sched_getparam(0, &param);
                    if(cur_state==executing){
                            rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "executing" << endl;
                    }
                    else{
                            rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "retrying" << endl;
                    }
                    rec.push_back(rec_in.str());
                    rec_in.str("");
*/
                    /******************* Debug 4 end ********************************/

		}
		/********************************* Debug 5 start ********************************/
		//tra_start.push_back(stamp);
		/********************************* Debug 5 end ********************************/
                /******************** Debug 3 start **********************/                
/*
                clock_gettime(CLOCK_REALTIME, &rec_time);
                sched_getparam(0, &param);
                rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "endBeginTx" << endl;
                rec.push_back(rec_in.str());
                rec_in.str("");                
*/
                /******************** Debug 3 end **********************/
	    }catch(exception e){
	    	cout << "onBeginTransaction exception: " << e.what() << endl;
	    }
	}

	virtual void onTransactionCommitted() {
		try{
                    if(param.sched_priority!=PNF_M_PRIO){
                        //increase priority of current thread before locking
                        param.sched_priority=PNF_M_PRIO;
                        sched_setparam(0,&param);
                    }
                    /******************** Debug 3 start **********************/                    
/*
                    clock_gettime(CLOCK_REALTIME, &rec_time);
                    sched_getparam(0, &param);
                    rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "startTxCommitted" << endl;
                    rec.push_back(rec_in.str());
                    rec_in.str("");                    
*/
                    /******************** Debug 3 end **********************/
			//pthread_mutex_lock(&m_set_mutx);
                        mu_lock();
                        if(cur_state!=executing){
                            //current Tx is committing while retrying or checking (it happens
                            //because no other transaction conflicts with it). Current Tx is in 
                            //n_set. Remove it from there
                            n_set.erase(n_set.begin()+nSetPos((void*) this));
                        }
                        else{
                            //Remove accessed objects from m_set
			int acc_obj_size=acc_obj.size();
			int m_set_objs_size=m_set_objs.size();
			for(int j=0;j<acc_obj_size;j++){
				for(int i=0;i<m_set_objs_size;i++){
					if(acc_obj[j]==m_set_objs[i]){
						m_set_objs.erase(m_set_objs.begin()+i);
						m_set_objs_size--;
						break;
					}
				}
			}
                        }
                        
		//Change state of highest priority transaction to "checking" (if it exists), and increase its priority to "RUN_PRIO"
		//So, if processors available, n_set[0] can check itself against m_set.
		//Sequentially, following transactions in n_set can check conflict with m_set in their order in n_set
			if(!n_set.empty()){
				((PNF*)(n_set[(unsigned int) 0]))->cur_state=checking;
			//Increasing priority to "RUN_PRIO" is important. If n_set[0] has a lower priority than other executing tasks
			//then no processor is available to n_set[0]. Otherwise, n_set[0] can preempt the lowest priority thread
			//(lower than Tx itself) if all processors are occupied
				//pthread_setschedprio(*(((PNF*)(n_set[(unsigned int) 0]))->th), RUN_PRIO);
                                param_tmp.sched_priority=PNF_M_PRIO;
                                sched_setparam(((PNF*)(n_set[(unsigned int) 0]))->th, &param_tmp);
                                /******************** Debug 3 start **********************/                           
/*
                                clock_gettime(CLOCK_REALTIME, &rec_time);
                                //pthread_getschedparam((*th), &policy, &param);
                                sched_getparam(0,&param);
                                rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "increase " << (((PNF*)(n_set[(unsigned int) 0]))->th) << " to RUN_PRIO" << endl;
                                rec.push_back(rec_in.str());
                                rec_in.str("");                                 
*/
                                /******************** Debug 3 end **********************/
			}
                        /******************** Debug 3 start **********************/                        
/*
                        clock_gettime(CLOCK_REALTIME, &rec_time);
                        sched_getparam(0, &param);
                        rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "endTxCommitted" << endl;
                        rec.push_back(rec_in.str());
                        rec_in.str(""); 
                        rec_in<<"abort\t"<<total_abort_duration<<endl;
                        rec.push_back(rec_in.str());
                        rec_in.str("");
			//rec_final=rec;
			//rec=vector<string> ();
*/
                        /******************** Debug 3 end **********************/
                        //pthread_mutex_unlock(&m_set_mutx);
                        //Restore default values for m_set and cur_state. Otherwise, the next Tx
                        //will go on with the last values for these variables
			m_set=false;
			cur_state=released;
                        mu_unlock();
                        //Reduce priority of current task to RUN_PRIO because Tx finished
			//pthread_setschedprio(*th,RUN_PRIO);
		}catch(exception e){
			cout << "onTransactionCommitted exception: " << e.what() << endl;
		}
	}

      virtual void onTransactionAborted() {
	  try{
		clock_gettime(CLOCK_REALTIME, &tra_abort);
		total_abort_duration+=subtract_ts_mod(&stamp,&tra_abort);
                /******************** Debug 3 start **********************/                
/*
                sched_getparam(0, &param);
                rec_in << (tra_abort.tv_sec) << "\t" << (tra_abort.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << cur_state << "\t" << m_set << "\t" << "Aborted" << endl;
                rec.push_back(rec_in.str());
                rec_in.str("");                 
*/
                /******************** Debug 3 end **********************/
		/*************************** Debug 6 start ********************************/
		//tra_abr.push_back(tra_abort);
		//tra_int.push_back(subtract_ts_mod(&stamp,&tra_abort));
		/*************************** Debug 6 end ********************************/
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
	if(m_set==true && !other_mset){
		return AbortOther;
	}
	else if(!m_set && other_mset){
		return AbortSelf;
	}
	else if(m_set && other_mset){
		/*
		 * Due to timing issues, this case can arise by the following scenario:
		 * When a Tx commits, it erases its objects first, then it changes state of first Tx in n_set to
		 * "checking". Then, it changes its state to "released" and its m_set to "false". Between changing
		 * the state of first n_set Tx to "checking" and changing its own state to "released", the first "n_set"
		 * Tx can become executing because previous objects have been removed from m_objs set. In this case,
		 * two conflicting executing Tx exist. So, the first Tx must finish first. So, we resolve conflicts
		 * based on FIFO
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

/*
inline void stm::LCM::onBeginTransaction()
{
  try{
        clock_gettime(CLOCK_REALTIME, &stamp);

        //cout << "stamp:" << stamp.tv_sec << " sec, " << stamp.tv_nsec << " nsec" << endl;

    }catch(exception e){
        cout << "LCM:onBeginTransaction exception: " << e.what() << endl;
    }
}
*/

#endif // __LCM_H__
