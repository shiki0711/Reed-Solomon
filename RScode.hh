/*
 * RScode.hh
 *
 *  Created on: 2016/10/11
 *      Author: yanfeng
 */

#ifndef RSCODE_HH_
#define RSCODE_HH_

#include <iostream>
#include <type_traits>
#include <limits>

using namespace std;


/* T is a floating point type or class type which must has interfaces:
 * T(int)
 * T& operator=(const T &);
 * bool operator==(const T &a) const
 * bool operator!=(const T &a) const
 * T operator+(const T &) const
 * T operator-(const T &) const
 * T operator*(const T &) const
 * T operator/(const T &) const
 * void output(std::ostream &)
 * unsigned char value(void)
 * static unsigned int limit(void);
 *  */

/* TODO: floating point matrix based encoding/decoding
 * */
template<typename T>
class RScode {
public:
	typedef enum {
		e_rscode_sts_ok = 0,
		e_rscode_sts_init,
		e_rscode_sts_construct_err,
		e_rscode_sts_encoding_err,
		e_rscode_sts_decoding_err,
	} E_RSCODE_STS;
private:
	struct value_type_traits : public is_floating_point<T>{ };
	typedef typename is_floating_point<T>::type T_IS_FLOATING;

public:
	RScode(unsigned int encodeLineSize, unsigned int dataLineSize){
		if(encodeLineSize<1 || encodeLineSize>limit(T(), T_IS_FLOATING())){
			m_error = e_rscode_sts_construct_err;
		}else{
			m_encodeLineSize = encodeLineSize;
			m_limit = limit(T(), T_IS_FLOATING());
			m_curLine = 0;
			m_pCauchyMatrix = new T[encodeLineSize*m_limit];
			m_pL = new T[m_encodeLineSize*m_encodeLineSize];
			m_pU = new T[m_encodeLineSize*m_encodeLineSize];
			m_pLInverseMatrix = new T[m_encodeLineSize*m_encodeLineSize];
			m_pUInverseMatrix = new T[m_encodeLineSize*m_encodeLineSize];
			m_pEncodeMatrix = new T[m_encodeLineSize*m_encodeLineSize];
			m_pEncodeInverseMatrix = new T[m_encodeLineSize*m_encodeLineSize];
			const T tmp0(0);
			const T tmp1(1);
			/* create cauchy matrix */
			for(int i=0; i<encodeLineSize; ++i){
				for(int j=0; j<encodeLineSize; ++j){
					if(i==j){
						*position(m_pCauchyMatrix, i, j) = tmp1;
					}else{
						*position(m_pCauchyMatrix, i, j) = tmp0;
					}
				}
			}
			for(int i=encodeLineSize; i<m_limit; ++i){
				for(int j=0; j<encodeLineSize; ++j){
					T x(i);		/* x = {(encodeLineSize),...,(m_limit-1)} */
					T y(j);		/* y = {0,...,(encodeLineSize-1)} */
					*position(m_pCauchyMatrix, i, j) = T(1)/(x+y);	/* 1/(x+y) */
				}
			}
			m_error = e_rscode_sts_ok;
		}
	}
	~RScode(){
		if(m_error != e_rscode_sts_init && m_error != e_rscode_sts_construct_err){
			if(m_pCauchyMatrix) delete [] m_pCauchyMatrix;
			if(m_pL) delete [] m_pL;
			if(m_pU) delete [] m_pU;
			if(m_pLInverseMatrix) delete [] m_pLInverseMatrix;
			if(m_pUInverseMatrix) delete [] m_pUInverseMatrix;
			if(m_pEncodeMatrix) delete [] m_pEncodeMatrix;
			if(m_pEncodeInverseMatrix) delete [] m_pEncodeInverseMatrix;
		}
	}
	void clear(){
		m_curLine = 0;
		if(m_error == e_rscode_sts_encoding_err || m_error == e_rscode_sts_decoding_err){
			m_error = e_rscode_sts_ok;
		}
	}
	int encodeLine(const unsigned char *data, unsigned int dataLineSize, unsigned char *encode){
		int ret;
		if(m_error == e_rscode_sts_init || m_error == e_rscode_sts_construct_err){
			cout << "Encoding line size error. Check the encodeLineSize parameter of constructor." << endl;
			return -1;
		}
		if(dataLineSize == 0){
			m_error = e_rscode_sts_encoding_err;
			cout << "dataLineSize line size error. It should greater then 0." << endl;
			return -1;
		}
		if(m_curLine >= m_limit){
			m_error = e_rscode_sts_encoding_err;
			cout << "Limit(" << m_curLine << ") error. No more date can be encoded." << endl;
			return -1;
		}
		T *tData = new T[dataLineSize*m_encodeLineSize];
		T *tEncode = new T[dataLineSize];
		for(unsigned int i=0; i<dataLineSize*m_encodeLineSize; ++i){
			tData[i] = T((unsigned int)data[i]);
		}
		ret = matrixMultiplication(position(m_pCauchyMatrix, m_curLine, 0), 1, m_encodeLineSize,
				tData, m_encodeLineSize, dataLineSize, tEncode);
		if(ret == 0){
			for(unsigned int i=0; i<dataLineSize; ++i){
				encode[i] = value(tEncode[i], T_IS_FLOATING());
			}
			m_curLine ++;
		}
		delete []tData;
		delete []tEncode;
		return ret;
	}
	/* Must swap rows of encoded data putting identity rows in their proper places
	 * and filling missed rows with encoding matrix before calling this method
	 * if not, the encoded matrix will have no valid LU factorization.
	 * For example:
	 * A encoding matrix and data like this
	 * ----------------------------------------------------
	 * encoding matrix		original data	encoded data
	 * 1 0 0 0				D1				D1
	 * 0 1 0 0				D2				D2	<--- missed
	 * 0 0 1 0			X	D3			 =	D3
	 * 0 0 0 1				D4				D4
	 * I J K L								C1
	 * O P Q R								C2
	 * ----------------------------------------------------
	 * To decode the original data, you must fill missed data(D2) using C1 or C2 like this
	 * ----------------------------------------------------
	 * encoded data
	 * D1
	 * C1(or C2)
	 * D3
	 * D4
	 * ----------------------------------------------------
	 * And indexArray parameter should be [0, 4(or 5), 2, 3]
	 *  */
	int decode(const unsigned char *encode, const unsigned int *indexArray, unsigned char *data, unsigned int dataLineSize){
		if(m_error == e_rscode_sts_init || m_error == e_rscode_sts_construct_err){
			cout << "Encoding line size error. Check the encodeLineSize parameter of constructor." << endl;
			return -1;
		}
		if(dataLineSize == 0){
			m_error = e_rscode_sts_decoding_err;
			cout << "dataLineSize line size error. It should greater then 0." << endl;
			return -1;
		}

		/* Calculate the inverse matrix of encoding matrix using LU factorization */
		LUFactorization(indexArray);
		inverseEncodeMatrix();
		/* Calculate missing lines using inverse matrix */
		T *tEncode = new T[m_encodeLineSize*dataLineSize];
		T *tData = new T[m_encodeLineSize*dataLineSize];
		for(unsigned int i=0; i<m_encodeLineSize; ++i){
			for(unsigned int j=0; j<dataLineSize; ++j){
				tEncode[i*dataLineSize+j] = T((unsigned int)encode[i*dataLineSize+j]);
			}
		}
		matrixMultiplication(m_pEncodeInverseMatrix, m_encodeLineSize, m_encodeLineSize, tEncode, m_encodeLineSize, dataLineSize, tData);
		for(unsigned int i=0; i<m_encodeLineSize*dataLineSize; ++i){
			data[i] = value(tData[i], T_IS_FLOATING());
		}
		return 0;
	}

	inline E_RSCODE_STS error(void){return m_error;};

	/* Wrappers for T */
private:
	/* value of T type object */
	inline unsigned char value(const T& t, true_type){
		return (unsigned char)t;
	}
	inline unsigned char value(const T& t, false_type){
		return t.value();
	}
	/* output method of T type object */
	inline void output(const T& t, std::ostream &s, true_type){
		s << t << " ";
	}
	inline void output(const T& t, std::ostream &s, false_type){
		t.output(s);
	}
	/* encoding matrix rows limit of T type */
	inline unsigned int limit(const T&, true_type){
		return 256;
	}
	inline unsigned int limit(const T&, false_type){
		return T::limit();
	}

	/* Internal methods */
private:
	/* Value of p[row][column] */
	inline T* position(T* p, unsigned int row, unsigned column){
		return p+row*m_encodeLineSize+column;
	}
	/* LUFactorization using iteration */
	void LUFactorization(const unsigned int *pByteIndexArray){
		T t;
		unsigned int curIndex;
		for(unsigned int i=0; i<m_encodeLineSize; ++i){
			curIndex = *(pByteIndexArray+i);
			for(unsigned int j=0; j<m_encodeLineSize; ++j){
				*position(m_pEncodeMatrix, i, j) = *position(m_pCauchyMatrix, curIndex, j);	/* original matrix */
			}
		}

		for(unsigned int i=0; i<m_encodeLineSize; ++i){
			for(unsigned int j=0; j<m_encodeLineSize; ++j){
				if(i==0){
					*position(m_pU, 0, j) = *position(m_pEncodeMatrix, 0, j);
					*position(m_pL, j, 0) = (*position(m_pEncodeMatrix, j, 0)) / (*position(m_pEncodeMatrix, 0, 0));
				}else{
					if(i<=j){
						/* calculate u[i][j] */
						t = T(0);
						for(unsigned int k=0; k<i; ++k){
							t =  t + (*position(m_pL, i, k)) * (*position(m_pU, k, j));
						}
						*position(m_pU, i, j) = *position(m_pEncodeMatrix, i, j) - t;

						/* calculate l[j][i] */
						if(i==j){
							*position(m_pL, j, i) = T(1);
						}else{
							t = T(0);
							for(unsigned int k=0; k<i; ++k){
								t =  t + (*position(m_pL, j, k)) * (*position(m_pU, k, i));
							}
							*position(m_pL, j, i) = (*position(m_pEncodeMatrix, j, i) - t) / (*position(m_pU, i, i));
						}
					}else{
						*position(m_pU, i, j) = T(0);
						*position(m_pL, j, i) = T(0);
					}
				}
			}
		}
	}
	/* Calculate the inverse matrix of L using iteration */
	void inverseLMatrix(void){
		const T t0(0);
		const T t1(1);
		for(unsigned int i=0; i<m_encodeLineSize; ++i){
			for(unsigned int j=0; j<m_encodeLineSize; ++j){
				if(i<j){
					*position(m_pLInverseMatrix, i, j) = t0;
				}else if(i==j){
					*position(m_pLInverseMatrix, i, j) = t1;
				}else{
					T r(0);
					for(unsigned int k=j; k<i; ++k){
						r = r + (*position(m_pL, i, k)) * (*position(m_pLInverseMatrix, k, j));
					}
					*position(m_pLInverseMatrix, i, j) = t0 - r;
				}
			}
		}
	}
	/* Calculate the inverse matrix of U using iteration */
	void inverseUMatrix(void){
		const T t0(0);
		const T t1(1);
		for(int i=m_encodeLineSize-1; i>=0; --i){
			for(unsigned int j=0; j<m_encodeLineSize; ++j){
				if(i>j){
					*position(m_pUInverseMatrix, i, j) = t0;
				}else if(i==j){
					*position(m_pUInverseMatrix, i, j) = t1/(*position(m_pU, i, i));
				}else{
					T r(0);
					for(unsigned int k=j; k>i; --k){
						r = r + (*position(m_pU, i, k)) * (*position(m_pUInverseMatrix, k, j));
					}
					*position(m_pUInverseMatrix, i, j) = (t0 - r)/(*position(m_pU, i, i));
				}
			}
		}
	}
	int matrixMultiplication(T *x, unsigned int xSizeI, unsigned int xSizeJ,
			T *y, unsigned int ySizeI, unsigned int ySizeJ, T *r){
		if(xSizeJ != ySizeI){
			return -1;
		}
		const T t(0);
		for(unsigned i=0; i<xSizeI; ++i){
			for(unsigned j=0; j<ySizeJ; ++j){
				*position(r, i, j) = t;
				for(unsigned k=0; k<xSizeJ; ++k){
					*position(r, i, j) = (*position(r, i, j)) + (*position(x, i, k)) * (*position(y, k, j));
				}
			}
		}
		return 0;
	}
	void inverseEncodeMatrix(void){
		/* Encoding matrix(A) = LxU
		 * Decoding matrix(Inverse(A)) = Inverse(U)xInverse(L)
		 *  */
		inverseLMatrix();
		inverseUMatrix();
		matrixMultiplication(m_pUInverseMatrix, m_encodeLineSize, m_encodeLineSize,
				m_pLInverseMatrix, m_encodeLineSize, m_encodeLineSize, m_pEncodeInverseMatrix);
	}

	/* Internal member */
private:
	unsigned int m_encodeLineSize;		/* Encoding matrix line size */
	unsigned int m_limit;				/* Encoding matrix limitation */
	unsigned int m_curLine;				/* Cursor */
	T *m_pCauchyMatrix = NULL;			/* Cauchy matrix */
	T *m_pL = NULL;						/* Result of encoding matrix's LUFactorization  */
	T *m_pU = NULL;						/* Result of encoding matrix's LUFactorization  */
	T *m_pLInverseMatrix = NULL;		/* Inverse matrix of L */
	T *m_pUInverseMatrix = NULL;		/* Inverse matrix of U */
	T *m_pEncodeMatrix = NULL;			/* Encoding matrix */
	T *m_pEncodeInverseMatrix = NULL;	/* Inverse of Encoding matrix */
	E_RSCODE_STS m_error = e_rscode_sts_init;


	/* For debug */
public:
	void debug(void){
		const char *title[] = {"Cauchy Matrix",
				"L Matrix",
				"U Matrix",
				"Inverse L Matrix",
				"Inverse U Matrix",
				"Encoding Matrix",
				"Inverse Encoding Matrix",
				NULL};
		T *matrix[] = {m_pCauchyMatrix,
				m_pL,
				m_pU,
				m_pLInverseMatrix,
				m_pUInverseMatrix,
				m_pEncodeMatrix,
				m_pEncodeInverseMatrix,
				NULL};
		unsigned int size[] = {m_limit,
				m_encodeLineSize,
				m_encodeLineSize,
				m_encodeLineSize,
				m_encodeLineSize,
				m_encodeLineSize,
				m_encodeLineSize,
				0};
		for(unsigned int i=0; title[i]!=NULL; ++i){
			debug(title[i], matrix[i], size[i], m_encodeLineSize);
		}
		/* Verify if the LU factorization and inverse matrix was correct */
		debugMatrixMultiplication("LxU Matrix:", m_pL, m_encodeLineSize, m_encodeLineSize,
				m_pU, m_encodeLineSize, m_encodeLineSize);
		debugMatrixMultiplication("LxLInvwese", m_pL, m_encodeLineSize, m_encodeLineSize,
				m_pLInverseMatrix, m_encodeLineSize, m_encodeLineSize);
		debugMatrixMultiplication("UxUInvwese", m_pU, m_encodeLineSize, m_encodeLineSize,
				m_pUInverseMatrix, m_encodeLineSize, m_encodeLineSize);
		debugMatrixMultiplication("AxAInverse", m_pEncodeMatrix, m_encodeLineSize, m_encodeLineSize,
				m_pEncodeInverseMatrix, m_encodeLineSize, m_encodeLineSize);
	}
	/* For debug */
private:
	void setupDebugFormat(std::ostream &s, streamsize width, char c, ios_base::fmtflags fmtfl){
		dbg_w = s.width();
		s.width(3);
		dbg_c = s.fill();
		s.fill('0');
		dbg_f = fmtfl;
		s.setf(dbg_f, std::ios::basefield);
	}
	void clearDebugFormat(std::ostream &s){
		s.unsetf(dbg_f);
		s.fill(dbg_c);
		s.width(dbg_w);
	}
	void debug(const char *title, T *matrix, unsigned int sizeX, unsigned int sizeY) {
		cout << title << endl;
		for(int i=0; i<sizeX; ++i){
			cout << "L" ;
			setupDebugFormat(cout, 4, '0', std::ios::dec);
			cout << i+1 << ": ";
			clearDebugFormat(cout);
			for(int j=0; j<sizeY; ++j){
				output(*position(matrix, i, j), cout, T_IS_FLOATING());
			}
			cout << endl;
		}
	}
	int debugMatrixMultiplication(const char *title, T *x, unsigned int xSizeI, unsigned int xSizeJ,
			T *y, unsigned int ySizeI, unsigned int ySizeJ){
		int ret;
		T *r = new T[xSizeJ*ySizeI];
		ret = matrixMultiplication(x, xSizeI, xSizeJ, y, ySizeI, ySizeJ, r);
		if(ret == 0){
			debug(title, r, xSizeI, ySizeJ);
		}else{
			cout << "debugMatrixMultiple error." << endl;
		}
		delete [] r;
		return ret;
	}
private:
	std::streamsize dbg_w;
	char dbg_c;
	ios_base::fmtflags dbg_f;
};


#endif /* RSCODE_HH_ */
