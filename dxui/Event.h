#pragma once

#include <list>
#include <functional>

namespace DXUI
{
	class Control;

	template<class T = void*> class Event
	{
	protected:
		typedef std::tr1::function<bool (Control*, T)> F;
		typedef std::tr1::function<bool (Control*)> F0;
		typedef std::tr1::function<bool ()> G;

		struct Target {F f; void* e;};

		std::list<Target>* m_targets;
		G* m_guard;

		void AddTarget(const F& f, void* e = NULL, bool reset = false)
		{
			if(m_targets == NULL)
			{
				m_targets = new std::list<Target>();
			}

			if(reset)
			{
				m_targets->clear();
			}

			Target t = {f, e};

			m_targets->push_back(t);
		}

	public:
		Event() 
			: m_targets(NULL)
			, m_guard(NULL)
		{
		}

		~Event()
		{
			delete m_targets;
			delete m_guard;
		}

		template<class M, class O> void Add(M m, O o, bool reset = false)
		{
			AddTarget(bind(m, o, _1, _2), NULL, reset);
		}

		template<class M, class O> void Add0(M m, O o, bool reset = false)
		{
			AddTarget(bind(m, o, _1), NULL, reset);
		}

		void Add(const F& f, bool reset = false)
		{
			AddTarget(f, NULL, reset);
		}

		void Add0(const F0& f0, bool reset = false)
		{
			F f = [f0] (Control* c, T p) -> bool {return f0(c);};

			AddTarget(f, NULL, reset);
		}

		void Chain(Event<T>& e, bool reset = false)
		{
			F f = [&e] (Control* c, T p) -> bool {return e(c, p);};

			AddTarget(f, &e, reset);
		}

		void Chain0(Event<void*>& e, bool reset = false)
		{
			F f = [&e] (Control* c, T p) -> bool {return e(c);};

			AddTarget(f, &e, reset);
		}

		void Unchain(Event<T>& e)
		{
			if(m_targets != NULL)
			{
				for(auto i = m_targets->begin(); i != m_targets->end(); i++)
				{
					if(i->e == &e)
					{
						m_targets->erase(i);

						return;
					}
				}
			}
		}

		void Unchain0(Event<void*>& e)
		{
			if(m_targets != NULL)
			{
				for(auto i = m_targets->begin(); i != m_targets->end(); i++)
				{
					if(i->e == &e)
					{
						m_targets->erase(i);

						return;
					}
				}
			}
		}
/*
		void operator ^= (const F& f)
		{
			clear();

			AddTarget(f);
		}

		void operator += (const F& f)
		{
			AddTarget(f);
		}

		void operator *= (const F0& f0) // another += would be ambiguous (compiler cannot decide between F and F0)
		{
			F f = [f0] (Control* c, T p) -> bool {return f0(c);};

			AddTarget(f);
		}
*/
		void Guard(const G& g)
		{
			if(g)
			{
				if(m_guard == NULL)
				{
					m_guard = new G();
				}

				*m_guard = g;
			}
			else
			{
				delete m_guard;

				m_guard = NULL;
			}
		}

		bool operator () (Control* c, T p = 0)
		{
			if(m_guard != NULL && !(*m_guard)())
			{
				return true;
			}

			bool ret = false;

			if(m_targets != NULL)
			{
				for(auto i = m_targets->begin(); i != m_targets->end(); i++)
				{
					if(i->f(c, p))
					{
						ret = true;
					}
				}
			}

			return ret;
		}

		void clear()
		{
			if(m_targets != NULL)
			{
				delete m_targets;

				m_targets = NULL;
			}
		}

		bool empty() const
		{
			return m_targets == NULL || m_targets->empty();
		}
	};
}
