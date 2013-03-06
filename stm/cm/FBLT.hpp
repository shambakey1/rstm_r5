#ifndef __FBLT_H__
#define __FBLT_H__

#ifdef _MSC_VER
#include <time.h>
#include <winsock.h>
#else
//#include <sys/time.h>
#endif

#include "ContentionManager.hpp"
#include <time.h>
#include <iostream>
#include <sstream>
#include <pthread.h>
#include <chronos/chronos.h>
#include <chronos/chronos_utils.h>
#include <cmath>
#include <limits.h>
using namespace std;

namespace stm
{
  /**
   *  Compares the deadline tasks with conflicting transactions
   *  the lower absolute deadline wins
   */
    
    struct fblt_args{
    unsigned long length;    //execution time of the atomic section
    double psy;                 //threshold value used to calculate alpha
    };
    
  class FBLT: public ContentionManager
  {
    private:
        

      ConflictResolutions shouldAbort(ContentionManager* enemy);

    public:
	
	bool m_set;                     //If true, current transaction is in m_set
        struct timespec m_set_join;     //Records time of joining current transaction to m_set
        struct fblt_args args;
        struct timespec now;
        
        FBLT(){
            new_tx=true;
	    m_set=false;
	    eta=-1;
            //Initilize m_set_join to dummy value
            m_set_join.tv_sec=LONG_MAX;
            m_set_join.tv_nsec=LONG_MAX;
        }
        
	FBLT(void* t_args){
		new_tx=true;
		m_set=false;
		eta=-1;
                //Initilize m_set_join to dummy value
                m_set_join.tv_sec=LONG_MAX;
                m_set_join.tv_nsec=LONG_MAX;
                args=*((struct fblt_args*)t_args);
                length=args.length;
                psy=args.psy;
                
	}
      
      //The inherited time_param is the input deadline
      timespec* GetDeadline() {return &time_param;}
      timespec* GetTimestamp() { return &stamp; }
      struct fblt_args* getFBLTargs(){
          return &args;
      }

      void onBeginTransaction(){
        clock_gettime(CLOCK_REALTIME, &stamp); 
	if((!m_set) && (eta==0)){
            //End work in real time priority to increase priority of current transaction
            //First time to include current transaction in m_set
            //set m_set to true and increase priority of current transaction
            end_rtseg_self(PNF_M_PRIO);
            m_set=true;
            //Record time of joining m_set to be used in conflict resolution
            clock_gettime(CLOCK_REALTIME, &m_set_join);
	}
		/******************** Debug 3 start **********************/              
/* 
                clock_gettime(CLOCK_REALTIME, &rec_time);
                sched_getparam(0, &param);
                rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << m_set << "\t" <<eta<<"\t"<<m_set_join.tv_sec<<"\t"<<m_set_join.tv_nsec<<"\t"<< "startBeginTx" << endl;
                rec.push_back(rec_in.str());
                rec_in.str("");              
*/
                /******************** Debug 3 end **********************/
      }
      
      void onTransactionCommitted(){
          try{
//	      eta=-1;
	      new_tx=true;
              //If current tx is in m_set, reduce its priority to real-time task
              if(m_set){
                  m_set=false;
		  m_set_join.tv_sec=LONG_MAX;
		  m_set_join.tv_nsec=LONG_MAX;
                  begin_rtseg_self(task_run_prio, task_util, task_deadline, task_period,
		task_unlocked + task_locked);
              }
                /******************** Debug 3 start **********************/
/*  
                clock_gettime(CLOCK_REALTIME, &rec_time);
                sched_getparam(0, &param);
                rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << m_set << "\t" <<eta<<"\t"<<m_set_join.tv_sec<<"\t"<<m_set_join.tv_nsec<<"\t"<< "committingTx" << endl;
                rec.push_back(rec_in.str());
                rec_in.str("");              
*/
                /******************** Debug 3 end **********************/
          }catch(exception e){
		cout<<"onTxCommitted exception: "<<e.what()<<endl;
	    }
      }
      
      void onTransactionAborted(){
          try{
              clock_gettime(CLOCK_REALTIME, &tra_abort);
              total_abort_duration+=subtract_ts_mod(&stamp,&tra_abort);
	      if(eta>0){
		eta--;
	      }
                /******************** Debug 3 start **********************/
/*  
                clock_gettime(CLOCK_REALTIME, &rec_time);
                sched_getparam(0, &param);
                rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << (param.sched_priority) << "\t" << m_set << "\t" <<eta<<"\t"<<m_set_join.tv_sec<<"\t"<<m_set_join.tv_nsec<<"\t"<<total_abort_duration<<"\t"<< "AbortTx" << endl;
                rec.push_back(rec_in.str());
                rec_in.str("");              
*/
                /******************** Debug 3 end **********************/

          }catch(exception e){
              cout<<"onTransactionAborted exception: "<<e.what()<<endl;
          }
      }

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
      long double checkAlpha(struct FBLT*,struct FBLT*);
      long double getPassed();
  };
}; // namespace stm

inline stm::ConflictResolutions
stm::FBLT::shouldAbort(ContentionManager* enemy)
{
    //struct timespec* deadline=this->GetDeadline();
    FBLT* t=static_cast<FBLT*>(enemy);
    timespec* other_time_param=t->getTimeParam();
    bool other_m_set=t->m_set;
    timespec other_m_set_join=t->m_set_join;
    struct timespec* other_timestamp=t->GetTimestamp();
    long double alpha,passed;   //alpha is the threshold of the interfered transaction
                                // passed is the percentage of already executed part of the interfered transaction
    if(m_set){
        if(!other_m_set){
            return AbortOther;
        }else{
            if(m_set_join.tv_sec<other_m_set_join.tv_sec){
                return AbortOther;
            }else if(m_set_join.tv_sec==other_m_set_join.tv_sec && m_set_join.tv_nsec<other_m_set_join.tv_nsec){
                return AbortOther;
            }
            else{
                return AbortSelf;
            }
        }
    }
    else{
        if(other_m_set){
            return AbortSelf;
        }else{
            //Normal comparison using priority. Default to LCM
            if(time_param.tv_sec<other_time_param->tv_sec || (time_param.tv_sec==other_time_param->tv_sec && time_param.tv_nsec<=other_time_param->tv_nsec)){
                //If current transaction started BEFORE the other one
                if(stamp.tv_sec<other_timestamp->tv_sec || (stamp.tv_sec==other_timestamp->tv_sec && stamp.tv_nsec <= other_timestamp->tv_nsec)){
                    return AbortOther;
                }
                //If current transaction started AFTER the other
                else{
                        //Needs to check alpha
                        alpha=checkAlpha((FBLT*)enemy,this); //Alpha of the enemy
                        passed=((FBLT*)enemy)->getPassed();  //passed of the enemy
                        if(passed<=alpha){
                                return AbortOther;
                        }
                }
             }
             //If current task has a LOWER priority
             else{
                //If current transaction started AFTER the other
                if(stamp.tv_sec>other_timestamp->tv_sec || (stamp.tv_sec==other_timestamp->tv_sec && stamp.tv_nsec > other_timestamp->tv_nsec)){
                        return AbortSelf;
                }
                //If current transaction started BEFORE the other
                else{
                        //Needs to check alpha
                        alpha=checkAlpha(this,(FBLT*)enemy); //alpha of current LCM
                        passed=this->getPassed();           //passed of current LCM
                        if(passed>alpha){
                        return AbortOther;
                }
                }
             }
    return AbortSelf;
        }
    }
}

inline long double stm::FBLT::checkAlpha(struct FBLT* first,struct FBLT* second){
    //This function returns the alpha percentage between the two transactions
    //first is the one with lower priority and started before the second
    double c=(double)second->getLength()/first->getLength();
    return (double)log(second->getPsy())/(log(second->getPsy())-c);
}

inline long double stm::FBLT::getPassed(){
    //This function returns the percentage of the already executed lenght of the current transaction
    //relative to the total length of the current transaction
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return ((long double)subtract_ts_mod(&stamp,&now)/(this->getLength()))/1000;//Note that subtract_ts produces result in n_sec but length is in u_sec
}
#endif // __FBLT_H__
