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

#ifndef __arm_rt_common_gen_store_h
#define __arm_rt_common_gen_store_h

#include <unordered_map>
#include <tuple>

#include <base/base.h>

template <class T1, class T2>
struct SameType
{
    static constexpr bool value = false;
};
template<class T>
struct SameType<T, T>
{
    static constexpr bool value = true;
};

template<typename Key, typename ...Types>
struct GenericStoreMap {

  //private:
    template<typename Val>
    struct _map {
        std::unordered_map<Key,Val> map;
        using value_type = typename std::unordered_map<Key,Val>::value_type;
    };

    typedef std::tuple<_map<Types>...> mapt;

    mapt _maps;

    template<int N, typename T>
    struct MapOfType: SameType<T,
       typename std::tuple_element<N, mapt>::type::value_type::second_type>
    { };

    template <int N, class T, class Tuple,
             bool Match = false> // this =false is only for clarity
    struct MatchingField
    {
        static std::unordered_map<Key,T>& get(Tuple& tp)
        {
            // The "non-matching" version
            return MatchingField<N+1, T, Tuple,
                    MapOfType<N+1, T>::value>::get(tp);
        }
    };

    template <int N, class T, class Tuple>
    struct MatchingField<N, T, Tuple, true>
    {
       static std::unordered_map<Key,T>& get(Tuple& tp)
       {
           return std::get<N>(tp).map;
       }
    };


  public:

    template <typename T>
    std::unordered_map<Key,T>& access()
    {
        return MatchingField<0, T, mapt,
               MapOfType<0, T>::value>::get(_maps);
    }

  private:

    // Iterate through maps, clearing them
    template <int dummy>//necessary to stop template resolution (cannot define empty template list for functions)
    void _clearHelper() { }
    template<int dummy, typename Head, typename... Tail>
    void _clearHelper() { access<Head>().clear(); _clearHelper<dummy,Tail...>(); }


  public:
    void clearMaps()
    {
        _clearHelper<0,Types...>();
    }
};


// Helper class for encapsulating multiple possible types.
// Same idea of tuples, but works like an union.
// However, to avoid data corruption, only one of the possible types will
// actually be stored. This will be given by the very first type used
// with the get method. If a subsequent call to get used a different type
// an error is issued.

template<typename ...Types>
class VariantStore {

public:

    template<typename Type>
    struct IsNull {
        bool val;
        typedef Type type;
    };
    typedef std::tuple<IsNull<Types>...> NullTuple;

    void* _rawPtr;

    NullTuple _isNull;


    template<int N, typename T>
    struct IsOfType: SameType<T,
       typename std::tuple_element<N, NullTuple>::type::type>
    { };

    template <int N, class T, class Tuple,
             bool Match = false> // this =false is only for clarity
    struct MatchingField
    {
        static bool& get(Tuple& tp)
        {
            // The "non-matching" version
            return MatchingField<N+1, T, Tuple,
                    IsOfType<N+1, T>::value>::get(tp);
        }
    };
    template <int N, class T, class Tuple>
    struct MatchingField<N, T, Tuple, true>
    {
       static bool& get(Tuple& tp)
       {
           return std::get<N>(tp).val;
       }
    };
    template <typename T>
    bool& _accessIsNull()
    {
        return MatchingField<0, T, NullTuple,
               IsOfType<0, T>::value>::get(_isNull);
    }

    template <int dummy>//necessary to stop template resolution (cannot define empty template list for functions)
    void _clearIsNull() { }
    template<int dummy, typename Head, typename... Tail>
    void _clearIsNull() { _accessIsNull<Head>() = true; _clearIsNull<dummy,Tail...>(); }

    template <int dummy>//necessary to stop template resolution (cannot define empty template list for functions)
    void _deletePtr() { }
    template<int dummy, typename Head, typename... Tail>
    void _deletePtr()
    {
        if(!_accessIsNull<Head>()){
            assert_true(_rawPtr != nullptr);
            delete ((Head*)_rawPtr);
            _rawPtr = nullptr;
            _accessIsNull<Head>() = true;
        }
        _deletePtr<dummy,Tail...>();
    }


  public:

    VariantStore()
        :_rawPtr(nullptr)
    {
        _clearIsNull<0,Types...>();
    }

    ~VariantStore()
    {
        _deletePtr<0,Types...>();
    }


    // Initializes the value stored using type T
    // If args are not provided, the the value is default-constructed
    template<typename T, typename... Args>
    void init(Args... args)
    {
        if(_rawPtr != nullptr)
            arm_throw(VariantStoreException,"Value stored in the variant can be of only one type");
        _rawPtr = new T(args...);
        _accessIsNull<T>() = false;
    }

    // Obtain the value stored of type T
    // If value is not initialized, it's default constructed

    template<typename T>
    T& get()
    {
        if(_accessIsNull<T>()) init<T>();
        return *((T*)_rawPtr);
    }

    bool notDefined() { return _rawPtr == nullptr; }

};

#endif /* ACTUATOR_H_ */
