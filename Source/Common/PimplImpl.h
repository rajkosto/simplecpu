#include "Pimpl.h"

#include <utility>

template<typename T>
Pimpl<T>::Pimpl() : m(new T()) { }

template<typename T>
template<typename Arg1>
Pimpl<T>::Pimpl( Arg1&& arg1 )
	: m( new T( std::forward<Arg1>(arg1) ) ) { }

template<typename T>
template<typename Arg1, typename Arg2>
Pimpl<T>::Pimpl( Arg1&& arg1, Arg2&& arg2 )
	: m( new T( std::forward<Arg1>(arg1), std::forward<Arg2>(arg2) ) ) { }

template<typename T>
template<typename Arg1, typename Arg2, typename Arg3>
Pimpl<T>::Pimpl( Arg1&& arg1, Arg2&& arg2, Arg3&& arg3 )
	: m( new T( std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3) ) ) { }

template<typename T>
Pimpl<T>::~Pimpl() { }

template<typename T>
T* Pimpl<T>::operator->() { return m.get(); }

template<typename T>
T* Pimpl<T>::operator->() const { return m.get(); }

template<typename T>
T& Pimpl<T>::operator*() { return *m.get(); }

template<typename T>
T& Pimpl<T>::operator*() const { return *m.get(); }