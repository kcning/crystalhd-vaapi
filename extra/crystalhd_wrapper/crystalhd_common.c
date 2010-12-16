#include "crystalhd_common.h"

int _indent_level = 0;
pthread_mutex_t mutex_func = PTHREAD_MUTEX_INITIALIZER;

void * __dlopen_crystalhd_so()
{
	static void *chd_so = NULL;
	if (!chd_so)
	{
		chd_so = dlopen("libcrystalhd.so", RTLD_NOW);
		if (!chd_so)
		{
			printf("dlopen(\"libcrystalhd.so\", RTLD_NOW) failed!\n");
			exit(1);
		}
	}
	return chd_so;
}

const char *string_of_BC_STATUS(BC_STATUS status)
{
	switch (status) {
#define STATUS(sts) case BC_STS_##sts: return "BC_STS_" #sts
		STATUS(SUCCESS);
		STATUS(INV_ARG);
		STATUS(BUSY);
		STATUS(NOT_IMPL);
		STATUS(PGM_QUIT);
		STATUS(NO_ACCESS);
		STATUS(INSUFF_RES);
		STATUS(IO_ERROR);
		STATUS(NO_DATA);
		STATUS(VER_MISMATCH);
		STATUS(TIMEOUT);
		STATUS(FW_CMD_ERR);
		STATUS(DEC_NOT_OPEN);
		STATUS(ERR_USAGE);
		STATUS(IO_USER_ABORT);
		STATUS(IO_XFR_ERROR);
		STATUS(DEC_NOT_STARTED);
		STATUS(FWHEX_NOT_FOUND);
		STATUS(FMT_CHANGE);
		STATUS(HIF_ACCESS);
		STATUS(CMD_CANCELLED);
		STATUS(FW_AUTH_FAILED);
		STATUS(BOOTLOADER_FAILED);
		STATUS(CERT_VERIFY_ERROR);
		STATUS(DEC_EXIST_OPEN);
		STATUS(PENDING);
		STATUS(CLK_NOCHG);
		STATUS(ERROR);
#undef STATUS
	}
	return "<unknown>";
}

const char *string_of_BC_MEDIA_SUBTYPE(BC_MEDIA_SUBTYPE type)
{
	switch (type) {
#define SUBTYPE(t) case BC_MSUBTYPE_##t: return "BC_MSUBTYPE_" #t
		SUBTYPE(INVALID);
		SUBTYPE(MPEG1VIDEO);
		SUBTYPE(MPEG2VIDEO);
		SUBTYPE(H264);
		SUBTYPE(WVC1);
		SUBTYPE(WMV3);
		SUBTYPE(AVC1);
		SUBTYPE(WMVA);
		SUBTYPE(VC1);
		SUBTYPE(DIVX);
		SUBTYPE(DIVX311);
		SUBTYPE(OTHERS);	/* Types of facilitate PES conversion */
	}
#undef SUBTYPE
	return "<unknown>";
}
