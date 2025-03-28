// Copyright © 2023 ESWIN. All rights reserved.
//
// Beijing ESWIN Computing Technology Co., Ltd and its affiliated companies ("ESWIN") retain
// all intellectual property and proprietary rights in and to this software. Except as expressly
// authorized by ESWIN, no part of the software may be released, copied, distributed, reproduced,
// modified, adapted, translated, or created derivative work of, in whole or in part.

#ifndef __DSP_KERNEL_TEST_COMMON_H__
#define __DSP_KERNEL_TEST_COMMON_H__

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include "es_dsp_types.h"
#include "es_dsp_op_types.h"
#include "es_dsp_error.h"
#include "es_sys_memory.h"

#define DSP_LOG_DBG(fmt, ...) printf("[DSP][TEST][DEBUG](%s:%d) " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define DSP_LOG_MSG(fmt, ...) printf("[DSP][TEST][INFO](%s:%d) " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define DSP_LOG_WARN(fmt, ...) printf("[DSP][TEST][WARN](%s:%d) " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define DSP_LOG_ERR(fmt, ...) printf("[DSP][TEST][ERROR](%s:%d) " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define DSP_MEM_CACHE_MODE SYS_CACHE_MODE_NOCACHE
#define DSP_MEM_MMB "dsp kernel test"

#if defined DSP_ENV_SIM && DSP_ENV_SIM
#define DSP_MEM_ZONE "reserved"
#else
#define DSP_MEM_ZONE NULL
#endif

typedef struct CMD_ARGS {
    ES_DSP_ID_E dspId;
    ES_S32 inputFileNum;
    ES_CHAR* inputFiles[BUFFER_CNT_MAXSIZE];
    ES_CHAR* operatorDir;
    ES_CHAR* jsonFile;
    ES_CHAR* outputDir;
    ES_DSP_LOG_LEVEL_E logLevel;
} CMD_ARGS_S;

ES_S32 parseCmdArgs(ES_S32 argc, ES_CHAR* argv[], CMD_ARGS_S* args);

ES_S32 loadBin(const ES_CHAR* fileName, const int64_t size, ES_CHAR* outBuf);
ES_S32 writeBin(const ES_CHAR* fileName, const int64_t size, const ES_CHAR* buf);

ES_S32 loadParamsData(const ES_DSP_OPERATOR_DESC_S* desc, ES_DEVICE_BUFFER_GROUP_S* params);
ES_S32 loadInputsData(ES_CHAR** inputFiles, const ES_DSP_OPERATOR_DESC_S* desc, ES_DEVICE_BUFFER_GROUP_S* inputs);
ES_S32 prepareOutputBuffer(const ES_DSP_OPERATOR_DESC_S* desc, ES_DEVICE_BUFFER_GROUP_S* outputs);
ES_S32 dumpDspOutput(ES_DEVICE_BUFFER_GROUP_S* outputs, const ES_CHAR* outputsDir);

void prepareTaskBuffer(ES_DSP_TASK_S* task, ES_DEVICE_BUFFER_GROUP_S* params, ES_DEVICE_BUFFER_GROUP_S* inputs,
                       ES_DEVICE_BUFFER_GROUP_S* outputs);
void freeDeviceBuffers(ES_DEVICE_BUFFER_GROUP_S* params, ES_DEVICE_BUFFER_GROUP_S* inputs,
                       ES_DEVICE_BUFFER_GROUP_S* outputs);

ES_S32 prepareDmaBuffers(ES_S32 dspFd, ES_DEVICE_BUFFER_GROUP_S* devBuf, ES_U32 bufCnt);
ES_S32 unPrepareDmaBuffers(ES_S32 dspFd, ES_DEVICE_BUFFER_GROUP_S* devBuf, ES_U32 bufCnt);

#endif