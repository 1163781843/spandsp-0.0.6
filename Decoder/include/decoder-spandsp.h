#ifndef __DECODER_SPANDSP_H__
#define __DECODER_SPANDSP_H__

#include <iostream>
#include <string>

#include <spandsp.h>
#include <sndfile.h>

using namespace std;

#define SPANDSP_SAMPLES_LENGTH 160
#define SPANDSP_SAMPLES_RATE   8000

typedef enum standard {
	NONE_STANDARD,
	FSK_STANDARD,
	JFSK_STANDARD,
	DTMF_STANDARD
} standard;

class spandsp {
protected:
	spandsp(string &filename, bool debug = false);
public:
	virtual ~spandsp();
	virtual int init() = 0;
	virtual int destroy() = 0;
	virtual int start() = 0;
	virtual int stop() = 0;
	int open();
	int close();
	SNDFILE *get_sndfile() const;
	string &get_filename() const;
	bool get_debug() const;
	t30_state_t *get_t30_state() const;
private:
	bool debug;
	t30_state_t *t30_state;
	SNDFILE *sndfile;
	SF_INFO sndinfo;
	string &filename;
};

#endif
