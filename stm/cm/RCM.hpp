#ifndef __RCM_H__
#define __RCM_H__

#ifdef _MSC_VER
#include <time.h>
#include <winsock.h>
#else
#include <sys/time.h>
#endif

#include "ContentionManager.hpp"
#include <time.h>

namespace stm
{
  /**
   *  Compares the deadline tasks with conflicting transactions
   *  the lower absolute deadline wins
   */
  class RCM: public ContentionManager
  {
    private:
/*
      void onOpen(void)
      {
          defunct = false;
          tries = 0;
          max_tries = MAX_TRIES;
      }
*/
      ConflictResolutions shouldAbort(ContentionManager* enemy);

    public:
//      RCM(void* t_args){
//          time_param=*((struct timespec*)t_args);
//    }
	RCM()
		{}
      timespec* GetPeriod() {return &time_param;}
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
/*
            void onBeginTransaction(){
          clock_gettime(CLOCK_REALTIME, &stamp);
          rec_in << (stamp.tv_sec) << "\t" << (stamp.tv_nsec) << "\t" << th << "\t" << "startBeginTx" << endl;
          rec.push_back(rec_in.str());
          rec_in.str("");                    
          rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << "endBeginTx" << endl;
          rec.push_back(rec_in.str());
          rec_in.str("");                         
      }
      
      void onTransactionCommitted(){
          try{
          clock_gettime(CLOCK_REALTIME, &rec_time);
          //sched_getparam(0, &param);
          rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << "startTxCommitted" << endl;
          rec.push_back(rec_in.str());
          rec_in.str("");                    
	  //tra_start.push_back(stamp);
          rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << "endTxCommitted" << endl;
          rec.push_back(rec_in.str());
          rec_in.str("");                         
	  rec_in<<"abort\t"<<total_abort_duration<<endl;
          rec.push_back(rec_in.str());
          rec_in.str("");
          }catch(exception e){
		cout<<"onBeginTransaction exception: "<<e.what()<<endl;
	    }
      }
      
      void onTransactionAborted(){
          try{
              clock_gettime(CLOCK_REALTIME, &tra_abort);
              total_abort_duration+=subtract_ts_mod(&stamp,&tra_abort);
              //clock_gettime(CLOCK_REALTIME, &rec_time);
              //sched_getparam(0, &param);
              rec_time=tra_abort;
              rec_in << (rec_time.tv_sec) << "\t" << (rec_time.tv_nsec) << "\t" << th << "\t" << "Aborted" << endl;
              rec.push_back(rec_in.str());
              rec_in.str("");                         
          }catch(exception e){
              cout<<"onTransactionAborted exception: "<<e.what()<<endl;
          }
      }
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
stm::RCM::shouldAbort(ContentionManager* enemy)
{
    RCM* t=static_cast<RCM*>(enemy);
    timespec* other_period=t->GetPeriod();
    if(time_param.tv_sec < other_period->tv_sec){
	return AbortOther;
    }
    else if(time_param.tv_sec == other_period->tv_sec && time_param.tv_nsec < other_period->tv_nsec){
	return AbortOther;
    }
    //The following case needs more restrict criteria
    else if(time_param.tv_sec==other_period->tv_sec && time_param.tv_nsec==other_period->tv_nsec){
        return AbortOther;
    }
/*
    Timestamp* t = static_cast<Timestamp*>(enemy);
    defunct = false;

    if (!t)
        return AbortOther;

    // always abort younger transactions
    if (t->GetTimestamp() <= stamp)
        return AbortOther;

    // if it's been a while, mark the enemy defunct
    if (tries == (max_tries/2)) {
        t->SetDefunct();
    }
    // at some point, finally give up and abort the enemy
    else if (tries == max_tries) {
        return AbortOther;
    }
    // if the enemy was marked defunct and isn't anymore, then we reset a bit
    else if ((tries > (max_tries/2)) && (t->GetDefunct() == false)) {
        tries = 2;
        max_tries *= 2;
    }
*/
    return AbortSelf;
}
/*
inline void stm::RCM::onBeginTransaction()
{
  try{
        clock_gettime(CLOCK_REALTIME, &stamp);

        //cout<<"stamp:"<<stamp.tv_sec<<" sec, "<<stamp.tv_nsec<<" nsec"<<endl;

    }catch(exception e){
        cout<<"RCM:onBeginTransaction exception: "<<e.what()<<endl;
    }
}
*/
/*
inline void stm::Timestamp::onBeginTransaction()
{
    struct timeval t;

    defunct = false;
    gettimeofday(&t, NULL);
    stamp = t.tv_sec;
}
*/

#endif // __RCM_H__
