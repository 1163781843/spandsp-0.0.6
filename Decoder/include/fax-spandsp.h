#ifndef __FAX_SPANDSP_H__
#define __FAX_SPANDSP_H__

#include <iostream>
#include <string>

#include <spandsp.h>
#include <spandsp/private/power_meter.h>
#include <spandsp/private/fsk.h>
#include <spandsp/fsk.h>
#include <spandsp/private/modem_connect_tones.h>
#include <spandsp/modem_connect_tones.h>
#include <spandsp/private/hdlc.h>
#include <spandsp/hdlc.h>

#include <decoder-spandsp.h>

using namespace std;

class fax_spandsp : public spandsp {
public:
	fax_spandsp(string &filename, bool debug = false);
	~fax_spandsp();
	int init();
	int destroy();
	int start();
	int stop();
	fsk_rx_state_t *get_fsk() const;
	hdlc_rx_state_t *get_hdlcrx() const;
	void set_cng_flags(bool flags);
	bool get_cng_flags() const;
	void set_ced_flags(bool flags);
	bool get_ced_flags() const;
	void fax_frame_decoder(const uint8_t *msg, int len);
private:
	fsk_rx_state_t *fsk;
	hdlc_rx_state_t *hdlcrx;
	modem_connect_tones_rx_state_t *cng_rx;
	modem_connect_tones_rx_state_t *ced_rx;
	int cng_flags;
	int ced_flags;
	int16_t samples[SPANDSP_SAMPLES_LENGTH];
};

#endif
