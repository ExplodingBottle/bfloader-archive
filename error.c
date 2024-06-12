#include "error.h"
#include <stdio.h>

void handle_error(int error)
{
	const char *s;

	switch (error) {
		case ERR_POLL_TIMEOUT:
			s = "Poll/status timeout"; break;
		case ERR_VERIFY_WRITE:
			s = "Verify failed"; break;
		case ERR_INVALID_SECTOR:
			s = "Invalid sector"; break;
		case ERR_INVALID_BLOCK:
			s = "Invalid block"; break;
		case ERR_UNKNOWN_COMMAND:
			s = "Unknown command"; break;
		case ERR_PROCESS_COMMAND:
			s = "Error when processing command"; break;
		case ERR_READ:
			s = "Read error"; break;
		case ERR_DRV_NOT_AT_BREAK:
			s = "Fatal: Driver not at break"; break;
		case ERR_BUFFER_IS_NULL:
			s = "Fatal: Buffer is NULL"; break;
		case ERR_BUFSIZE_EXCESS:
			s = "Buffer Size exceeded"; break;
		case ERR_CHIP_UNSUPPORTED:
			s = "Flash chip unsupported"; break;
		default:
			fprintf(stderr, "Error %d\n", error);
			s = "Unknown error"; break;
	}
	fprintf(stderr, "Error: %s\n", s);
}

