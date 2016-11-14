/*
 * GF28Value.cc
 *
 *  Created on: 2016/10/11
 *      Author: yanfeng
 */

#include <cstdio>

#include "GF28Value.hh"

using namespace std;

unsigned int GF28Value::_MULTIPLICATION_TABLE::s_forwardTbl[256] = { };
unsigned int GF28Value::_MULTIPLICATION_TABLE::s_backwardTbl[256] = { };
unsigned int GF28Value::_MULTIPLICATION_TABLE::s_reverseTbl[256] = { };

void GF28Value::_MULTIPLICATION_TABLE::debug(void) const {
    std::streamsize w = cout.width();
    cout.width(2);
    char f = cout.fill();
    cout.fill('0');
    cout.setf(std::ios::hex, std::ios::basefield);

    cout << "forward table:" << endl;
    for (int i = 0; i < 256; ++i) {
        cout.width(2);
        cout << s_forwardTbl[i] << " ";
        if (i % 16 == 15) {
            cout << endl;
        }
    }
    cout << "backward table:" << endl;
    for (int i = 0; i < 256; ++i) {
        cout.width(2);
        cout << s_backwardTbl[i] << " ";
        if (i % 16 == 15) {
            cout << endl;
        }
    }
    cout << "reverse table:" << endl;
    for (int i = 0; i < 256; ++i) {
        cout.width(2);
        cout << s_backwardTbl[i] << " ";
        if (i % 16 == 15) {
            cout << endl;
        }
    }
    for (unsigned int i = 0; i < 255; ++i) {
        if (s_backwardTbl[s_forwardTbl[i]] != i) {
            cout << "forward/backward table error at index(" << i << ")."
                    << endl;
        }
        if (GF28Value(s_reverseTbl[i + 1]) * GF28Value(i + 1) != GF28Value(1)) {
            cout << "reverse table error at index(" << i + 1 << ")." << endl;
        }
    }

    cout.unsetf(std::ios::hex);
    cout.fill(f);
    cout.width(w);
}
