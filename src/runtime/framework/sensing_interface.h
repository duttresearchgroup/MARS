/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef __arm_rt_framework_sensing_interface_h
#define __arm_rt_framework_sensing_interface_h

#include <type_traits>

#include "types.h"
#include <base/base.h>

class SensingWindowManager;

struct SensingInterface {
    friend class SensingWindowManager;

	/*
	 * Returns the sensed value for given window (or the current window if not specified).
	 * When sense is called in the reflecting context (e.g. when a policy is run as
	 * a model), sense behaves like senseIf
	 */
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT *rsc, int wid)
	{
	    if(_currentContext.reflecting) {
	        assert_true(wid == _currentContext.wid);
	        return senseIf<SEN_T,ResourceT>(rsc);
	    }
	    else
	        return Impl::sense<SEN_T,ResourceT>(rsc,wid);
	}
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT *rsc)
	{
	    if(_currentContext.reflecting)
	        return senseIf<SEN_T,ResourceT>(rsc);
	    else
	        return Impl::sense<SEN_T,ResourceT>(rsc,_currentContext.wid);
	}
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid)
	{
	    if(_currentContext.reflecting) {
	        assert_true(wid == _currentContext.wid);
	        return senseIf<SEN_T,ResourceT>(p,rsc);
	    }
	    else
	        return Impl::sense<SEN_T,ResourceT>(p,rsc,wid);
	}
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
	{
	    if(_currentContext.reflecting)
	        return senseIf<SEN_T,ResourceT>(p,rsc);
	    else
	        return Impl::sense<SEN_T,ResourceT>(p,rsc,_currentContext.wid);
	}


    /*
     * Returns aggregated sensed value for the
     * current and previous instances of the given window
     * (or the current window if not specified)
     */
    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT *rsc, int wid)
    {
        if(_currentContext.reflecting) {
            assert_true(wid == _currentContext.wid);
            return senseAggIf<SEN_T,ResourceT>(rsc);
        }
        else
            return Impl::senseAgg<SEN_T,ResourceT>(rsc,wid);
    }
    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT *rsc)
    {
        if(_currentContext.reflecting)
            return senseAggIf<SEN_T,ResourceT>(rsc);
        else
            return Impl::senseAgg<SEN_T,ResourceT>(rsc,_currentContext.wid);
    }
    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid)
    {
        if(_currentContext.reflecting) {
            assert_true(wid == _currentContext.wid);
            return senseAggIf<SEN_T,ResourceT>(p,rsc);
        }
        else
            return Impl::senseAgg<SEN_T,ResourceT>(p,rsc,wid);
    }
    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
    {
        if(_currentContext.reflecting)
            return senseAggIf<SEN_T,ResourceT>(p,rsc);
        else
            return Impl::senseAgg<SEN_T,ResourceT>(p,rsc,_currentContext.wid);
    }



    /*
     * Returns the predicted sensed data for the current window.
     * If tryActuate was not called, or calls to tryActuate does not affect
     * the requested sensed data, then this return the same as sense.
     */
    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseIf(const ResourceT *rsc)
    {
        return Reflective::sense<SEN_T,ResourceT>(rsc);
    }

    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseIf(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
    {
        return Reflective::sense<SEN_T,ResourceT>(p,rsc);
    }

    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAggIf(const ResourceT *rsc)
    {
        return Reflective::senseAgg<SEN_T,ResourceT>(rsc);
    }

    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAggIf(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
    {
        return Reflective::senseAgg<SEN_T,ResourceT>(p,rsc);
    }

    static int currentWID() { return _currentContext.wid; }

    static int isReflecting() { return _currentContext.reflecting; }

  private:

	/*
	 * These are the functions that need to be specialized
	 */
	struct Impl {
	    /*
	     * Returns the sensed value for given window
	     */
	    template<SensingType SEN_T,typename ResourceT>
	    static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT *rsc, int wid);

	    template<SensingType SEN_T,typename ResourceT>
	    static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid);


	    /*
	     * Returns aggregated sensed value for the
	     * current and previous instances of the given window
	     */
	    template<SensingType SEN_T,typename ResourceT>
	    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT *rsc, int wid);

	    template<SensingType SEN_T,typename ResourceT>
	    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid);
	};

    /*
     * These are the functions that need to be specialized
     */
    struct Reflective {

        template<SensingType SEN_T,typename ResourceT>
        static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT *rsc)
        {
            arm_throw(SensingInterfaceException,"Not implemented");
        }

        template<SensingType SEN_T,typename ResourceT>
        static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
        {
            arm_throw(SensingInterfaceException,"Not implemented");
        }

        template<SensingType SEN_T,typename ResourceT>
        static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT *rsc)
        {
            arm_throw(SensingInterfaceException,"Not implemented");
        }

        template<SensingType SEN_T,typename ResourceT>
        static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
        {
            arm_throw(SensingInterfaceException,"Not implemented");
        }
    };

	struct SensingContext {
	    // Current window id
	    int wid;

	    // If true, sense returns predicted sensed values given an actuation
	    // made by tryActuate.
	    // Otherwise, returns real sensed data
	    bool reflecting;

	    // Timestamp incremented at every window.
	    int timestamp;
	};

	static thread_local SensingContext _currentContext;

	static SensingContext& getCurrentContext() { return _currentContext; }
};

#endif /* ACTUATOR_H_ */
