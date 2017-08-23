#ifndef CSTRINGBUFFER_H
#define CSTRINGBUFFER_H

#include <cstring>


class CStringBuffer {
public:
    CStringBuffer()
        : _str(0)
        , _len(0)
    {
    }

    ~CStringBuffer()
    {
        clear();
    }

    const char * str()
    {
        return _str;
    }

    int length()
    {
        return _len;
    }

    void clear()
    {
        if (_str != NULL) {
            std::free(_str);
            _str = NULL;
            _len = 0;
        }
    }

    bool append(const char *new_str, int new_len=-1)
    {
        if (new_str == NULL)
            return true;

        if (new_len < 0)
            new_len = std::strlen(new_str);
        int joint_len = _len + new_len;
        char *joint_str = (char *)std::realloc(_str, joint_len + 1);

        if (joint_str == NULL)
            return false;

        std::memcpy(joint_str + _len, new_str, new_len);
        joint_str[joint_len] = 0;
        _str = joint_str;
        _len = joint_len;
        return true;
    }

private:
    char *_str;
    int _len;
};

#endif /* CSTRINGBUFFER_H */
