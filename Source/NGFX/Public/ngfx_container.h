#pragma once
#ifndef __ngfx_container_h__
#define __ngfx_container_h__

#include <map>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <ngfx_allocator.h>

namespace ngfx
{
	template <typename key_type, typename value_type>
	class HashMap
	{
		typedef std::unordered_map<key_type, value_type> this_map;
	public:
		HashMap() {}
		~HashMap() {}

		bool contains(key_type const& key) const
		{
			return m_map.find(key) != m_map.end();
		}

	private:
		this_map m_map;
	};

	template <typename value_type, class allocator_type = std::allocator<value_type> >
	class Vec
	{
		typedef std::vector<value_type, allocator_type> this_array;
        
	public:
        typedef typename std::vector<value_type, allocator_type>::iterator iterator;
		Vec() {}
		~Vec() {}

        iterator begin() {
            return m_array.begin();
        }

        iterator end() {
            return m_array.end();
        }

        void push(value_type const& val) {
            m_array.push_back(val);
        }

        void push(Vec<value_type> const& other) 
        {
            other.iter([this](value_type const& v) 
            {
                this->push(v);
            });
        }

        void push(value_type && val) {
            m_array.push_back(std::forward<value_type>(val));
        }

        void pop() {
            m_array.pop_back();
        }

        template <typename iter_fn>
        void iter(iter_fn fn_iter) const
        {
            for (value_type const& v : m_array)
            {
                fn_iter(v);
            }
        }

        template <typename iter_fn>
        void iter_mut(iter_fn fn_iter)
        {
            for (value_type& v : m_array)
            {
                fn_iter(v);
            }
        }

        inline bool empty() const { return m_array.empty(); }

        void clear()
        {
            m_array.clear();
        }

        size_t num() const { return m_array.size(); }

        const value_type& at(size_t i) const { return m_array[i]; }
        value_type& at(size_t i) { return m_array[i]; }

        size_t add_uninitialized()
        {
            size_t id = m_array.size();
            m_array.push_back(value_type());
            return id;
        }

	private:
		this_array m_array;
	};

    template <typename value_type>
    class VecUPtr : public Vec<UniqPtr<value_type> >
    {
    public:
        constexpr VecUPtr() = default;
    };
}

template <typename value_type, class allocator_type>
void* operator new(size_t size, ngfx::Vec<value_type, allocator_type>& inArray) {
    size_t id = inArray.add_uninitialized();
    return &inArray.at(id);
}
#endif
