#ifndef __core_lists_h
#define __core_lists_h

//VITAMINS intrusive linked list implementation
//Everything is inline and macro-based


#define vitamins_list_head(name) __vitaminslist_head_##name
#define vitamins_list_tail(name) __vitaminslist_tail_##name

#define define_vitamins_list(type,name) \
    type* vitamins_list_head(name); \
    type* vitamins_list_tail(name)


#define __listnext(tag) _listnext_##tag
#define __listprev(tag) _listprev_##tag

#define define_list_addable(type,tag) \
    type* __listnext(tag); \
    type* __listprev(tag)

#define _defaultTag main

#define define_list_addable_default(type) \
    define_list_addable(type,_defaultTag)

#define _nextObject(obj,tag) \
    ((obj)->__listnext(tag))

#define _prevObject(obj,tag) \
    ((obj)->__listprev(tag))

#define _nextObject_default(obj) _nextObject(obj,_defaultTag)
#define _prevObject_default(obj) _prevObject(obj,_defaultTag)

#define clear_list(list_name) \
    do{ \
        vitamins_list_head(list_name) = nullptr; \
        vitamins_list_tail(list_name) = nullptr; \
    }while(0)

#define clear_internal_list(container,list_name) \
    do{ \
        (container)->vitamins_list_head(list_name) = nullptr; \
        (container)->vitamins_list_tail(list_name) = nullptr; \
    }while(0)

#define clear_object(obj,tag) \
    do{ \
        _nextObject(obj,tag) = nullptr; \
        _prevObject(obj,tag) = nullptr; \
    }while(0)

#define clear_object_default(obj) \
    do{ \
        _nextObject(obj,_defaultTag) = nullptr; \
        _prevObject(obj,_defaultTag) = nullptr; \
    }while(0)

#define add_to_list(list_name,obj,tag) \
    do{ \
        _prevObject(obj,tag) = nullptr; \
        _nextObject(obj,tag) = nullptr; \
        if(vitamins_list_head(list_name) == nullptr){ \
            vitamins_list_head(list_name) = obj; \
            vitamins_list_tail(list_name) = obj; \
            _nextObject(obj,tag) = nullptr; \
        } \
        else{ \
            _nextObject(vitamins_list_tail(list_name),tag) = obj; \
            _prevObject(obj,tag) = vitamins_list_tail(list_name); \
            vitamins_list_tail(list_name) = obj; \
        } \
        _nextObject(obj,tag) = nullptr; \
    }while(0)

#define add_to_internal_list(container,list_name,obj,tag) \
    do{ \
        _prevObject(obj,tag) = nullptr; \
        _nextObject(obj,tag) = nullptr; \
        if((container)->vitamins_list_head(list_name) == nullptr){ \
            (container)->vitamins_list_head(list_name) = obj; \
            (container)->vitamins_list_tail(list_name) = obj; \
            _nextObject(obj,tag) = nullptr; \
        } \
        else{ \
            _nextObject((container)->vitamins_list_tail(list_name),tag) = obj; \
            _prevObject(obj,tag) = (container)->vitamins_list_tail(list_name); \
            (container)->vitamins_list_tail(list_name) = obj; \
        } \
        _nextObject(obj,tag) = nullptr; \
    }while(0)

#define add_to_list_default(list_name,obj) \
    add_to_list(list_name,obj,_defaultTag)

#define add_to_priority_list(list_name,obj,tag,comparator,iterator) \
    do{ \
        _prevObject(obj,tag) = nullptr; \
        _nextObject(obj,tag) = nullptr; \
        if(vitamins_list_head(list_name) == nullptr){ \
            vitamins_list_head(list_name) = obj; \
            vitamins_list_tail(list_name) = obj; \
        } \
        else{ \
            iterator = vitamins_list_head(list_name);\
            \
            while(iterator != nullptr){\
                if(comparator(obj,iterator)) break;\
                iterator = _nextObject(iterator,tag);\
            }\
            \
            if(iterator != nullptr){\
                _nextObject(obj,tag) = iterator; \
                _prevObject(obj,tag) = _prevObject(iterator,tag); \
                if(iterator == vitamins_list_head(list_name)) \
                    vitamins_list_head(list_name) = obj;\
                else \
                    _nextObject(_prevObject(iterator,tag),tag) = obj; \
                _prevObject(iterator,tag) = obj; \
            }\
            else{\
                _nextObject(vitamins_list_tail(list_name),tag) = obj; \
                _prevObject(obj,tag) = vitamins_list_tail(list_name); \
                vitamins_list_tail(list_name) = obj; \
            }\
        } \
    }while(0)


#define add_to_priority_list_default(list_name,obj,comparator,iterator) \
    add_to_priority_list(list_name,obj,_defaultTag,comparator,iterator)

#define remove_from_list(list_name,task,tag) \
do{ \
    if(_nextObject(task,tag) != nullptr) \
        _prevObject(_nextObject(task,tag),tag) = _prevObject(task,tag); \
    if(_prevObject(task,tag) != nullptr) \
        _nextObject(_prevObject(task,tag),tag) = _nextObject(task,tag); \
    if(task == vitamins_list_head(list_name))\
        vitamins_list_head(list_name) = _nextObject(task,tag); \
    if(task == vitamins_list_tail(list_name))\
        vitamins_list_tail(list_name) = _prevObject(task,tag); \
}while(0)

#define remove_from_internal_list(container,list_name,task,tag) \
do{ \
    if(_nextObject(task,tag) != nullptr) \
        _prevObject(_nextObject(task,tag),tag) = _prevObject(task,tag); \
    if(_prevObject(task,tag) != nullptr) \
        _nextObject(_prevObject(task,tag),tag) = _nextObject(task,tag); \
    if(task == (container)->vitamins_list_head(list_name))\
        (container)->vitamins_list_head(list_name) = _nextObject(task,tag); \
    if(task == (container)->vitamins_list_tail(list_name))\
        (container)->vitamins_list_tail(list_name) = _prevObject(task,tag); \
}while(0)

#define remove_from_list_default(list_name,task) \
    remove_from_list(list_name,task,_defaultTag)


#define for_each_in_list_head(list_head,iterator,tag) \
    for(iterator=list_head; iterator != nullptr; iterator = _nextObject(iterator,tag))

#define for_each_in_list_head_default(list_head,iterator) \
    for_each_in_list_head(list_head,iterator,_defaultTag)

#define for_each_in_list(list_name,iterator,tag) \
    for_each_in_list_head(vitamins_list_head(list_name),iterator,tag)

#define for_each_in_internal_list(container,list_name,iterator,tag) \
    for_each_in_list_head((container)->vitamins_list_head(list_name),iterator,tag)

#define for_each_in_list_default(list_name,iterator) \
    for_each_in_list(list_name,iterator,_defaultTag)


#endif
