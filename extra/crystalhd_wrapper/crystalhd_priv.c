#include "crystalhd_common.h"

#if 0
FUNC_WRAPPER(
	DtsDeviceOpen, BC_STATUS, LIST(HANDLE *hDevice, uint32_t mode),
	LIST(HANDLE *, uint32_t), LIST(hDevice, mode),
	"hDevice = %p, mode = %u"
);

FUNC_WRAPPER(
	DtsDeviceClose, BC_STATUS, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p"
);

FUNC_WRAPPER(
	DtsGetVersion, BC_STATUS, LIST(HANDLE hDevice, uint32_t *DrVer, uint32_t *DilVer),
	LIST(HANDLE *, uint32_t *, uint32_t *), LIST(hDevice, DrVer, DilVer),
	"hDevice = %p, DrVer = %p, DilVer = %p",
	LIST(
		eprintf("*DrVer = 0x%08x (%u)", *DrVer, *DrVer);
		eprintf("*DilVer = 0x%08x (%u)", *DilVer, *DilVer);
	)
);

FUNC_WRAPPER(
	DtsGetFWVersionFromFile, BC_STATUS, LIST(HANDLE hDevice, uint32_t *StreamVer, uint32_t *DecVer, char *fname),
	LIST(HANDLE *, uint32_t *, uint32_t *, char *), LIST(hDevice, StreamVer, DecVer, fname),
	"hDevice = %p, StreamVer = %p, DecVer = %p, fname = %s");

FUNC_WRAPPER(
	DtsGetFWVersion, BC_STATUS, LIST(HANDLE hDevice, uint32_t *StreamVer, uint32_t *DecVer, uint32_t *HwVer, char *fname, uint32_t flag),
	LIST(HANDLE *, uint32_t *, uint32_t *, uint32_t *, char *, uint32_t), LIST(hDevice, StreamVer, DecVer, HwVer, fname, flag),
	"hDevice = %p, StreamVer = %p, DecVer = %p, HwVer = %p, fname = %s, flag = %u");

FUNC_WRAPPER(
	DtsOpenDecoder, BC_STATUS, LIST(HANDLE hDevice, uint32_t StreamType),
	LIST(HANDLE, uint32_t), LIST(hDevice, StreamType),
	"hDevice = %p, StreamType = %u");

FUNC_WRAPPER(
	DtsCloseDecoder, BC_STATUS, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p");

FUNC_WRAPPER(
	DtsStartDecoder, BC_STATUS, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p");

FUNC_WRAPPER(
	DtsSetVideoParams, BC_STATUS, LIST(HANDLE hDevice, uint32_t videoAlg, BOOL FGTEnable, BOOL MetaDataEnable, BOOL Progressive, uint32_t OptFlags),
	LIST(HANDLE, uint32_t, BOOL, BOOL, BOOL, uint32_t), LIST(hDevice, videoAlg, FGTEnable, MetaDataEnable, Progressive, OptFlags),
	"hDevice = %p, videoAlg = %u, FGTEnable = %u, MetaDataEnable = %u, Progressive = %u, OptFlags = %u");

FUNC_WRAPPER(
	DtsSetInputFormat, BC_STATUS, LIST(HANDLE hDevice, BC_INPUT_FORMAT *pInputFormat),
	LIST(HANDLE, BC_INPUT_FORMAT *), LIST(hDevice, pInputFormat),
	"hDevice = %p, pInputFormat = %p");

FUNC_WRAPPER(
	DtsFormatChange, BC_STATUS, LIST(HANDLE hDevice, uint32_t videoAlg, BOOL FGTEnable, BOOL MetaDataEnable, BOOL Progressive, uint32_t reserved),
	LIST(HANDLE, uint32_t, BOOL, BOOL, BOOL, uint32_t), LIST(hDevice, videoAlg, FGTEnable, MetaDataEnable, Progressive, reserved),
	"hDevice = %p, videoAlg = %u, FGTEnable = %u, MetaDataEnable = %u, Progressive = %u, reserved = %u");

FUNC_WRAPPER(
	DtsStopDecoder, BC_STATUS, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p");

FUNC_WRAPPER(
	DtsPauseDecoder, BC_STATUS, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p");

FUNC_WRAPPER(
	DtsResumeDecoder, BC_STATUS, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p");

FUNC_WRAPPER(
	DtsSetVideoPID, BC_STATUS, LIST(HANDLE hDevice, uint32_t pid),
	LIST(HANDLE, uint32_t), LIST(hDevice, pid),
	"hDevice = %p, pInputFormat = %u");

FUNC_WRAPPER(
	DtsStartCaptureImmidiate, BC_STATUS, LIST(HANDLE hDevice, uint32_t reserved),
	LIST(HANDLE, uint32_t), LIST(hDevice, reserved),
	"hDevice = %p, reserved = %u");

FUNC_WRAPPER(
	DtsStartCapture, BC_STATUS, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p");

FUNC_WRAPPER(
	DtsFlushRxCapture, BC_STATUS, LIST(HANDLE hDevice, BOOL bDiscardOnly),
	LIST(HANDLE, BOOL), LIST(hDevice, bDiscardOnly),
	"hDevice = %p, bDiscardOnly = %u");

#include <execinfo.h>
#include <unistd.h>

FUNC_WRAPPER(
	DtsProcOutput, BC_STATUS, LIST(HANDLE hDevice, uint32_t milliSecWait, BC_DTS_PROC_OUT *pOut),
	LIST(HANDLE, uint32_t, BC_DTS_PROC_OUT *), LIST(hDevice, milliSecWait, pOut),
	"hDevice = %p, milliSecWait = %u, pOut = %p",
	LIST(
		eprintf("pOut = {");
		eprintf("    Ybuff = %p (YbuffSz = %u, YBuffDoneSz = %u)", pOut->Ybuff, pOut->YbuffSz, pOut->YBuffDoneSz);
		eprintf("    UVbuff = %p (UVbuffSz = %u, UVBuffDoneSz = %u)", pOut->UVbuff, pOut->UVbuffSz, pOut->UVBuffDoneSz);
		eprintf("    StrideSz = %u, PoutFlags = 0x%08x, discCnt = %u", pOut->StrideSz, pOut->PoutFlags, pOut->discCnt);
		eprintf("    PicInfo = {");
		eprintf("        timeStamp = %llu, picture_number = %u", pOut->PicInfo.timeStamp, pOut->PicInfo.picture_number);
		eprintf("        width = %u, height = %u, chroma_format = 0x%03x", pOut->PicInfo.width, pOut->PicInfo.height, pOut->PicInfo.chroma_format);
		eprintf("    }");
		eprintf("}");
	)
);

FUNC_WRAPPER(
	DtsProcOutputNoCopy, BC_STATUS, LIST(HANDLE hDevice, uint32_t milliSecWait, BC_DTS_PROC_OUT *pOut),
	LIST(HANDLE, uint32_t, BC_DTS_PROC_OUT *), LIST(hDevice, milliSecWait, pOut),
	"hDevice = %p, milliSecWait = %u, pOut = %p");

FUNC_WRAPPER(
	DtsReleaseOutputBuffs, BC_STATUS, LIST(HANDLE hDevice, PVOID reserved, BOOL fChange),
	LIST(HANDLE, PVOID, BOOL), LIST(hDevice, reserved, fChange),
	"hDevice = %p, reserved = %p, fChange = %u");

FUNC_WRAPPER(
	DtsProcInput, BC_STATUS, LIST(HANDLE hDevice, uint8_t *pUserData, uint32_t ulSizeInBytes, uint64_t timeStamp, BOOL encrypted),
	LIST(HANDLE, uint8_t *, uint32_t, uint64_t, BOOL), LIST(hDevice, pUserData, ulSizeInBytes, timeStamp, encrypted),
	"hDevice = %p, pUserData = %p, ulSizeInBytes = %u, timeStamp = %llu, encrypted = %u");

FUNC_WRAPPER(
	DtsGetColorPrimaries, BC_STATUS, LIST(HANDLE hDevice, uint32_t *colorPrimaries),
	LIST(HANDLE, uint32_t *), LIST(hDevice, colorPrimaries),
	"hDevice = %p, colorPrimaries = %p");

FUNC_WRAPPER(
	DtsFlushInput, BC_STATUS, LIST(HANDLE hDevice, uint32_t mode),
	LIST(HANDLE, uint32_t), LIST(hDevice, mode),
	"hDevice = %p, mode = %u");

FUNC_WRAPPER(
	DtsSetRateChange, BC_STATUS, LIST(HANDLE hDevice, uint32_t mode, uint8_t direction),
	LIST(HANDLE, uint32_t, uint8_t), LIST(hDevice, mode, direction),
	"hDevice = %p, mode = %u, direction = %u");

FUNC_WRAPPER(
	DtsSetFFRate, BC_STATUS, LIST(HANDLE hDevice, uint32_t rate),
	LIST(HANDLE, uint32_t), LIST(hDevice, rate),
	"hDevice = %p, rate = %u");

FUNC_WRAPPER(
	DtsSetSkipPictureMode, BC_STATUS, LIST(HANDLE hDevice, uint32_t SkipMode),
	LIST(HANDLE, uint32_t), LIST(hDevice, SkipMode),
	"hDevice = %p, SkipMode = %u");

FUNC_WRAPPER(
	DtsSetIFrameTrickMode, BC_STATUS, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p");

FUNC_WRAPPER(
	DtsStepDecoder, BC_STATUS, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p");

FUNC_WRAPPER(
	DtsIs422Supported, BC_STATUS, LIST(HANDLE hDevice, uint8_t *bSupported),
	LIST(HANDLE, uint8_t *), LIST(hDevice, bSupported),
	"hDevice = %p, bSupported = %p");

FUNC_WRAPPER(
	DtsSetColorSpace, BC_STATUS, LIST(HANDLE hDevice, BC_OUTPUT_FORMAT Mode422),
	LIST(HANDLE, BC_OUTPUT_FORMAT), LIST(hDevice, Mode422),
	"hDevice = %p, Mode422 = %u");

FUNC_WRAPPER(
	DtsSet422Mode, BC_STATUS, LIST(HANDLE hDevice, uint8_t Mode422),
	LIST(HANDLE, uint8_t), LIST(hDevice, Mode422),
	"hDevice = %p, Mode422 = %u");

FUNC_WRAPPER(
	DtsGetDILPath, BC_STATUS, LIST(HANDLE hDevice, char *DilPath, uint32_t size),
	LIST(HANDLE, char *, uint32_t), LIST(hDevice, DilPath, size),
	"hDevice = %p, DilPath = %p, size = %u");

FUNC_WRAPPER(
	DtsDropPictures, BC_STATUS, LIST(HANDLE hDevice, uint32_t Picture),
	LIST(HANDLE, uint32_t), LIST(hDevice, Picture),
	"hDevice = %p, Picture = %u");

FUNC_WRAPPER(
	DtsGetDriverStatus, BC_STATUS, LIST(HANDLE hDevice, BC_DTS_STATUS *pStatus),
	LIST(HANDLE, BC_DTS_STATUS *), LIST(hDevice, pStatus),
	"hDevice = %p, pStatus = %p");

FUNC_WRAPPER(
	DtsGetCapabilities, BC_STATUS, LIST(HANDLE hDevice, PBC_HW_CAPS pCaps),
	LIST(HANDLE, PBC_HW_CAPS), LIST(hDevice, pCaps),
	"hDevice = %p, pCaps = %p");

FUNC_WRAPPER(
	DtsSetScaleParams, BC_STATUS, LIST(HANDLE hDevice, PBC_SCALING_PARAMS pScaleParams),
	LIST(HANDLE, PBC_SCALING_PARAMS), LIST(hDevice, pScaleParams),
	"hDevice = %p, pScaleParams = %p");

FUNC_WRAPPER(
	DtsIsEndOfStream, BC_STATUS, LIST(HANDLE hDevice, uint8_t *bEOS),
	LIST(HANDLE, uint8_t *), LIST(hDevice, bEOS),
	"hDevice = %p, bEOS = %p");

FUNC_WRAPPER(
	DtsCrystalHDVersion, BC_STATUS, LIST(HANDLE hDevice, PBC_INFO_CRYSTAL pCrystalInfo),
	LIST(HANDLE, PBC_INFO_CRYSTAL), LIST(hDevice, pCrystalInfo),
	"hDevice = %p, pCrystalInfo = %p");

FUNC_WRAPPER(
	DtsTxFreeSize, uint32_t, LIST(HANDLE hDevice),
	LIST(HANDLE), LIST(hDevice),
	"hDevice = %p");
#endif
