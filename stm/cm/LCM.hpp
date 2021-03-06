// THE WHOLE FILE NEEDS TO BE WRITTEN BECAUSE IT CURRENTLY ACTS LIKE RCM
// SO THIS CODE IS NOT THE ACTUAL IMPLEMENTATION OF LCM, IT IS ONLY A PLACEHOLDER SO
// RSTM CAN BE COMBILED WITHOUT BUGS
#ifndef __LCM_H__
#define __LCM_H__

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
#include <chronos/chronos_utils.h>
#include <cmath>


namespace stm
{
    struct lcm_args{
    unsigned long length;    //execution time of the atomic section
    double psy;                 //threshold value used to calculate alpha
    };
	

  /**
   *  Compares the deadline tasks with conflicting transactions
   *  the lower absolute deadline wins
   */
  class LCM: public ContentionManager
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
        struct lcm_args args;
        //struct timespec stamp;
        struct timespec now;
	//unsigned long length;
	//double psy;
	

        ConflictResolutions shouldAbort(ContentionManager* enemy);

    public:

	LCM(){
	}

      LCM(void* t_args){
	args=*((struct lcm_args*)t_args);
	length=args.length;
	psy=args.psy;
    }
      timespec* GetTimestamp() { return &stamp; }
      struct lcm_args* getLCMargs(){
          return &args;
      }
/*
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
*/

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
      long double checkAlpha(struct LCM*,struct LCM*);
      long double getPassed();
  };
}; // namespace stm

inline stm::ConflictResolutions
stm::LCM::shouldAbort(ContentionManager* enemy)
{
    LCM* t=static_cast<LCM*>(enemy);
    struct timespec* other_timestamp=t->GetTimestamp();
    struct timespec* other_time_param=t->getTimeParam();
    long double alpha,passed;//alpha is the threshold of the interfered transaction
                        // passed is the percentage of already executed part of the interfered transaction
    //If current task has a HIGHER priority
    if(time_param.tv_sec<other_time_param->tv_sec || (time_param.tv_sec==other_time_param->tv_sec && time_param.tv_nsec<=other_time_param->tv_nsec)){
        //If current transaction started BEFORE the other one
        if(stamp.tv_sec<other_timestamp->tv_sec || (stamp.tv_sec==other_timestamp->tv_sec && stamp.tv_nsec <= other_timestamp->tv_nsec)){
            return AbortOther;
        }
        //If current transaction started AFTER the other
        else{
            //Needs to check alpha
            alpha=checkAlpha((LCM*)enemy,this); //Alpha of the enemy
            passed=((LCM*)enemy)->getPassed();  //passed of the enemy
            if(passed<=alpha){
                /********************* Debug 1 start *************************/
                //cout<<"c_d:"<<time_param.tv_sec<<" sec, "<<time_param.tv_nsec<<" nsec, o_d:"<<other_time_param->tv_sec<<" sec, "<<other_time_param->tv_nsec<<" nsec, o_alpha:"<<alpha<<", o_passed:"<<passed<<", abort_other"<<endl;
                /********************* Debug 1 end *************************/
                return AbortOther;
            }
            else{
                /********************* Debug 2 start *************************/
                //cout<<"c_d:"<<time_param.tv_sec<<" sec, "<<time_param.tv_nsec<<" nsec, o_d:"<<other_time_param->tv_sec<<" sec, "<<other_time_param->tv_nsec<<" nsec, o_alpha:"<<alpha<<", o_passed:"<<passed<<", abort_me"<<endl;
                /********************* Debug 2 end *************************/
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
            alpha=checkAlpha(this,(LCM*)enemy); //alpha of current LCM
            passed=this->getPassed();           //passed of current LCM
            if(passed>alpha){
                /********************* Debug 3 start *************************/
                //cout<<"c_d:"<<time_param.tv_sec<<" sec, "<<time_param.tv_nsec<<" nsec, o_d:"<<other_time_param->tv_sec<<" sec, "<<other_time_param->tv_nsec<<" nsec, c_alpha:"<<alpha<<", c_passed:"<<passed<<", abort_other"<<endl;
                /********************* Debug 3 end *************************/
                return AbortOther;
            }
            else{
                /********************* Debug 4 start *************************/
                //cout<<"c_d:"<<time_param.tv_sec<<" sec, "<<time_param.tv_nsec<<" nsec, o_d:"<<other_time_param->tv_sec<<" sec, "<<other_time_param->tv_nsec<<" nsec, c_alpha:"<<alpha<<", c_passed:"<<passed<<", abort_me"<<endl;
                /********************* Debug 4 end *************************/
            }
        }
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

inline long double stm::LCM::checkAlpha(struct LCM* first,struct LCM* second){
    //This function returns the alpha percentage between the two transactions
    //first is the one with lower priority and started before the second
    double c=(double)second->getLength()/first->getLength();
    return (double)log(second->getPsy())/(log(second->getPsy())-c);
}

inline long double stm::LCM::getPassed(){
    //This function returns the percentage of the already executed lenght of the current transaction
    //relative to the total length of the current transaction
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    /***************************** Debug 5 start **********************************/
    /*
    double diff=subtract_ts(&stamp,&now);
    double result=(diff/(this->args.length))/1000;
    cout<<"stamp:"<<stamp.tv_sec<<" sec, "<<stamp.tv_nsec<<" nsec"<<", now:"<<now.tv_sec<<" sec, "<<now.tv_nsec<<" nsec"<<", diff:"<<diff<<", length:"<<this->args.length<<", passed:"<<result<<endl;
    */
    /***************************** Debug 5 end **********************************/
    return ((long double)subtract_ts(&stamp,&now)/(this->getLength()))/1000;//Note that subtract_ts produces result in n_sec but length is in u_sec
    //return result;
}
/*
inline void stm::LCM::onBeginTransaction()
{
  try{
        clock_gettime(CLOCK_REALTIME, &stamp);

        //cout<<"stamp:"<<stamp.tv_sec<<" sec, "<<stamp.tv_nsec<<" nsec"<<endl;

    }catch(exception e){
        cout<<"LCM:onBeginTransaction exception: "<<e.what()<<endl;
    }
}
*/

#endif // __LCM_H__
