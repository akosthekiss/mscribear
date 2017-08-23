#ifndef MORSE_H
#define MORSE_H

#include "mbed.h"


class Morse {
public:
    Morse(DigitalOut &out, float unit_length=0.25)
        : _out(out)
        , _unit_length(unit_length)
    {
    }

    void reset()
    {
        _out = 0;
    }

    int putditdah(const char *ditdah)
    {
        if (ditdah == NULL)
            return -1;

        int cnt = 0;

        for (; *ditdah != 0; ++ditdah) {
            if (cnt != 0)
                wait(_unit_length);

            switch (*ditdah) {
            case '.':
                _out = 1;
                wait(_unit_length);
                _out = 0;
                cnt++;
                break;
            case '-':
                _out = 1;
                wait(_unit_length * 3);
                _out = 0;
                cnt++;
                break;
            }
        }

        return cnt;
    }

    int putc(int c)
    {
        if (c < 0 || c >= 128)
            return -1;

        if (putditdah(_code[c]) < 1)
            return -1;

        return c;
    }

    int puts(const char *str, bool prosign=false)
    {
        if (str == NULL)
            return -1;

        int cnt = 0;
        bool needsletterspace = false;

        for (; *str != 0; ++str) {
            if (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') {
                wait(_unit_length * 7);
                needsletterspace = false;
            } else {
                if (needsletterspace)
                    wait(prosign ? _unit_length : _unit_length * 3);

                if (putc(*str) >= 0) {
                    cnt++;
                    needsletterspace = true;
                }
            }
        }

        return cnt;
    }

private:
    static const char * const _code[];

    DigitalOut &_out;
    float _unit_length;
};

#endif /* MORSE_H */
