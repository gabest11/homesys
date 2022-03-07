#pragma once

namespace SSF
{
	// simple array class for simple types without constructors and it doesn't free its reserves on SetCount(0)

	template<class T> class Array
	{
		T* m_data;
		int m_size;
		int m_maxsize;
		int m_growby;

	public:
		Array() 
		{
			m_data = NULL; 
			m_size = 0;
			m_maxsize = 0; 
			m_growby = 4096;
		}

		virtual ~Array()
		{
			if(m_data) 
			{
				_aligned_free(m_data);
			}
		}

		void SetCount(int size, int growby = 0)
		{
			if(growby > 0)
			{
				m_growby = growby;
			}

			if(size > m_maxsize)
			{
				m_maxsize = size + max(m_growby, m_size);
				int bytes = m_maxsize * sizeof(T);
				m_data = (T*)_aligned_realloc(m_data, bytes, 16);
			}

			m_size = size;
		}

		int GetCount() const 
		{
			return m_size;
		}

		void RemoveAll() 
		{
			m_size = 0;
		}

		bool IsEmpty() const 
		{
			return m_size == 0;
		}

		T* GetData() 
		{
			return m_data;
		}

		void Add(const T& t)
		{
			int i = m_size;
			SetCount(m_size + 1);
			m_data[i] = t;
		}

		void Append(const Array& a, int growby = 0)
		{
			Append(a.m_data, a.m_size, growby);
		}

		void Append(const T* ptr, int size, int growby = 0)
		{
			if(!size) return;
			int oldsize = m_size;
			SetCount(oldsize + size);
			memcpy(m_data + oldsize, ptr, size * sizeof(T));
		}

		const T& operator [] (int i) const
		{
			return m_data[i];
		}

		T& operator [] (int i) 
		{
			return m_data[i];
		}

		void Copy(const Array& v)
		{
			SetCount(v.GetCount());

			memcpy(m_data, v.m_data, m_size * sizeof(T));
		}

		void Move(Array& v)
		{
			Swap(v);

			v.SetCount(0);
		}

		void Swap(Array& v)
		{
			T* data = m_data; m_data = v.m_data; v.m_data = data;
			int size = m_size; m_size = v.m_size; v.m_size = size;
			int maxsize = m_maxsize; m_maxsize = v.m_maxsize; v.m_maxsize = maxsize;
			int growby = m_growby; m_growby = v.m_growby; v.m_growby = growby;
		}
	};
}