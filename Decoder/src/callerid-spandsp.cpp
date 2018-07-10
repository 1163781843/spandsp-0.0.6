#include <callerid-spandsp.h>

#define CALLERID_SPACE_MARK "-----------------------------------------------------------------------------------------"

#define CALLERID_SIGNAL_CONSOLE     "\033[31;1m0x%02x\33[0m"

static void callerid_msg(void *user_data, const uint8_t *msg, int len)
{
	callerid_spandsp *caller_id = (callerid_spandsp *)user_data;

	switch (caller_id->get_standard()) {
		case ADSI_STANDARD_CLASS:
			caller_id->class_callerid_standard(msg, len);
			break;
		case ADSI_STANDARD_CLIP:
			caller_id->clip_callerid_standard(msg, len);
			break;
		case ADSI_STANDARD_ACLIP:
			caller_id->aclip_callerid_standard(msg, len);
			break;
		case ADSI_STANDARD_JCLIP:
			caller_id->jclip_callerid_standard(msg, len);
			break;
		case ADSI_STANDARD_CLIP_DTMF:
			caller_id->clip_dtmf_callerid_standard(msg, len);
			break;
		case ADSI_STANDARD_TDD:
			caller_id->tdd_callerid_standard(msg, len);
			break;
		default:
			LOG(LOG_WAR, "Cannot support '%d' standard decode!\n", caller_id->get_standard());
			break;
	}
}

callerid_spandsp::callerid_spandsp(string &filename, bool debug, int type) : spandsp(filename, debug)
{
	switch (type) {
		case NONE_STANDARD:
		case FSK_STANDARD:
			standard = ADSI_STANDARD_CLASS;
			break;
		case JFSK_STANDARD:
			standard = ADSI_STANDARD_JCLIP;
			break;
		case DTMF_STANDARD:
			standard = ADSI_STANDARD_CLIP_DTMF;
			break;
		default:
			standard = ADSI_STANDARD_CLASS;;
			break;
	}
}

callerid_spandsp::~callerid_spandsp()
{
	stop();
}

int callerid_spandsp::init()
{
	callerid_rx = adsi_rx_init(NULL, standard, callerid_msg, this);
}

int callerid_spandsp::destroy()
{
	if (callerid_rx) {
		adsi_rx_free(callerid_rx);
		callerid_rx = NULL;
	}
}

int callerid_spandsp::start()
{
	int length = 0;

	for (; ; ) {
		length = sf_readf_short(this->get_sndfile(), samples, SPANDSP_SAMPLES_LENGTH);
		if (0 == length) {
			//LOG(LOG_DEB, "Read audio '%s' file finish!\n", this->get_filename().c_str());
			break;
		}

		adsi_rx(callerid_rx, samples, length);
	}
}

int callerid_spandsp::stop()
{
	destroy();
}

adsi_rx_state_t *callerid_spandsp::get_callerid_rx() const
{
	return callerid_rx;
}

int callerid_spandsp::get_standard() const
{
	return standard;
}

void callerid_spandsp::class_callerid_standard(const uint8_t *msg, int len)
{
	char msg_binary[1024] = {0};
	int msg_binary_len = 0;
	int position = - 1, message_type = -1;
	uint8_t field_type = 0;
	const uint8_t *field_body = NULL;
	int field_len = 0;
	uint8_t body[256] = {0};

	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "%s\n", CALLERID_SPACE_MARK);
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Received binary callerid data(%d) bytes!\n", len);
	msg_binary_len += callerid_binary_data(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, msg, len);

	do {
		position = adsi_next_field(get_callerid_rx(), msg, len, position, &field_type, &field_body, &field_len);
		if (position > 0) {
			if (field_body) {
				memcpy(body, field_body, field_len);
				body[field_len] = '\0';
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Field type: "CALLERID_SIGNAL_CONSOLE", length: %d, ", field_type, field_len);

				switch (message_type) {
					case CLASS_SDMF_CALLERID:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Data and time or CallerID[%s]\n", body);
						break;
					case CLASS_MDMF_CALLERID:
						switch (field_type) {
							case MCLASS_DATETIME:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Date and time (MMDDHHMM): [%s]\n", body);
								break;
							case MCLASS_CALLER_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number: [%s]\n", body);
								break;
							case MCLASS_DIALED_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Dialed number: [%s]\n", body);
								break;
							case MCLASS_ABSENCE1:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number absent: 'O' or 'P': [%s]\n", body);
								break;
							case MCLASS_REDIRECT:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Call forward: universal ('0'), on busy ('1'), or on unanswered ('2'): [%s]\n", body);
								break;
							case MCLASS_QUALIFIER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Long distance: 'L': [%s]\n", body);
								break;
							case MCLASS_CALLER_NAME:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's name: [%s]\n", body);
								break;
							case MCLASS_ABSENCE2:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's name absent: 'O' or 'P': [%s]\n", body);
								break;
							}
						break;
					case CLASS_SDMF_MSG_WAITING:
						break;
					case CLASS_MDMF_MSG_WAITING:
						switch (field_type) {
							case MCLASS_VISUAL_INDICATOR:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Message waiting/not waiting\n");
								break;
							case MCLASS_DATETIME:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Date and time (MMDDHHMM): [%s]\n", body);
								break;
							case MCLASS_CALLER_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number: [%s]\n", body);
								break;
							case MCLASS_DIALED_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Dialed number: [%s]\n", body);
								break;
							case MCLASS_ABSENCE1:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number absent: 'O' or 'P': [%s]\n", body);
								break;
							case MCLASS_REDIRECT:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Call forward: universal ('0'), on busy ('1'), or on unanswered ('2'): [%s]\n", body);
								break;
							case MCLASS_QUALIFIER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Long distance: 'L': [%s]\n", body);
								break;
							case MCLASS_CALLER_NAME:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's name: [%s]\n", body);
								break;
							case MCLASS_ABSENCE2:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's name absent: 'O' or 'P': [%s]\n", body);
								break;
						}
						break;
				}
			} else {
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Message type: "CALLERID_SIGNAL_CONSOLE", ", field_type);
				message_type = field_type;
				switch (message_type) {
					case CLASS_SDMF_CALLERID:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Single data message caller ID\n");
						break;
					case CLASS_MDMF_CALLERID:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Multiple data message caller ID\n");
						break;
					case CLASS_SDMF_MSG_WAITING:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Single data message message waiting\n");
						break;
					case CLASS_MDMF_MSG_WAITING:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Multiple data message message waiting\n");
						break;
					default:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Unknown\n");
						break;
				}
			}
		}
	} while (position > 0);

	if (position < -1) {
		msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\nBad callerid message contents\n");
	}
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\n%s\n", CALLERID_SPACE_MARK);

	LOG(LOG_DEB, "\n%s\n", msg_binary);

}

void callerid_spandsp::clip_callerid_standard(const uint8_t *msg, int len)
{
	char msg_binary[1024] = {0};
	int msg_binary_len = 0;
	int position = - 1;
	uint8_t field_type = 0, message_type = -1;
	const uint8_t *field_body = NULL;
	int field_len = 0;
	uint8_t body[256] = {0};

	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "%s\n", CALLERID_SPACE_MARK);
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Received binary callerid data(%d) bytes!\n", len);
	msg_binary_len += callerid_binary_data(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, msg, len);

	do {
		position = adsi_next_field(get_callerid_rx(), msg, len, position, &field_type, &field_body, &field_len);
		if (position > 0) {
			if (field_body) {
				memcpy(body, field_body, field_len);
				body[field_len] = '\0';
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Field type: "CALLERID_SIGNAL_CONSOLE", length: %d\n", field_type, field_len);
				switch (message_type) {
					case CLIP_MDMF_CALLERID:
					case CLIP_MDMF_MSG_WAITING:
					case CLIP_MDMF_CHARGE_INFO:
					case CLIP_MDMF_SMS:
						switch (field_type) {
							case CLIP_DATETIME:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Date and time (MMDDHHMM): [%s]\n", body);
								break;
							case CLIP_CALLER_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number: [%s]\n", body);
								break;
							case CLIP_DIALED_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Dialed number: [%s]\n", body);
								break;
							case CLIP_ABSENCE1:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number absent: [%s]\n", body);
								break;
							case CLIP_CALLER_NAME:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's name: [%s]\n", body);
								break;
							case CLIP_ABSENCE2:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's name absent: [%s]\n", body);
								break;
							case CLIP_VISUAL_INDICATOR:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Visual indicator: [%s]\n", body);
								break;
							case CLIP_MESSAGE_ID:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Message ID: [%s]\n", body);
								break;
							case CLIP_CALLTYPE:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Voice call, ring-back-when-free call, or msg waiting call: [%s]\n", body);
								break;
							case CLIP_NUM_MSG:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Number of messages: [%s]\n", body);
								break;
							case CLIP_CHARGE:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Charge: [%s]\n", body);
								break;
							case CLIP_DURATION:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Duration of the call: [%s]\n", body);
								break;
							case CLIP_ADD_CHARGE:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Additional charge: [%s]\n", body);
								break;
							case CLIP_DISPLAY_INFO:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Display information: [%s]\n", body);
								break;
							case CLIP_SERVICE_INFO:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Service information: [%s]\n", body);
								break;
						}
						break;
				}
			} else {
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Message type: "CALLERID_SIGNAL_CONSOLE"\n", field_type);
				message_type = field_type;
				switch (message_type) {
					case CLIP_MDMF_CALLERID:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Multiple data message caller ID\n");
						break;
					case CLIP_MDMF_MSG_WAITING:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Multiple data message message waiting\n");
						break;
					case CLIP_MDMF_CHARGE_INFO:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Multiple data message charge info\n");
						break;
					case CLIP_MDMF_SMS:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Multiple data message SMS\n");
						break;
					default:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Unknown\n");
						break;
				}
			}
		}
	} while (position > 0);

	if (position < -1) {
		msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\nBad callerid message contents\n");
	}
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\n%s\n", CALLERID_SPACE_MARK);

	LOG(LOG_DEB, "\n%s\n", msg_binary);

}

void callerid_spandsp::aclip_callerid_standard(const uint8_t *msg, int len)
{
	char msg_binary[1024] = {0};
	int msg_binary_len = 0;
	int position = - 1;
	uint8_t field_type = 0, message_type = -1;
	const uint8_t *field_body = NULL;
	int field_len = 0;
	uint8_t body[256] = {0};

	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "%s\n", CALLERID_SPACE_MARK);
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Received binary callerid data(%d) bytes!\n", len);
	msg_binary_len += callerid_binary_data(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, msg, len);

	do {
		position = adsi_next_field(get_callerid_rx(), msg, len, position, &field_type, &field_body, &field_len);
		if (position > 0) {
			if (field_body) {
				memcpy(body, field_body, field_len);
				body[field_len] = '\0';
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Field type: "CALLERID_SIGNAL_CONSOLE", length: %d, ", field_type, field_len);
				switch (message_type) {
					case ACLIP_SDMF_CALLERID:
						break;
					case ACLIP_MDMF_CALLERID:
						switch (field_type) {
							case ACLIP_DATETIME:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Date and time (MMDDHHMM): [%s]\n", body);
								break;
							case ACLIP_CALLER_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number: [%s]\n", body);
								break;
							case ACLIP_DIALED_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Dialed number: [%s]\n", body);
								break;
							case ACLIP_NUMBER_ABSENCE:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number absent: 'O' or 'P': [%s]\n", body);
								break;
							case ACLIP_REDIRECT:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Call forward: universal, on busy, or on unanswered: [%s]\n", body);
								break;
							case ACLIP_QUALIFIER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Long distance call: 'L': [%s]\n", body);
								break;
							case ACLIP_CALLER_NAME:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's name: [%s]\n", body);
								break;
							case ACLIP_NAME_ABSENCE:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's name absent: 'O' or 'P': [%s]\n", body);
								break;
						}
						break;
				}
			} else {
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Message type: "CALLERID_SIGNAL_CONSOLE", ", field_type);
				message_type = field_type;
				switch (message_type) {
					case ACLIP_SDMF_CALLERID:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Single data message caller ID frame\n");
						break;
					case ACLIP_MDMF_CALLERID:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Multiple data message caller ID frame\n");
						break;
					default:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Unknown\n");
						break;
				}
			}
		}
	} while (position > 0);

	if (position < -1) {
		msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\nBad callerid message contents\n");
	}
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\n%s\n", CALLERID_SPACE_MARK);

	LOG(LOG_DEB, "\n%s\n", msg_binary);

}

void callerid_spandsp::jclip_callerid_standard(const uint8_t *msg, int len)
{
	char msg_binary[1024] = {0};
	int msg_binary_len = 0;
	int position = - 1, message_type = -1;
	uint8_t field_type = 0;
	const uint8_t *field_body = NULL;
	int field_len = 0;
	uint8_t body[256] = {0};

	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "%s\n", CALLERID_SPACE_MARK);
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Received binary callerid data(%d) bytes!\n", len);
	msg_binary_len += callerid_binary_data(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, msg, len);

	do {
		position = adsi_next_field(get_callerid_rx(), msg, len, position, &field_type, &field_body, &field_len);
		if (position > 0) {
			if (field_body) {
				memcpy(body, field_body, field_len);
				body[field_len] = '\0';
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Field type: "CALLERID_SIGNAL_CONSOLE", length: %d, ", field_type, field_len);

				switch (message_type) {
					case JCLIP_MDMF_CALLERID:
						switch (field_type) {
							case JCLIP_CALLER_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number: [%s]\n", body);
								break;
							case JCLIP_CALLER_NUM_DES:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number data extension signal: [%s]\n", body);
								break;
							case JCLIP_DIALED_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Dialed number: [%s]\n", body);
								break;
							case JCLIP_DIALED_NUM_DES:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Dialed number data extension signal: [%s]\n", body);
								break;
							case JCLIP_ABSENCE:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number absent: 'C', 'O', 'P' or 'S': [%s]\n", body);
								break;
						}
						break;
				}
			} else {
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Message type: "CALLERID_SIGNAL_CONSOLE", ", field_type);
				message_type = field_type;
				switch (message_type) {
					case JCLIP_MDMF_CALLERID:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Multiple data message caller ID frame\n");
						break;
					default:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Unknow\n");
						break;
				}
			}
		}
	} while (position > 0);

	if (position < -1) {
		msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\nBad callerid message contents\n");

		switch (message_type) {}
	}
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\n%s\n", CALLERID_SPACE_MARK);

	LOG(LOG_DEB, "\n%s\n", msg_binary);
}

void callerid_spandsp::clip_dtmf_callerid_standard(const uint8_t *msg, int len)
{
	char msg_binary[1024] = {0};
	int msg_binary_len = 0;
	int position = - 1;
	uint8_t field_type = 0, message_type = -1;
	const uint8_t *field_body = NULL;
	int field_len = 0;
	uint8_t body[256] = {0};

	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "%s\n", CALLERID_SPACE_MARK);
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Received binary callerid data(%d) bytes!\n", len);
	msg_binary_len += callerid_binary_data(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, msg, len);

	do {
		position = adsi_next_field(get_callerid_rx(), msg, len, position, &field_type, &field_body, &field_len);
		if (position > 0) {
			if (field_body) {
				memcpy(body, field_body, field_len);
				body[field_len] = '\0';
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Field type: "CALLERID_SIGNAL_CONSOLE", length: %d\n", field_type, field_len);
				switch (message_type) {
					case CLIP_DTMF_HASH_TERMINATED:
						switch (field_type) {
							case CLIP_DTMF_HASH_CALLER_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number: [%s]\n", body);
								break;
							case CLIP_DTMF_HASH_ABSENCE:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number absent: private (1), overseas (2) or not available (3): [%s]\n", body);
								break;
							case CLIP_DTMF_HASH_UNSPECIFIED:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Unspecified: [%s]\n", body);
								break;
						}
						break;
					case CLIP_DTMF_C_TERMINATED:
						switch (field_type) {
							case CLIP_DTMF_C_CALLER_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number: [%s]\n", body);
								break;
							case CLIP_DTMF_C_REDIRECT_NUMBER:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Redirect number: [%s]\n", body);
								break;
							case CLIP_DTMF_C_ABSENCE:
								msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Caller's number absent: [%s]\n", body);
								break;
						}
						break;
				}
			} else {
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Message type: "CALLERID_SIGNAL_CONSOLE", ", field_type);
				message_type = field_type;
				switch (message_type) {
					case CLIP_DTMF_HASH_TERMINATED:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "# terminated\n");
						break;
					case CLIP_DTMF_C_TERMINATED:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "C terminated\n");
						break;
					default:
						msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Unknow\n");
						break;
				}
			}
		}
	} while (position > 0);

	if (position < -1) {
		msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\nBad callerid message contents\n");
	}
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\n%s\n", CALLERID_SPACE_MARK);

	LOG(LOG_DEB, "\n%s\n", msg_binary);

}

void callerid_spandsp::tdd_callerid_standard(const uint8_t *msg, int len)
{
	char msg_binary[1024] = {0};
	int msg_binary_len = 0;
	int position = - 1;
	uint8_t field_type = 0, message_type = -1;
	const uint8_t *field_body = NULL;
	int field_len = 0;
	uint8_t body[256] = {0};

	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "%s\n", CALLERID_SPACE_MARK);
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Received binary callerid data(%d) bytes!\n", len);
	msg_binary_len += callerid_binary_data(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, msg, len);

	do {
		position = adsi_next_field(get_callerid_rx(), msg, len, position, &field_type, &field_body, &field_len);
		if (position > 0) {
			if (field_body) {
				memcpy(body, field_body, field_len);
				body[field_len] = '\0';
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Field type: "CALLERID_SIGNAL_CONSOLE", length: %d\n", field_type, field_len);
				if (len != 59 || memcmp(msg, "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG 0123456789#$*()", 59)) {
					msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "String error\n");
				}
			} else {
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Message type: "CALLERID_SIGNAL_CONSOLE", ", field_type);
				message_type = field_type;
				msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "Unknow\n");
			}
		}
	} while (position > 0);

	if (position < -1) {
		msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\nBad callerid message contents\n");
	}
	msg_binary_len += snprintf(msg_binary + msg_binary_len, sizeof(msg_binary) - msg_binary_len - 1, "\n%s\n", CALLERID_SPACE_MARK);

	LOG(LOG_DEB, "\n%s\n", msg_binary);

}

int callerid_spandsp::callerid_binary_data(char *msgstr, int msgstr_len, const uint8_t *msg, int len)
{
	int length = 0;

	for (int x = 0; x < len; x++) {
		if (0xf == (x & 0xf)) {
			length += snprintf(msgstr + length, msgstr_len - length, "%s0x%02x  ", "\n", msg[x]);
		} else {
			length += snprintf(msgstr + length, msgstr_len - length, "0x%02x  ", msg[x]);
		}
	}
	length += snprintf(msgstr + length, msgstr_len - length, "%s", "\n");

	return length;
}
