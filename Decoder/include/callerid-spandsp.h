#ifndef __CALLERID_SPANDSP_H__
#define __CALLERID_SPANDSP_H__

#include <sndfile.h>

#include <logger.h>

#include <spandsp.h>

#include <decoder-spandsp.h>

class callerid_spandsp : public spandsp {
public:
	callerid_spandsp(string &filename, bool debug = false, int type = ADSI_STANDARD_CLASS);
	virtual ~callerid_spandsp();
	int init();
	int destroy();
	int start();
	int stop();
	adsi_rx_state_t *get_callerid_rx() const;
	int get_standard() const;
	void class_callerid_standard(const uint8_t *msg, int len);
	void clip_callerid_standard(const uint8_t *msg, int len);
	void aclip_callerid_standard(const uint8_t *msg, int len);
	void jclip_callerid_standard(const uint8_t *msg, int len);
	void clip_dtmf_callerid_standard(const uint8_t *msg, int len);
	void tdd_callerid_standard(const uint8_t *msg, int len);
	int callerid_binary_data(char *msgstr, int msgstr_len, const uint8_t *msg, int len);
private:
	adsi_rx_state_t *callerid_rx;
	int standard;
	int16_t samples[SPANDSP_SAMPLES_LENGTH];
};

#endif
