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

#include <limits>

#include "dummy.h"

#include <runtime/framework/sensing_interface.h>
#include <runtime/framework/actuation_interface.h>

static int actDummyVal1 = 1;
static int actDummyVal2 = 2;

static typename ActuationTypeInfo<ACT_DUMMY1>::Ranges actDummyRanges1 = {
        std::numeric_limits<ActuationTypeInfo<ACT_DUMMY1>::ValType>::min(),
        std::numeric_limits<ActuationTypeInfo<ACT_DUMMY1>::ValType>::max()
};
static typename ActuationTypeInfo<ACT_DUMMY2>::Ranges actDummyRanges2 = {
        std::numeric_limits<ActuationTypeInfo<ACT_DUMMY2>::ValType>::min(),
        std::numeric_limits<ActuationTypeInfo<ACT_DUMMY2>::ValType>::max()
};

static DummySensor *dummySensor = nullptr;

void DummySensor::create(SensingModule *sm)
{
    assert_true(dummySensor == nullptr);
    dummySensor = new DummySensor;
    sm->attachSensor(dummySensor);
}

void DummySensor::destroy()
{
    assert_true(dummySensor != nullptr);
    delete dummySensor;
}

typename SensingTypeInfo<SEN_DUMMY>::ValType DummySensor::readSample()
{
    return actDummyVal1 + actDummyVal2;
}

static inline typename SensingTypeInfo<SEN_DUMMY>::ValType dummySense(int wid) {
    assert_true(dummySensor != nullptr);
    return dummySensor->accData(wid) / dummySensor->samples(wid);
}
static inline typename SensingTypeInfo<SEN_DUMMY>::ValType dummySenseAgg(int wid) {
    assert_true(dummySensor != nullptr);
    return dummySensor->accDataAgg(wid) / dummySensor->samplesAgg(wid);
}


/*
 * Sensing interface functions implementation
 */

template<>
typename SensingTypeInfo<SEN_DUMMY>::ValType
SensingInterface::sense<SEN_DUMMY,NullResource>(const NullResource *rsc, int wid)
{
    return dummySense(wid);
}
template<>
typename SensingTypeInfo<SEN_DUMMY>::ValType
SensingInterface::senseAgg<SEN_DUMMY,NullResource>(const NullResource *rsc, int wid)
{
    return dummySenseAgg(wid);
}


/*
 * Actuation interface functions implementation
 */

template<>
void
ActuationInterface::actuate<ACT_DUMMY1,NullResource>(
        const NullResource *rsc,
        typename ActuationTypeInfo<ACT_DUMMY1>::ValType val)
{
    actDummyVal1 = val;
}
template<>
typename ActuationTypeInfo<ACT_DUMMY1>::ValType
ActuationInterface::actuationVal<ACT_DUMMY1,NullResource>(const NullResource *rsc)
{
    return actDummyVal1;
}
template<>
const typename ActuationTypeInfo<ACT_DUMMY1>::Ranges&
ActuationInterface::actuationRanges<ACT_DUMMY1,NullResource>(const NullResource *rsc)
{
    return actDummyRanges1;
}
template<>
void
ActuationInterface::actuationRanges<ACT_DUMMY1,NullResource>(const NullResource *rsc,
        const typename ActuationTypeInfo<ACT_DUMMY1>::Ranges &newRange)
{
    actDummyRanges1 = newRange;
}


template<>
void
ActuationInterface::actuate<ACT_DUMMY2,NullResource>(
        const NullResource *rsc,
        typename ActuationTypeInfo<ACT_DUMMY2>::ValType val)
{
    actDummyVal2 = val;
}
template<>
typename ActuationTypeInfo<ACT_DUMMY2>::ValType
ActuationInterface::actuationVal<ACT_DUMMY2,NullResource>(const NullResource *rsc)
{
    return actDummyVal2;
}
template<>
const typename ActuationTypeInfo<ACT_DUMMY2>::Ranges&
ActuationInterface::actuationRanges<ACT_DUMMY2,NullResource>(const NullResource *rsc)
{
    return actDummyRanges2;
}
template<>
void
ActuationInterface::actuationRanges<ACT_DUMMY2,NullResource>(const NullResource *rsc,
        const typename ActuationTypeInfo<ACT_DUMMY2>::Ranges &newRange)
{
    actDummyRanges2 = newRange;
}


