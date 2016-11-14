/*
 * RScodeTest.cc
 *
 *  Created on: 2016/10/10
 *      Author: yanfeng
 */

#include <iostream>
#include <cstdio>
#include <algorithm>	/* for_each */

#include "GF28Value.hh"
#include "RScode.hh"

using namespace std;


/* Test */
const static unsigned int DATA_SIZE = 32;
const static unsigned int ENCODE_SIZE = 256;

static RScode<GF28Value> rs = RScode<GF28Value>(DATA_SIZE, DATA_SIZE);

static bool verifyData(unsigned char *encode, unsigned char *decode, unsigned int rows, unsigned int columns){
	for(unsigned int i=0; i<rows; ++i){
		for(unsigned int j=0; j<columns; ++j){
			if(*(encode+i*columns+j) != *(decode+i*columns+j)){
				cout << "verfy error at [" << i << "][" << j << "]" << endl;
				return false;
			}
		}
	}
	return true;
}

#include <time.h>
static void _doTest(unsigned int *lines, bool showResult){
	unsigned char data[DATA_SIZE][DATA_SIZE] = {{0}};		/* original data */
	unsigned char encode[ENCODE_SIZE][DATA_SIZE] = {{0}};	/* encoded data */
	unsigned char decode[DATA_SIZE][DATA_SIZE] = {{0}};		/* data which to be decode */
	unsigned char recover[DATA_SIZE][DATA_SIZE] = {{0}};	/* should be equal with original data */

	if(showResult){
		cout << "Original data:" << endl;
	}
	std::streamsize w = cout.width();
	cout.width(2);
	char f = cout.fill();
	cout.fill('0');
	cout.setf(std::ios::hex, std::ios::basefield);
	for(unsigned int i=0; i<DATA_SIZE; ++i){
		for(unsigned int j=0; j<DATA_SIZE; ++j){
			/* make data */
			data[i][j] = rand()%256;
			if(showResult){
				cout.width(2);	/* width reset to 0 automatically after inserting numbers to cout. */
				cout << (unsigned int)data[i][j] << "";
			}
		}
		if(showResult){
			cout << endl;
		}
	}
	cout.unsetf(std::ios::hex);
	cout.fill(f);
	cout.width(w);

	/* encode */
	for(unsigned int i=0; i<ENCODE_SIZE; ++i){
		rs.encodeLine((unsigned char *)data, DATA_SIZE, encode[i]);
	}
	/* swap rows of encode data */
	for(unsigned int i=0; i<DATA_SIZE; ++i){
		if(lines[i] < DATA_SIZE && lines[i] != i){
			unsigned int index = lines[i];
			lines[i] = lines[index];
			lines[index] = index;
		}
	}
	for(unsigned int i=0;i<DATA_SIZE; ++i){
		memcpy(decode[i], &encode[lines[i]], DATA_SIZE);
	}
	/* decode */
	rs.decode((unsigned char *)decode, lines, (unsigned char *)recover, DATA_SIZE);

	w = cout.width();
	cout.width(2);
	f = cout.fill();
	cout.fill('0');
	cout.setf(std::ios::hex, std::ios::basefield);
	if(showResult){
		cout<<"Recover data:"<<endl;
		for(unsigned int i=0; i<DATA_SIZE; ++i){
			for(unsigned int j=0; j<DATA_SIZE; ++j){
				cout.width(2);
				cout << (unsigned int)recover[i][j];
			}
			cout << endl;
		}
	}
	cout.unsetf(std::ios::hex);
	cout.fill(f);
	cout.width(w);

	bool res = verifyData((unsigned char*)data, (unsigned char*)recover, DATA_SIZE, DATA_SIZE);
	if(res){
		//cout << "verify ok" << endl;
	}else{
		cout << "verify error" << endl;
	}

	rs.clear();
}

#include <set>
static void testAll(void){
	srand (time(NULL));
	unsigned int indexArray[DATA_SIZE] = {0};

	/* no missing */
	cout <<"Test no missing:" << endl;
	for(unsigned int i=0; i<DATA_SIZE; ++i){
		indexArray[i] = i;
	}
	_doTest(indexArray, true);

	/* all missing */
	cout <<"Test all missing:" << endl;
	for(unsigned int i=0; i<DATA_SIZE; ++i){
		indexArray[i] = i+DATA_SIZE;
	}
	_doTest(indexArray, true);

	/* half missing */
	cout <<"Test half missing:" << endl;
	for(unsigned int i=0; i<DATA_SIZE; ++i){
		if(i%2){
			indexArray[i] = i+DATA_SIZE;
		}else{
			indexArray[i] = i;
		}
	}
	_doTest(indexArray, true);
	//rs.debug();

	/* random rows */
	cout <<"Test random rows:" << endl;
	const int maxTimes = 1000;
	for(unsigned int count=1; count<=maxTimes; count++){
		set<unsigned int> r;
		for(unsigned int i=0; i<DATA_SIZE; ++i){
			unsigned int v = rand()%256;
			while(r.find(v) != r.end()){
				v = rand()%256;
			}
			r.insert(v);
		}
		_doTest(indexArray, false);
		if(count % (maxTimes/10) == 0){
			cout << count <<" test passed..." << endl;
		}
	}
	cout << "done." << endl;

}

int main(void) {
	testAll();
	return 0;
}

