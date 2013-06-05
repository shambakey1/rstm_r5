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

#ifndef COUNTER_HPP__
#define COUNTER_HPP__

#include <stm/stm.hpp>
#include "Benchmark.hpp"
#include <iostream>
/************************* SH-START ***********************/
#include <string>
#include <vector>
/************************* SH-END ***********************/

// the type being counted
#define MYTYPE int

/******************* SH-START *******************/
using namespace std;

/******************* SH-END **********************/

namespace bench
{
    class Counter : public stm::Object
    {
        GENERATE_FIELD(MYTYPE, value);

      public:
	
        Counter(MYTYPE startingValue = 0) : m_value(startingValue) {
	}
    };



    class CounterBench : public Benchmark
    {
      private:

        stm::sh_ptr<Counter> m_counter;
	/******************** SH-START *************************/
	int id;
	/******************** SH-END *************************/

      public:

	/************************** SH-START ***************************/
	volatile unsigned long free_value;		//Maybe obselete if multiobject implementation works
	volatile unsigned long* free_value_ptr;		//Maybe obselete if multiobject implementation works
	/************************** SH-END ***************************/
	/************************ MULTIOBJECT-START ****************************/
	stm::sh_ptr<Counter>* m_v_counter;		//Used for multiple objects per STM transaction
	stm::wr_ptr<Counter>* m_w_counter;		//Used to write objects in m_v_counter
	volatile unsigned long* m_free_value_ptr;	//Used pointer for multiple objects for lock-free
	/************************ MULTIOBJECT-END ****************************/

//        CounterBench() : m_counter(new Counter()),	//This constructor is for single object per transaction
//                id(0)					//This constructor maybe obselete if multiobject implementation works

//        {
		/********************* SH-START ********************************/
//		free_value=0;
//		free_value_ptr=&free_value;
		/********************* SH-END ********************************/
//	}

	/************************ MULTIOBJECT-START ****************************/
        CounterBench() : id(0)		//This constructor for multi-objects per transaction

        {}

	void setMaxObjNo(int max_obj_num){
            //max_obj_num is maximum number of shared objects
            //Allocate right size for object arrays
		m_v_counter=new stm::sh_ptr<Counter> [max_obj_num];
		m_w_counter=new stm::wr_ptr<Counter> [max_obj_num];
		m_free_value_ptr=new volatile unsigned long[max_obj_num];
		//Initialize multiobject for STM and lock-free
		for(int i=0;i<max_obj_num;i++){
			/**********************************************************
				DON'T FORGET TO UNCOMMENT THE FOLLOWING TWO LINES
					COMMENT IS ONLY FOR DEBUGGING
			**********************************************************/
			m_v_counter[i]=stm::sh_ptr<Counter> (new Counter());	//initialize shared pointers for counters
//			m_w_counter[i]=stm::wr_ptr<Counter> (m_v_counter[i]);	//Initialize writable pointers for counters
			m_free_value_ptr[i]=0;
		}
        }

	void multi_reset_advance(unsigned int num_loops,void* t,vector<double> obj_no,int th,double wr_per){
		/*
		 * Must know objects before beginning tx like in PNF
		 */
		unsigned int tmp_size=obj_no.size();
		unsigned int loop_itr=1;		//Holds loop iteration number
		unsigned int itr_per_obj=num_loops<tmp_size?1:num_loops/tmp_size;	//No of iterations per each object in case of CHECKPOINTING
		unsigned int obj_indx=0;
		BEGIN_TRANSACTION_M(t,obj_no,th);
		for(unsigned int i=0;i<tmp_size;i++){
			m_w_counter[(unsigned int)(obj_no[i])]=stm::wr_ptr<Counter> (m_v_counter[(unsigned int)(obj_no[i])]);
		}
		while(loop_itr<=num_loops){
		//Traverse through counters specified by obj_no. In each iteration set one of them to 0
			if((loop_itr%100)/100.0 < wr_per){
			//write operation
				m_w_counter[(unsigned int)(obj_no[obj_indx])]->set_value(0,m_w_counter[(unsigned int)(obj_no[obj_indx])]);
			}
			else{
			//read operation
				m_w_counter[(unsigned int)(obj_no[obj_indx])]->get_value(m_w_counter[(unsigned int)(obj_no[obj_indx])]);
			}
			if(loop_itr%itr_per_obj==0 && num_loops-loop_itr>itr_per_obj){
				obj_indx++;
				obj_indx=obj_indx>tmp_size?tmp_size:obj_indx;
			}
			loop_itr++;
		}
		END_TRANSACTION;
	}

        void multi_reset(unsigned int num_loops,void* t,vector<double> obj_no,int th,double wr_per){
			//obj_no specifies which objects to set values
			//wr_per is "write" percentage. "1-wr_per" will be the "read" percentage
			unsigned int tmp_size=obj_no.size();
			unsigned int loop_itr=1;		//Holds loop iteration number
			unsigned int obj_indx=0;
			unsigned int itr_per_obj=num_loops<tmp_size?1:num_loops/tmp_size;	//No of iterations per each object in case of CHECKPOINTING
			BEGIN_TRANSACTION_M(t,obj_no,th);
			/*
			 * In case of CHECKPOINTING, the whole transactional length is divided over
			 * number of objects, so there is a time interval between each object access
			 */
			while(loop_itr<=num_loops){
			//Traverse through counters specified by obj_no. In each iteration set one of them to 0
				m_w_counter[(unsigned int)(obj_no[obj_indx])]=stm::wr_ptr<Counter> (m_v_counter[(unsigned int)(obj_no[obj_indx])]);
				if((loop_itr%100)/100.0 < wr_per){
				//write operation
					m_w_counter[(unsigned int)(obj_no[obj_indx])]->set_value(0,m_w_counter[(unsigned int)(obj_no[obj_indx])]);
				}
				else{
				//read operation
					m_w_counter[(unsigned int)(obj_no[obj_indx])]->get_value(m_w_counter[(unsigned int)(obj_no[obj_indx])]);
				}
				if(loop_itr%itr_per_obj==0 && num_loops-loop_itr>itr_per_obj){
					obj_indx++;
					obj_indx=obj_indx>tmp_size?tmp_size:obj_indx;
				}
				loop_itr++;
			}
			END_TRANSACTION;
        }
        
        void lcm_multi_reset(unsigned int num_loops,double psy,unsigned long exec,void* t,vector<double> obj_no,int th,double wr_per){
		//obj_no specifies which objects to set values
		//wr_per is "write" percentage. "1-wr_per" will be the "read" percentage
		unsigned int tmp_size=obj_no.size();
		unsigned int loop_itr=1;		//Holds loop iteration number
		unsigned int obj_indx=0;
		unsigned int itr_per_obj=num_loops<tmp_size?1:num_loops/tmp_size;	//No of iterations per each object in case of CHECKPOINTING
		BEGIN_TRANSACTION_LCM(psy,exec,t,obj_no,th);
		/*
		 * In case of CHECKPOINTING, the whole transactional length is divided over
		 * number of objects, so there is a time interval between each object access
		 */
		while(loop_itr<=num_loops){
		//Traverse through counters specified by obj_no. In each iteration set one of them to 0
			m_w_counter[(unsigned int)(obj_no[obj_indx])]=stm::wr_ptr<Counter> (m_v_counter[(unsigned int)(obj_no[obj_indx])]);
			if((loop_itr%100)/100.0 < wr_per){
			//write operation
				m_w_counter[(unsigned int)(obj_no[obj_indx])]->set_value(0,m_w_counter[(unsigned int)(obj_no[obj_indx])]);
			}
			else{
			//read operation
				m_w_counter[(unsigned int)(obj_no[obj_indx])]->get_value(m_w_counter[(unsigned int)(obj_no[obj_indx])]);
			}
			if(loop_itr%itr_per_obj==0 && num_loops-loop_itr>itr_per_obj){
				obj_indx++;
				obj_indx=obj_indx>tmp_size?tmp_size:obj_indx;
			}
			loop_itr++;
		}
		END_TRANSACTION;
        }

        void multi_reset_lock(unsigned int num_loops,unsigned long amount,vector<double> obj_no,double wr_per){
        	/*
        	 * For locking implementation. obj_no specifies which objects to set values.
        	 * wr_per is "write" percentage. 1-wr_per is read percentage
        	 */
        	unsigned int tmp_size=obj_no.size();
			unsigned int loop_itr=1;		//Holds loop iteration number
			unsigned int obj_indx=0;
			unsigned long read_val;
			unsigned int itr_per_obj=num_loops<tmp_size?1:num_loops/tmp_size;
			while(loop_itr<num_loops){
			//Traverse through "free_value"s specified by obj_no. In each iteration set one of them to amount
				if((loop_itr%100)/100.0 < wr_per){
				//write operation
					m_free_value_ptr[(int)(obj_no[obj_indx])]=amount;
				}
				else{
				//read operation
					read_val=m_free_value_ptr[(int)(obj_no[obj_indx])];
				}
				if(loop_itr%itr_per_obj==0 && (num_loops-loop_itr)>itr_per_obj){
					obj_indx++;
					obj_indx=obj_indx>tmp_size?tmp_size:obj_indx;
				}
				loop_itr++;
			}
		}

	
	vector<unsigned long long> multi_reset_lock_free(unsigned int num_loops,unsigned long amount,vector<double> obj_no,double wr_per){
		/*
		 * For lock-free implementation. obj_no specifies which objects to set values. wr_per is "write"
		 * percentage. 1-wr_per is read percentage. Return vector contains number of success access to
		 * object, number of failure access to object, and time for failure access.
		 */
		unsigned int tmp_size=obj_no.size();
		unsigned int loop_itr=1;		//Holds loop iteration number
		unsigned int obj_indx=0;
		unsigned long tmp_res;
		vector<unsigned long long> fin_res, med_res;	//final and intermediate result vectors
		fin_res.push_back(0);	//initial number of success access to object
		fin_res.push_back(0);	//initial number of failed access to object
		fin_res.push_back(0);	//initial time for failed access to object
		unsigned int itr_per_obj=num_loops<tmp_size?1:num_loops/tmp_size;
		while(loop_itr<num_loops){
			//Traverse through "free_value"s specified by obj_no. In each iteration set one of them to amount
			if((loop_itr%100)/100.0 < wr_per){
			//write operation
				med_res=fas(&(m_free_value_ptr[(int)(obj_no[obj_indx])]),amount);
				fin_res[1]+=med_res[0];
				fin_res[2]+=med_res[1];
			}
			else{
			//read operation
				tmp_res=m_free_value_ptr[(int)(obj_no[obj_indx])];
			}
			if(loop_itr%itr_per_obj==0 && num_loops-loop_itr>itr_per_obj){
				obj_indx++;
			}
			loop_itr++;
			obj_indx=obj_indx>tmp_size?tmp_size:obj_indx;
		}
		fin_res[0]=num_loops;
		return fin_res;
	}

	void multi_reset_lock_free(unsigned int num_loops,unsigned long amount,vector<double> obj_no){
	//For lock-free implementation
	//so it can be used to determine number of retrial loops
	//obj_no specifies which objects to set values
		unsigned int tmp_size=obj_no.size();
		unsigned int loop_itr=1;		//Holds loop iteration number
		unsigned int obj_indx=0;
		unsigned int itr_per_obj=num_loops<tmp_size?1:num_loops/tmp_size;
		while(loop_itr<num_loops){
		//Traverse through "free_value"s specified by obj_no. In each iteration set one of them to amount
			fas(&(m_free_value_ptr[(int)(obj_no[obj_indx])]),amount);
			if(loop_itr%itr_per_obj==0 && num_loops-loop_itr>itr_per_obj){
				obj_indx++;
				obj_indx=obj_indx>tmp_size?tmp_size:obj_indx;
			}
			loop_itr++;
		}
	}
	/************************ MULTIOBJECT-END ****************************/

        void random_transaction(thread_args_t* args, unsigned int* seed,
                                unsigned int val,    int chance)
        {
            BEGIN_TRANSACTION;
            stm::wr_ptr<Counter> wr(m_counter);
            wr->set_value(wr->get_value(wr) + 1, wr);
            END_TRANSACTION;
        }
        /*********************************** SH-START *****************************************/
	//All reset fns in this section between SH-START and SH-END may be obselete if multi-object reset fns work
/*
        void reset(void* t){
            BEGIN_TRANSACTION_M(t);
            stm::wr_ptr<Counter> wr(m_counter);
            wr->set_value(0, wr);
            END_TRANSACTION;
        }

        void multi_reset(int num_loops,void* t){
            BEGIN_TRANSACTION_M(t);
            stm::wr_ptr<Counter> wr(m_counter);
            for(int i=0;i<num_loops;i++){
                wr->set_value(0, wr);
            }
            END_TRANSACTION;
        }
	
	unsigned long long multi_reset_lock_free(int num_loops,unsigned long amount,unsigned long long th_ptr){
	//For lock-free implementation
	//th_ptr is a counter insided each thread (or task) to calculate the total number of loops including retrial
	//so it can be used to determine number of retrial loops
		for(int i=0;i<num_loops;i++){
			fas(free_value_ptr,amount,&th_ptr);
		}
		return th_ptr;
	}

	void multi_reset_lock_free(int num_loops,unsigned long amount){
	//For lock-free implementation
	//so it can be used to determine number of retrial loops
		for(int i=0;i<num_loops;i++){
			fas(free_value_ptr,amount);
		}
	}

        void lcm_multi_reset(int num_loops,double psy,unsigned long exec,void* t){
            BEGIN_TRANSACTION_LCM(psy,exec,t);
            stm::wr_ptr<Counter> wr(m_counter);
            for(int i=0;i<num_loops;i++){
                wr->set_value(0, wr);
            }
            END_TRANSACTION;
        }
*/
	void setID(int i){
		id=i;
	}

	int getID(){
		return id;
	}
        /*********************************** SH-END *****************************************/

        bool sanity_check() const
        {
            // not as useful as it could be...
            MYTYPE val = 0;
            BEGIN_TRANSACTION;
            stm::rd_ptr<Counter> rd(m_counter);
            val = rd->get_value(rd);
            END_TRANSACTION;
            std::cout << "final value = " << val << std::endl;
            return (val > 0);
        }


        // no data structure verification is implemented for the Counter...
        // yet
        virtual bool verify(VerifyLevel_t v) {
            return true;
        }
    };

}   // namespace bench

#endif  // COUNTER_HPP__
