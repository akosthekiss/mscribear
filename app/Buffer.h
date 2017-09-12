// Copyright (c) 2017 Akos Kiss.
//
// Licensed under the BSD 3-Clause License
// <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
// This file may not be copied, modified, or distributed except
// according to those terms.

#ifndef BUFFER_H
#define BUFFER_H

#include <cstdlib>
#include <cstring>


class Buffer {
public:
    Buffer(unsigned int capacity=0, unsigned int increment=0)
        : _ptr(0)
        , _size(0)
        , _capacity(0)
        , _increment(increment)
    {
        if (capacity != 0) {
            _ptr = (char *)std::malloc(capacity);
            if (_ptr != NULL)
                _capacity = capacity;
        }
    }

    ~Buffer()
    {
        clear();
    }

    const char * ptr()
    {
        return _ptr;
    }

    unsigned int size()
    {
        return _size;
    }

    void clear()
    {
        if (_ptr != NULL) {
            std::free(_ptr);
            _ptr = NULL;
            _size = 0;
            _capacity = 0;
        }
    }

    bool append(const char *data, unsigned int data_size)
    {
        if (data == NULL)
            return true;

        unsigned int new_size = _size + data_size;
        if (new_size > _capacity) {
            unsigned int new_capacity = _capacity + _increment;
            if (new_capacity < new_size)
                new_capacity = new_size;

            char *new_ptr = (char *)std::realloc(_ptr, new_capacity);
            if (new_ptr == NULL)
                return false;

            _ptr = new_ptr;
            _capacity = new_capacity;
        }

        std::memcpy(_ptr + _size, data, data_size);
        _size = new_size;
        return true;
    }

    void chop(unsigned int size)
    {
        _size = size < _size ? _size - size : 0;
    }

    void take(Buffer &buffer)
    {
        clear();
        _ptr = buffer._ptr;
        _size = buffer._size;
        _capacity = buffer._capacity;

        buffer._ptr = NULL;
        buffer._size = 0;
        buffer._capacity = 0;
    }

private:
    char *_ptr;
    unsigned int _size;
    unsigned int _capacity;
    unsigned int _increment;
};

#endif /* BUFFER_H */
