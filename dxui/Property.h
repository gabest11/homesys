#pragma once

#include "Event.h"
#include "../3rdparty/baseclasses/baseclasses.h"

namespace DXUI
{
	template<class T, bool Lock> class PropertyBase
	{
	protected:
		typedef std::tr1::function<void (T&)> F;

		T* m_value;
		F* m_get;
		F* m_set;
		PropertyBase* m_chain;
		CCritSec* m_lock;

		void Alloc()
		{
			if(m_value == NULL)
			{
				m_value = new T(); // only simple types are initialized to 0
			}
		}

		const T& GetValue()
		{
			if(m_chain != NULL)
			{
				return m_chain->GetValue();
			}

			if(Lock) m_lock->Lock();

			Alloc();

			if(m_get != NULL)
			{
				(*m_get)(*m_value);
			}

			if(Lock) m_lock->Unlock();

			return *m_value;
		}

		void SetValue(const T& value)
		{
			if(m_chain != NULL)
			{
				m_chain->SetValue(value);

				return;
			}

			if(Lock) m_lock->Lock();

			ChangingEvent(NULL, value);

			Alloc();

			*m_value = value;
			
			if(m_set != NULL)
			{
				(*m_set)(*m_value);
			}

			ChangedEvent(NULL, *m_value);

			if(Lock) m_lock->Unlock();
		}

	public:
		PropertyBase()
			: m_value(NULL)
			, m_get(NULL)
			, m_set(NULL)
			, m_chain(NULL)
		{
			if(Lock) m_lock = new CCritSec();
			else m_lock = NULL;
		}

		virtual ~PropertyBase()
		{
			delete m_get;
			delete m_set;
			delete m_value;
			delete m_lock;

			m_get = NULL;
			m_set = NULL;
			m_value = NULL;
			m_lock = NULL;
		}

		void Init(const T& value)
		{
			*m_value = value;
		}

		template<class M, class O> void Get(M m, O o)
		{
			if(m_get == NULL) 
			{
				m_get = new F();
			}

			*m_get = bind(m, o, std::tr1::placeholders::_1);
		}

		template<class M, class O> void Set(M m, O o)
		{
			if(m_set == NULL) 
			{
				m_set = new F();
			}

			*m_set = bind(m, o, std::tr1::placeholders::_1);
		}

		void Get(F f)
		{
			if(m_get == NULL) 
			{
				m_get = new F();
			}

			*m_get = f;
		}

		void Set(F f)
		{
			if(m_set == NULL) 
			{
				m_set = new F();
			}

			*m_set = f;
		}

		void Chain(PropertyBase* p)
		{
			if(m_chain)
			{
				m_chain->ChangingEvent.Unchain(ChangingEvent);
				m_chain->ChangedEvent.Unchain(ChangedEvent);
			}

			m_chain = p;

			if(m_chain)
			{
				m_chain->ChangingEvent.Chain(ChangingEvent);
				m_chain->ChangedEvent.Chain(ChangedEvent);
			}
		}

		Event<const T&> ChangingEvent;
		Event<const T&> ChangedEvent;
	};

	template<class T, bool Lock = false> class Property : public PropertyBase<T, Lock>
	{
	public:

		const T* operator -> ()
		{
			return &GetValue();
		}

		operator const T& ()
		{
			return GetValue();
		}

		Property& operator = (const T& value)
		{
			SetValue(value);

			return *this;
		}

		Property& operator = (Property& p)
		{
			SetValue(p.GetValue());

			return *this;
		}
	};

	template<class T, bool Lock> class Property<T*, Lock> : public PropertyBase<T*, Lock>
	{
	public:

		T* operator -> ()
		{
			return GetValue();
		}

		operator T* ()
		{
			return GetValue();
		}

		Property& operator = (T* value)
		{
			SetValue(value);

			return *this;
		}

		Property& operator = (Property& p)
		{
			SetValue(p.GetValue());

			return *this;
		}
	};
}
