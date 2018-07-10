#include <logger.h>

#include <decoder-spandsp.h>

#include <spandsp.h>
#include <spandsp/logging.h>
#include <spandsp/private/logging.h>
#include <spandsp/private/t4_t6_decode.h>
#include <spandsp/private/t4_t6_encode.h>
#include <spandsp/private/t4_rx.h>
#include <spandsp/private/t4_tx.h>
#include <spandsp/private/timezone.h>
#include <spandsp/private/t30.h>
#include <spandsp/private/hdlc.h>
#include <spandsp/timezone.h>
#include <spandsp/t30.h>
#include <spandsp/t4_rx.h>

#define SAMPLES_RATE 8000

static bool g_debug = false;

static void spandsp_log(int level, const char *file, int line, const char *msg)
{
	if (g_debug) {
		switch (level) {
			case SPAN_LOG_ERROR:
			case SPAN_LOG_PROTOCOL_ERROR:
				LOG(LOG_ERR, "[Spandsp]  <%s: %d>  %s\n", file, line, msg);
			case SPAN_LOG_WARNING:
			case SPAN_LOG_PROTOCOL_WARNING:
				LOG(LOG_WAR, "[Spandsp]  <%s: %d>  %s\n", file, line, msg);
				break;
			default:
				LOG(LOG_NOT, "[Spandsp]  <%s: %d>  %s", file, line, msg);
				break;
		}
	}
}

spandsp::spandsp(string &filename, bool debug) : filename(filename), debug(debug)
{
	t30_state = new t30_state_t;

	span_log_init(&t30_state->logging, SPAN_LOG_FLOW, NULL);
	span_log_set_protocol(&t30_state->logging, "Spandsp");
	span_log_set_message_handler(&t30_state->logging, spandsp_log);
	span_set_message_handler(spandsp_log);

	g_debug = debug;
}

spandsp::~spandsp()
{
	if (t30_state) {
		delete t30_state;
		t30_state = NULL;
	}
}

int spandsp::open()
{
	memset(&sndinfo, 0, sizeof(sndinfo));

	sndfile = sf_open(filename.c_str(), SFM_READ, &sndinfo);
	if (NULL == sndfile) {
		LOG(LOG_ERR, "Cannot open audio file '%s' for reading!\n", filename.c_str());
		return -1;
	}

	if (SAMPLES_RATE != sndinfo.samplerate) {
		LOG(LOG_ERR, "Unexpected sample rate in audio file '%s'!\n", filename.c_str());
		return -1;
	}

	if (1 != sndinfo.channels) {
		LOG(LOG_ERR, "Unexpected number of channels in audio file '%s'!\n", filename.c_str());
		return -1;
	}

	//LOG(LOG_DEB, "Open '%s' successfully!\n", filename.c_str());

	return 0;
}

int spandsp::close()
{
	if (sndfile) {
		sf_close(sndfile);
		sndfile = NULL;
	}
}

SNDFILE *spandsp::get_sndfile() const
{
	return sndfile;
}

string &spandsp::get_filename() const
{
	return filename;
}

bool spandsp::get_debug() const
{
	return debug;
}

t30_state_t *spandsp::get_t30_state() const
{
	return t30_state;
}
