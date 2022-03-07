#pragma once

template<int i> class __declspec(novtable) AlignedClass
{
public:
	AlignedClass()
	{
	}
	
	void* operator new (size_t size)
	{
		return _aligned_malloc(size, i);
	}

	void operator delete (void* p)
	{
		_aligned_free(p);
	}

	void* operator new [] (size_t size)
	{
		return _aligned_malloc(size, i);
	}

	void operator delete [] (void* p)
	{
		_aligned_free(p);
	}
};
