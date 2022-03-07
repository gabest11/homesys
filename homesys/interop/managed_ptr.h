#pragma once

#include <gcroot.h>

template<class T> public class managed_ptr
{
protected:
	gcroot<T^> t;

public:
	managed_ptr() {t = gcnew T();}
	managed_ptr(T^ _t) {t = _t;}
	T^ operator->() const {return t;}
	operator T^() const {return t;}
};

