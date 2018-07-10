#include <logger.h>

#include <sndfile.h>

#include <fax-spandsp.h>

#define FAX_SIGNAL_CONSOLE_STR "\033[33;1m%s\33[0m"
#define FAX_SIGNAL_CONSOLE     "\033[31;1m0x%02x\33[0m"

#define FAX_SPACE_MARK "--------------------------------------------------------------------------------"

static void fax_cng_ced_detected(void *user_data, int tone, int level, int delay)
{
	fax_spandsp *fax = (fax_spandsp *)user_data;

	switch (tone) {
		case MODEM_CONNECT_TONES_FAX_CNG:
			fax->set_cng_flags(true);
			break;
		case MODEM_CONNECT_TONES_FAX_CED:
			fax->set_ced_flags(true);
			break;
		default:
			break;
	}

	if (fax->get_cng_flags() || fax->get_ced_flags()) {
		LOG(LOG_NOT, FAX_SIGNAL_CONSOLE_STR" (%d)\n", modem_connect_tone_to_str(tone), tone);
	}
}

static char *fax_decode_blankspace_digit_msg(char *msgstr, int msgstr_len, const uint8_t *pkt, int len)
{
	int p;
	int k;
	char msg[T30_MAX_IDENT_LEN + 1] = {0};


	pkt += 2;
	p = len - 2;

	/* Strip trailing spaces */
	while (p > 1  && ' ' == pkt[p - 1]) {
		p--;
	}

	/* The string is actually backwards in the message */
	k = 0;
	while (p > 1) {
		msg[k++] = pkt[--p];
	}
	msg[k] = '\0';

	snprintf(msgstr, msgstr_len - 1, "%s", msg);

	return msgstr;
}

static const char *fax_name(uint8_t x)
{
	switch (x) {
		case T30_CSI:
			return "Called subscriber identification";
		case T30_TSI:
			return "Digital command signal";
		case T30_PWD:
			return "Password";
		case T30_SEP:
			return "Selective polling";
		case T30_SUB:
			return "Sub-address";
		case T30_SID:
			return "Sender identification";
	}
}

static int fax_decode_type_message(fax_spandsp *fax, char *msgstr, int msgstr_len, const uint8_t *msg, int len)
{
	const char *country = NULL, *vendor = NULL, *model = NULL;
	int x = 0, msg_len = 0;

	switch (msg[2] & 0xFE) {
		case T30_CSI:
		case T30_TSI:
		case T30_PWD:
		case T30_SEP:
		case T30_SUB:
		case T30_SID:
			{
				char temp[256] = {0};
				fax_decode_blankspace_digit_msg(temp, sizeof(temp), msg, len);
				msg_len += snprintf(msgstr + msg_len, msgstr_len - msg_len, "\n%s: [%s]!\n", fax_name(msg[2] & 0xFE), temp);
			}
			break;
		case T30_DIS:
		case T30_DTC:
		case T30_DCS:
			t30_decode_dis_dtc_dcs(fax->get_t30_state(), msg, len);
			break;
		case T30_NSF:
		case T30_NSS:
		case T30_NSC:
			if (t35_decode(&msg[x + 1], len - 3, &country, &vendor, &model)) {
				if (country) {
					msg_len += snprintf(msgstr + msg_len, msgstr_len - msg_len, "The remote was made in '%s'!\n", country);
				}

				if (vendor) {
					msg_len += snprintf(msgstr + msg_len, msgstr_len - msg_len, "The remote was made by '%s'!\n", vendor);
				}

				if (model) {
					msg_len += snprintf(msgstr + msg_len, msgstr_len - msg_len, "The remote is a '%s'!\n", model);
				}
			}
			break;
		default:
			break;
	}

	return msg_len;
}

static void fax_v21_put_bit(void *user_data, int bit)
{
	fax_spandsp *object = (fax_spandsp *)user_data;

	hdlc_rx_put_bit(object->get_hdlcrx(), bit);
}

static void fax_hdlc_accept(void *user_data, const uint8_t *msg, int len, int ok)
{
	fax_spandsp *fax = (fax_spandsp *)user_data;

	if (len < 0) {
		return;
	}

	if (len > 0 && ok) {
		if (0xFF != msg[0] || !(0x03 == msg[1] || 0x13 == msg[1])) {
			LOG(LOG_WAR, "Unidentified frame header - %02x %02x\n", msg[0], msg[1]);
			return;
		}

		fax->fax_frame_decoder(msg, len);
	}
}

fax_spandsp::fax_spandsp(string &filename, bool debug) : spandsp(filename, debug)
{}

fax_spandsp::~fax_spandsp()
{
	stop();
}

int fax_spandsp::init()
{
	hdlcrx = new hdlc_rx_state_t;

	if (NULL == hdlcrx) {
		LOG(LOG_ERR, "Init hdlcrx memory failure!\n");
		return -1;
	}

	cng_rx = new modem_connect_tones_rx_state_t;
	if (NULL == cng_rx) {
		LOG(LOG_ERR, "Init cng_rx memory failure!\n");
		return -1;
	}

	ced_rx = new modem_connect_tones_rx_state_t;
	if (NULL == ced_rx) {
		LOG(LOG_ERR, "Init ced_rx memory failure!\n");
		return -1;
	}

	modem_connect_tones_rx_init(cng_rx, MODEM_CONNECT_TONES_FAX_CNG, fax_cng_ced_detected, this);
	modem_connect_tones_rx_init(ced_rx, MODEM_CONNECT_TONES_FAX_CED, fax_cng_ced_detected, this);

	hdlc_rx_init(hdlcrx, FALSE, TRUE, 0, fax_hdlc_accept, this);

	fsk = fsk_rx_init(NULL, &preset_fsk_specs[FSK_V21CH2], FSK_FRAME_MODE_SYNC, fax_v21_put_bit, this);

	return 0;
}

int fax_spandsp::destroy()
{
	if (fsk) {
		fsk_rx_free(fsk);
		fsk = NULL;
	}

	if (hdlcrx) {
		delete hdlcrx;
		hdlcrx = NULL;
	}

	if (cng_rx) {
		delete cng_rx;
		cng_rx = NULL;
	}

	if (ced_rx) {
		delete ced_rx;
		ced_rx = NULL;
	}

	return 0;
}

int fax_spandsp::start()
{
	int length = 0;

	for (; ; ) {
		length = sf_readf_short(this->get_sndfile(), samples, SPANDSP_SAMPLES_LENGTH);
		if (0 == length) {
			//LOG(LOG_DEB, "Read audio '%s' file finish!\n", this->get_filename().c_str());
			break;
		}

		if (false == cng_flags) {
			modem_connect_tones_rx(cng_rx, samples, length);
		}

		if (false == ced_flags) {
			modem_connect_tones_rx(ced_rx, samples, length);
		}

		fsk_rx(fsk, samples, length);
	}

	return 0;
}

int fax_spandsp::stop()
{
	destroy();
}

fsk_rx_state_t *fax_spandsp::get_fsk() const
{
	return fsk;
}

hdlc_rx_state_t *fax_spandsp::get_hdlcrx() const
{
	return hdlcrx;
}

void fax_spandsp::set_cng_flags(bool flags)
{
	cng_flags |= flags;
}

bool fax_spandsp::get_cng_flags() const
{
	return cng_flags;
}

void fax_spandsp::set_ced_flags(bool flags)
{
	ced_flags |= flags;
}

bool fax_spandsp::get_ced_flags() const
{
	return ced_flags;
}

void fax_spandsp::fax_frame_decoder(const uint8_t *msg, int len)
{
	char msg_str[1024] = {0};
	int msg_len = 0, x = 0;

	uint8_t type = msg[2] & 0xFF;

	msg_len += snprintf(msg_str + msg_len, sizeof(msg_str) - msg_len - 1, "%s\n", FAX_SPACE_MARK);
	msg_len += snprintf(msg_str + msg_len, sizeof(msg_str) - msg_len - 1,
		FAX_SIGNAL_CONSOLE_STR"("FAX_SIGNAL_CONSOLE"):\n", t30_frametype(type), type);

	for (x = 0; x < len; x++) {
		if (2 == x) {
			msg_len += snprintf(msg_str + msg_len, sizeof(msg_str) - msg_len - 1, FAX_SIGNAL_CONSOLE" " , msg[x]);
		} else {
			msg_len += snprintf(msg_str + msg_len, sizeof(msg_str) - msg_len - 1, "0x%02x %s", msg[x], (0xF == (x & 0xF)) ? "\n" : "");
		}
	}

	msg_len += snprintf(msg_str + msg_len, sizeof(msg_str) - msg_len - 1, "%s", "\n");
	msg_len += fax_decode_type_message(this, msg_str + msg_len, sizeof(msg_str) - msg_len - 1, msg, len);
	msg_len += snprintf(msg_str + msg_len, sizeof(msg_str) - msg_len - 1, "%s\n", FAX_SPACE_MARK);

	LOG(LOG_VEB, "\n%s\n", msg_str);
}
