#pragma once

#include <memory>

template<typename T>
class Pimpl 
{
private:
	std::unique_ptr<T> m;
public:
	Pimpl();
	template<typename Arg1>
	Pimpl( Arg1&& arg1 );

	template<typename Arg1, typename Arg2>
	Pimpl( Arg1&& arg1, Arg2&& arg2 );

	template<typename Arg1, typename Arg2, typename Arg3>
	Pimpl( Arg1&& arg1, Arg2&& arg2, Arg3&& arg3 );

	~Pimpl();

	T* operator->();
	T* operator->() const;
	T& operator*();
	T& operator*() const;
};