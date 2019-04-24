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
#include "reflective.h"
#include "sensing_interface_impl.h"
#include <base/base.h>

class SensingWindowManager;

struct SensingInterface : SensingInterfaceImpl {

    friend class ReflectiveEngine;

	/*
	 * Returns the sensed value for given window (or the current window if not specified).
	 * When sense is called in the reflecting context (e.g. when a policy is run as
	 * a model), sense behaves like senseIf
	 */
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT *rsc, int wid)
	{
	    if(ReflectiveEngine::isReflecting()) {
	        assert_true(wid == ReflectiveEngine::currentWID());
	        return senseIf<SEN_T,ResourceT>(rsc);
	    }
	    else
	        return Impl::sense<SEN_T,ResourceT>(rsc,wid);
	}
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT *rsc)
	{
	    if(ReflectiveEngine::isReflecting())
	        return senseIf<SEN_T,ResourceT>(rsc);
	    else
	        return Impl::sense<SEN_T,ResourceT>(rsc,ReflectiveEngine::currentWID());
	}
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid)
	{
	    if(ReflectiveEngine::isReflecting()) {
	        assert_true(wid == ReflectiveEngine::currentWID());
	        return senseIf<SEN_T,ResourceT>(p,rsc);
	    }
	    else
	        return Impl::sense<SEN_T,ResourceT>(p,rsc,wid);
	}
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
	{
	    if(ReflectiveEngine::isReflecting())
	        return senseIf<SEN_T,ResourceT>(p,rsc);
	    else
	        return Impl::sense<SEN_T,ResourceT>(p,rsc,ReflectiveEngine::currentWID());
	}

	template<SensingType SEN_T,typename ResourceT>
	static int enableSensor(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, bool enable)
	{
	    return Impl::enableSensor<SEN_T,ResourceT>(p,rsc,enable);
	}

    /*
     * Returns aggregated sensed value for the
     * current and previous instances of the given window
     * (or the current window if not specified)
     */
    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT *rsc, int wid)
    {
        if(ReflectiveEngine::isReflecting()) {
            assert_true(wid == ReflectiveEngine::currentWID());
            return senseAggIf<SEN_T,ResourceT>(rsc);
        }
        else
            return Impl::senseAgg<SEN_T,ResourceT>(rsc,wid);
    }
    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT *rsc)
    {
        if(ReflectiveEngine::isReflecting())
            return senseAggIf<SEN_T,ResourceT>(rsc);
        else
            return Impl::senseAgg<SEN_T,ResourceT>(rsc,ReflectiveEngine::currentWID());
    }
    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid)
    {
        if(ReflectiveEngine::isReflecting()) {
            assert_true(wid == ReflectiveEngine::currentWID());
            return senseAggIf<SEN_T,ResourceT>(p,rsc);
        }
        else
            return Impl::senseAgg<SEN_T,ResourceT>(p,rsc,wid);
    }
    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
    {
        if(ReflectiveEngine::isReflecting())
            return senseAggIf<SEN_T,ResourceT>(p,rsc);
        else
            return Impl::senseAgg<SEN_T,ResourceT>(p,rsc,ReflectiveEngine::currentWID());
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

  private:

    /*
     * These are the functions that need to be specialized
     */
    struct Reflective {

        template<SensingType SEN_T,typename ResourceT>
        static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT *rsc)
        {
            ReflectiveEngine::get().runFinerGrainedModels();
            if(hasPredictedVals(rsc)) return ReflectiveEngine::get().predict<SEN_T>(rsc);
            else                      return Impl::sense<SEN_T>(rsc,ReflectiveEngine::currentWID());
        }

        template<SensingType SEN_T,typename ResourceT>
        static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
        {
            ReflectiveEngine::get().runFinerGrainedModels();
            if(hasPredictedVals(rsc)) return ReflectiveEngine::get().predict<SEN_T>(p,rsc);
            else                      return Impl::sense<SEN_T>(p,rsc,ReflectiveEngine::currentWID());
        }

        template<SensingType SEN_T,typename ResourceT>
        static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT *rsc)
        {
            arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
        }

        template<SensingType SEN_T,typename ResourceT>
        static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
        {
            arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
        }

        static bool hasPredictedVals(const void *rsc) { return ReflectiveEngine::get().needsPred(rsc); }
    };
};

#endif /* ACTUATOR_H_ */
