#ifndef __sasolver_globals_alloc_h
#define __sasolver_globals_alloc_h

#include "solver_defines.h"

#ifdef __KERNEL__
void* operator new (long unsigned int count, void* ptr );
#endif

namespace SASolverImpl {

namespace Global{

namespace Global_Internal{
	extern vit_allocator_f allocator;
	extern vit_deallocator_f deallocator;
}

template<typename T>
T* allocate(){
	return new ((*Global_Internal::allocator)(sizeof(T))) T;
}
template<typename T, typename P1>
T* allocate(P1 p1){
	return new ((*Global_Internal::allocator)(sizeof(T))) T(p1);
}
template<typename T, typename P1>
T* allocate(P1 p1, P1 p2){
	return new ((*Global_Internal::allocator)(sizeof(T))) T(p1,p2);
}
template<typename T, typename P1,typename P2,typename P3>
T* allocate(P1 p1, P2 p2, P3 p3){
	return new ((*Global_Internal::allocator)(sizeof(T))) T(p1,p2,p3);
}
template<typename T, typename P1,typename P2,typename P3,typename P4>
T* allocate(P1 p1, P2 p2, P3 p3, P3 p4){
	return new ((*Global_Internal::allocator)(sizeof(T))) T(p1,p2,p3,p4);
}

template<typename T>
T* allocate_array(long unsigned int size){
	T* aux = (T*)(*Global_Internal::allocator)(sizeof(T)*size);
	for(long unsigned int i = 0; i < size; ++i)
		aux[i] = 0;
	return aux;
}

template<typename T>
T* allocate_array_obj(long unsigned int size){
	T* aux = (T*)(*Global_Internal::allocator)(sizeof(T)*size);
	for(long unsigned int i = 0; i < size; ++i)
		aux[i] = T();
	return aux;
}

void deallocate(void* ptr);

template<typename T>
void deallocate_obj(T* ptr){
	ptr->~T();
	deallocate(ptr);
}

template<typename T>
void deallocate_array_obj(T* ptr, long unsigned int size){
	for(long unsigned int i; i < size; ++i)
		ptr->~T();
	deallocate(ptr);
}

template<typename T>
T** allocate_matrix(long unsigned int sizeI, long unsigned int sizeJ){
	T** aux = allocate_array<T*>(sizeI);
	if(aux != 0){
		long unsigned int i = 0;
		for(; i < sizeI; ++i){
			aux[i] = allocate_array<T>(sizeJ);
			if(aux[i] == 0) break;//error
		}
		if(i < sizeI){
			//error, deallocate everything
			for (long unsigned int j = i; i >= 0; --j) {
				deallocate(aux[j]);
			}
			deallocate(aux);
			aux = 0;
		}
	}
	return aux;
}

template<typename T>
T** allocate_matrix_obj(long unsigned int sizeI, long unsigned int sizeJ){
	T** aux = allocate_array<T*>(sizeI);
	if(aux != 0){
		long unsigned int i = 0;
		for(; i < sizeI; ++i){
			aux[i] = allocate_array_obj<T>(sizeJ);
			if(aux[i] == 0) break;//error
		}
		if(i < sizeI){
			//error, deallocate everything
			for (long unsigned int j = i; i >= 0; --j) {
				deallocate_array_obj(aux[j],sizeJ);
			}
			deallocate(aux);
			aux = 0;
		}
	}
	return aux;
}

template<typename T>
void deallocate_matrix(T** matrix, long unsigned int sizeI, long unsigned int sizeJ){
	for(long unsigned int i = 0; i < sizeI; ++i){
		deallocate(matrix[i]);
	}
	deallocate(matrix);
}

template<typename T>
void deallocate_matrix_obj(T** matrix, long unsigned int sizeI, long unsigned int sizeJ){
	for(long unsigned int i = 0; i < sizeI; ++i){
		deallocate_array_obj(matrix[i],sizeJ);
	}
	deallocate(matrix);
}

};

};

#endif
