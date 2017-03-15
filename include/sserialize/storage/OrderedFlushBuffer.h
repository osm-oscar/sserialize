#ifndef SSERIALIZE_FLUSH_BUFFER_H
#define SSERIALIZE_FLUSH_BUFFER_H
#inclue <sserialize/containers/OOMArray.h>
#pragma once
#include <thread>
#include <mutex>

namespace sserialize {
namespace detail {
namespace FlushBuffer {
	template<typename TDestination>
	class FlushOperator {
	public:
		FlushOperator(TDestination & dest);
		FlushOperator(TDestination * dest);
		template<typename T_ITERATOR>
		bool operator()(const T_ITERATOR & begin, const T_ITERATOR & end);
	private:
		TDestination * m_dest;
	};
}}

class FlushBuffer {
public:
	typedef uint32_t value_type;
	typedef OOMArray storage_container;
	typedef detail::FlushBuffer::FlushOperator<storage_container> flush_operator;
public:
	FlushBuffer();
	~FlushBuffer();
	void put(std::size_t position);
	std::size_t size() const;
protected:
	void doFlush();
private:
	std::vector<value_type> m_buffer1;
	std::vector<value_type> m_buffer2;
	flush_operator m_storage;
	std::mutex m_lock;
};


}

#endif