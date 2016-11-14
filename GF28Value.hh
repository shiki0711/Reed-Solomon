/*
 * GF28Value.hh
 *
 *  Created on: 2016/10/11
 *      Author: yanfeng
 */

#ifndef GF28VALUE_HH_
#define GF28VALUE_HH_

#include <iostream>

class GF28Value {
private:
	class _MULTIPLICATION_TABLE {
	public:
		_MULTIPLICATION_TABLE(){
			s_forwardTbl[0] = 1;	/* g^0 = e */
			for(int i=1; i<=254; ++i){
				s_forwardTbl[i] = (s_forwardTbl[i-1] << 1);	/* g(generator): x */
				if((s_forwardTbl[i] & 0x100) != 0){
					s_forwardTbl[i] ^= GF28_PRIME_POLYNOMIALS;
				}
			}
			for(int i=0; i<=254; ++i){
				s_backwardTbl[s_forwardTbl[i]] = i;
			}
			for(int i=1; i<=255; ++i){
				/*    i * s_reverseTbl[i] = e
				 * => g^k * g^(255-k) = e
				 * => s_reverseTbl[i] = g^(255-k)
				 *  */
				int k = s_backwardTbl[i];						/* calculate k */
				s_reverseTbl[i] = s_forwardTbl[(255-k)%255];	/* s_reverseTbl[i] */
			}
		}
		inline unsigned int getForward(unsigned int i) {
			if(i >= 255){
				return GF28_INVALID;
			}
			return s_forwardTbl[i];
		}
		inline unsigned int getBackward(unsigned int i) {
			if(i == 0 || i >= 256){
				return GF28_INVALID;
			}
			return s_backwardTbl[i];
		}
		inline unsigned int getReverse(unsigned int i) {
			if(i == 0 || i >= 256){
				return GF28_INVALID;
			}
			return s_reverseTbl[i];
		}
	public:
		void debug(void) const;
	private:
		static const unsigned int GF28_INVALID = 0x100;
		static const int GF28_PRIME_POLYNOMIALS = 0x11D;	/* x^8 + x^4 + x^3 +x +1 */
		static unsigned int s_forwardTbl[256];				/* s_forwardTbl[k] = g^k :(1<=s_forwardTbl[k]<=255, 0<=k<=254) */
		static unsigned int s_backwardTbl[256];				/* s_backwardTbl[g^k] = k (1<=g^k<=255, 0<=s_backwardTbl[g^k]<=254) */
		static unsigned int s_reverseTbl[256];				/* s_reverseTbl[k] = e/k (1<=k<255) */
	};

public:
	static const unsigned int GF28_INVALID = 0x100;
public:
	GF28Value(void):m_value(0){}
	GF28Value(unsigned int value):m_value((unsigned char)value){}
	~GF28Value(){}
	/* multiplication table singleton (needing c++11) */
	static inline _MULTIPLICATION_TABLE* getMultiplicationTblIns(void){
		static _MULTIPLICATION_TABLE s_multiple_tbl;
		return &s_multiple_tbl;
	}
	inline GF28Value& operator=(const GF28Value &a) {this->m_value = a.value(); return *this;}
	inline bool operator==(const GF28Value &a) const {return (this->m_value == a.value());}
	inline bool operator!=(const GF28Value &a) const {return !(this->m_value == a.value());}
	inline GF28Value operator+(const GF28Value &a) const {return GF28Value(this->value() ^ a.value());}
	inline GF28Value operator-(const GF28Value &a) const {return GF28Value(this->value() ^ a.value());}
	inline GF28Value operator*(const GF28Value &a) const {
		unsigned int r;
		unsigned int i = this->value();
		unsigned int j = a.value();
		_MULTIPLICATION_TABLE *ins = getMultiplicationTblIns();
		if(i == 0 || j == 0){
			r = 0;
		}else{
			r = ins->getForward((ins->getBackward(i) + ins->getBackward(j))%255);
		}
		return GF28Value(r);
	}
	inline GF28Value operator/(const GF28Value &a) const {
		unsigned int r;
		unsigned int i = this->value();
		unsigned int j = a.value();
		_MULTIPLICATION_TABLE *ins = getMultiplicationTblIns();
		if(j == 0){
			r = GF28_INVALID;
			std::cout << "Detect a div 0 error!!!" << std::endl;
		}else if(i == 0){
			r = 0;
		}else{
			r = ins->getForward(( ins->getBackward(i) + ins->getBackward(ins->getReverse(j)) ) % 255);
		}
		return GF28Value(r);
	}
	/* power(*this, a) */
	inline GF28Value operator^(const GF28Value &a) const {
		unsigned int i = this->value();
		unsigned int j = a.value();
		if(j == 0) {
			return GF28Value(1);
		}else if(i == 0) {
			return GF28Value(0);
		}else{
			GF28Value r(i);
			GF28Value t(i);
			for(int k=0; k<j-1; ++k) {
				r = r*t;
			}
			return r;
		}
	}
	inline unsigned int value(void) const {return m_value;}
	inline void output(std::ostream &s) const {
		std::streamsize w = s.width();
		s.width(2);
		char f = s.fill();
		s.fill('0');
		s.setf(std::ios::hex, std::ios::basefield);
		s << m_value << " ";
		s.unsetf(std::ios::hex);
		s.fill(f);
		s.width(w);
	}
	inline static unsigned int limit(void) {return 256;}
protected:
	unsigned int m_value;
};



#endif /* GF28VALUE_HH_ */
