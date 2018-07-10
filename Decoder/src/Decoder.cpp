#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <cstring>

#include <logger.h>

#include <decoder-spandsp.h>
#include <callerid-spandsp.h>
#include <fax-spandsp.h>

using namespace std;

typedef enum decoder {
	MODE_NONE,
	FAX_MODE,
	CALLERID_MODE
} decoder;

static char help[] = {
	"Usage: help information\n\n"
	"    -d  printf spandsp log\n"
	"    -f  attach audio file\n"
	"    -m  select decoder mode, options[fax, callerid]\n"
	"        fax decoder fax signal\n\n"
	"    fax mode:\n"
	"        decoder t.30 fax signal\n\n"
	"    callerid mode:\n"
	"        -s  select callerid decoder standard, options[FSK, JFSK, DTMF], default FSK\n"
};

int main(int argc, char **argv)
{
	int options = -1;
	int enable_debug = false;
	string filename;
	decoder mode = MODE_NONE;
	standard cidstandard = NONE_STANDARD;
	spandsp *object = NULL;

	while (-1 != (options = getopt(argc, argv, "df:m:s:"))) {
		switch (options) {
			case 'd':
				enable_debug = true;
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				if (0 == strcasecmp("fax", optarg)) {
					mode = FAX_MODE;
				} else if (0 == strcasecmp("callerid", optarg)) {
					mode = CALLERID_MODE;
				} else {
					fprintf(stderr, "%s\n", help);
					return -1;
				}
				break;
			case 's':
				if (0 == strcasecmp("FSK", optarg)) {
					cidstandard = FSK_STANDARD;
				} else if (0 == strcasecmp("JFSK", optarg)) {
					cidstandard = JFSK_STANDARD;
				} else if (0 == strcasecmp("DTMF", optarg)) {
					cidstandard = DTMF_STANDARD;
				} else {
					cidstandard = FSK_STANDARD;
				}
				break;
		}
	}

	if (MODE_NONE == mode) {
		fprintf(stderr, "%s\n", help);
		return -1;
	}

	switch (mode) {
		case FAX_MODE:
			object = new fax_spandsp(filename, enable_debug);
			break;
		case CALLERID_MODE:
			object = new callerid_spandsp(filename, enable_debug, cidstandard);
			break;
		default:
			fprintf(stderr, "%s\n", help);
			return -1;
	}

	//LOG(LOG_DEB, "filename: %s, enable_debug: %d, cidstandard: %d, object: %p, mode: %d\n", filename.c_str(), enable_debug, cidstandard, object, mode);

	if (object) {
		/*! Open audio file! */
		object->open();

		/*! Init spandsp! */
		object->init();

		/*! Start detected! */
		object->start();

		/*! Stop detected! */
		object->stop();

		/*! Destroy spandsp! */
		object->destroy();

		/*! Close audio file! */
		object->close();

		/*! Delete memory! */
		delete object;
	} else {
		fprintf(stderr, "New memory failure!\n");
		fprintf(stderr, "%s\n", help);
		return -1;
	}

	return 0;
}
