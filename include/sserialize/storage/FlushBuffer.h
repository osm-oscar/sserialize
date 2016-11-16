#ifndef SSERIALIZE_FLUSH_BUFFER_H
#define SSERIALIZE_FLUSH_BUFFER_H
#pragma once

#in

namespace sserialize {

class FlushBuffer {
public:
	FlushBuffer();
	~FlushBuffer();
	void put();
	std::size_t size() const;
protected:
	void doFlush();
};


}

#endif