// Copyright © 2024 ESWIN. All rights reserved.
//
// Beijing ESWIN Computing Technology Co., Ltd and its affiliated companies ("ESWIN") retain
// all intellectual property and proprietary rights in and to this software. Except as expressly
// authorized by ESWIN, no part of the software may be released, copied, distributed, reproduced,
// modified, adapted, translated, or created derivative work of, in whole or in part.

#include "common.h"
#include "es_dsp_ll_api.h"

#define SIZE_2M 0x200000
#define ALIGN(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))

ES_S32 parseCmdArgs(ES_S32 argc, ES_CHAR* argv[], CMD_ARGS_S* args)
{
    ES_CHAR* value = NULL;
    ES_CHAR ch;

    args->logLevel = ES_DSP_LOG_ERROR;

    for (;;) {
        if (optind >= argc) break;

        ch = getopt(argc, argv, "c:i:p:o:l:");
        switch (ch) {
            case 'c':
                args->dspId = (ES_DSP_ID_E)atoi(optarg);
                break;
            case 'o':
                args->outputDir = optarg;
                DSP_LOG_MSG("outputDir=%s\n", args->outputDir);
                break;
            case 'p':
                args->operatorDir = optarg;
                DSP_LOG_MSG("operatorDir=%s\n", args->operatorDir);
                break;
            case 'i':
                args->inputFileNum = 0;
                value = optarg;
                while (value && value[0] != '-') {
                    args->inputFiles[args->inputFileNum++] = value;
                    value = argv[optind++];
                }
                optind--;
                break;
            case 'l':
                args->logLevel = (ES_DSP_LOG_LEVEL_E)atoi(optarg);
                DSP_LOG_MSG("log level=%d\n", args->logLevel);
                break;
            default:
                DSP_LOG_ERR("get invalid param, ch=%c.\n", ch);
                DSP_LOG_ERR("param = %s.\n", optarg);
                return -1;
                break;
        }
    }

    if (args->dspId < 0 || !args->inputFiles[0]) {
        DSP_LOG_ERR("get invalid params.\n");
        return -1;
    }

    ES_CHAR* op_dir = NULL;
    if (args->operatorDir != NULL) {
        op_dir = realpath(args->operatorDir, NULL);
        if (!op_dir) {
            DSP_LOG_ERR("operator path %s cannot transfer to absolute, error.\n", args->operatorDir);
            return -1;
        }
    }

    if (args->outputDir) {
        op_dir = realpath(args->outputDir, NULL);
        if (!op_dir) {
            DSP_LOG_ERR("outputDir %s cannot transfer to absolute, error.\n", args->outputDir);
            return -1;
        }
    }
    DSP_LOG_MSG("set output dir as %s.\n", args->outputDir);

    ES_S32 i;
    for (i = 0; i < args->inputFileNum; i++) {
        if (!args->inputFiles[i]) {
            DSP_LOG_ERR("inputFiles[%d] is null.\n", i);
            return -1;
        }
    }

    return 0;
}

ES_S32 loadBin(const ES_CHAR* fileName, const int64_t size, ES_CHAR* outBuf)
{
    FILE* file_p = NULL;

    if (!fileName) {
        DSP_LOG_ERR("invalid file.\n");
        return ES_DSP_ERROR_OPEN_FILE_FAILED;
    }

    file_p = fopen(fileName, "r");
    if (file_p == NULL) {
        DSP_LOG_ERR("open file=%s failed.\n", fileName);
        return ES_DSP_ERROR_OPEN_FILE_FAILED;
    }

    size_t bytes_read = fread(outBuf, sizeof(ES_CHAR), size, file_p);
    if (bytes_read != size) {
        DSP_LOG_ERR("file=%s read failed.\n", fileName);
        fclose(file_p);
        return ES_DSP_ERROR_READ_FILE_FAILED;
    }
    fclose(file_p);

    DSP_LOG_MSG("%s load to buf:%p size=0x%lx\n", fileName, outBuf, size);

    return ES_DSP_SUCCESS;
}

ES_S32 writeBin(const ES_CHAR* fileName, const int64_t size, const ES_CHAR* buf)
{
    FILE* file_p = NULL;

    if (!fileName) {
        DSP_LOG_ERR("invalid file.\n");
        return ES_DSP_ERROR_OPEN_FILE_FAILED;
    }

    file_p = fopen(fileName, "w");
    if (file_p == NULL) {
        DSP_LOG_ERR("file=%s dump failed.\n", fileName);
        return ES_DSP_ERROR_OPEN_FILE_FAILED;
    }

    size_t bytes_written = fwrite((void*)buf, 1, size, file_p);
    if (bytes_written != size) {
        DSP_LOG_ERR("file=%s write failed.\n", fileName);
        fclose(file_p);
        return ES_DSP_ERROR_WRITE_FILE_FAILED;
    }
    fclose(file_p);

    DSP_LOG_MSG("dump to file=%s buf:%p size=0x%lx\n", fileName, buf, size);

    return ES_DSP_SUCCESS;
}

ES_S32 loadParamsData(const ES_DSP_OPERATOR_DESC_S* desc, ES_DEVICE_BUFFER_GROUP_S* params)
{
    if (desc->bufferCntCfg >= BUFFER_CNT_MAXSIZE) {
        DSP_LOG_ERR("bufferCntCfg out of max buf count.\n");
        return ES_DSP_ERROR_OUTBUF;
    }

    ES_S32 ret;
    ES_S32 i;
    params->bufferCnt = desc->bufferCntCfg;
    params->buffers = malloc(sizeof(ES_DEV_BUF_S) * params->bufferCnt);
    ES_VOID* paramBuf = NULL;
    ES_CHAR* paramAddr = (ES_CHAR*)&desc->paramData[0];
    ES_U32 bufSize;
    ES_U32 allocSize;
    for (i = 0; i < desc->bufferCntCfg; i++) {
        params->buffers[i].size = desc->bufferSize[i];
        params->buffers[i].offset = 0;
        bufSize = params->buffers[i].size;
        allocSize = ALIGN(bufSize, SIZE_2M);
        ret = ES_SYS_MemAlloc(&params->buffers[i].memFd, DSP_MEM_CACHE_MODE, DSP_MEM_MMB, DSP_MEM_ZONE, allocSize);
        if (ret != ES_DSP_SUCCESS) {
            DSP_LOG_ERR("ES_SYS_MemAlloc failed, ret: 0x%x!\n", ret);
            return ret;
        }

        paramBuf = ES_SYS_Mmap(params->buffers[i].memFd, allocSize, DSP_MEM_CACHE_MODE);
        if (!paramBuf) {
            DSP_LOG_ERR("ES_SYS_Mmap failed, ret: 0x%x!\n", ret);
            ES_SYS_MemFree(params->buffers[i].memFd);
            return ret;
        }
        memcpy(paramBuf, paramAddr, bufSize);

        ret = ES_SYS_Munmap(paramBuf, allocSize);
        if (ES_DSP_SUCCESS != ret) {
            DSP_LOG_ERR("ES_SYS_Munmap of params failed, ret: 0x%x!\n", ret);
            ES_SYS_MemFree(params->buffers[i].memFd);
            return ret;
        }
        paramAddr += bufSize;

        DSP_LOG_MSG("params->buffers[%d].memFd:0x%llx buffers.offset=0x%llx buffers.size=0x%x\n", i,
                    params->buffers[i].memFd, params->buffers[i].offset, bufSize);
    }

    return ES_DSP_SUCCESS;
}

ES_S32 loadInputsData(ES_CHAR** inputFiles, const ES_DSP_OPERATOR_DESC_S* desc, ES_DEVICE_BUFFER_GROUP_S* inputs)
{
    if (desc->bufferCntCfg >= BUFFER_CNT_MAXSIZE || desc->bufferCntInput >= BUFFER_CNT_MAXSIZE) {
        DSP_LOG_ERR("ES_DSP_OPERATOR_DESC_S out of max buf count.\n");
        return ES_DSP_ERROR_OUTBUF;
    }

    ES_S32 ret;
    ES_S32 i;
    ES_VOID* inputBuf = NULL;
    ES_U32 bufSize;
    ES_U32 allocSize;
    inputs->bufferCnt = desc->bufferCntInput;
    inputs->buffers = malloc(sizeof(ES_DEV_BUF_S) * inputs->bufferCnt);
    for (i = 0; i < desc->bufferCntInput; i++) {
        inputs->buffers[i].size = desc->bufferSize[i + desc->bufferCntCfg];
        inputs->buffers[i].offset = 0;
        bufSize = inputs->buffers[i].size;

        allocSize = ALIGN(bufSize, SIZE_2M);
        ret = ES_SYS_MemAlloc(&inputs->buffers[i].memFd, DSP_MEM_CACHE_MODE, DSP_MEM_MMB, DSP_MEM_ZONE, allocSize);
        if (ret != ES_DSP_SUCCESS) {
            DSP_LOG_ERR("ES_SYS_MemAlloc inputs failed, ret: 0x%x!\n", ret);
            return ret;
        }

        inputBuf = ES_SYS_Mmap(inputs->buffers[i].memFd, allocSize, DSP_MEM_CACHE_MODE);
        if (!inputBuf) {
            DSP_LOG_ERR("ES_SYS_Mmap failed, ret: 0x%x!\n", ret);
            ES_SYS_MemFree(inputs->buffers[i].memFd);
            return ret;
        }

        ret = loadBin(inputFiles[i], desc->bufferSize[desc->bufferCntCfg + i], inputBuf);
        if (ret != ES_DSP_SUCCESS) {
            DSP_LOG_ERR("load tensor from file:%s to addr:%p failed.\n", inputFiles[i], inputBuf);
            ES_SYS_MemFree(inputs->buffers[i].memFd);
            ES_SYS_Munmap(inputBuf, allocSize);
            return ret;
        }
        DSP_LOG_MSG("load tensor from file:%s to addr:%p\n", inputFiles[i], inputBuf);

        ret = ES_SYS_Munmap(inputBuf, allocSize);
        if (ES_DSP_SUCCESS != ret) {
            DSP_LOG_ERR("ES_SYS_Munmap of inputs failed, ret: 0x%x!\n", ret);
            ES_SYS_MemFree(inputs->buffers[i].memFd);
            return ret;
        }
    }

    return ES_DSP_SUCCESS;
}

ES_S32 prepareOutputBuffer(const ES_DSP_OPERATOR_DESC_S* desc, ES_DEVICE_BUFFER_GROUP_S* outputs)
{
    if (desc->bufferCntOutput >= BUFFER_CNT_MAXSIZE) {
        DSP_LOG_ERR("ES_DSP_OPERATOR_DESC_S out of max buf count.\n");
        return ES_DSP_ERROR_OUTBUF;
    }

    ES_S32 ret;
    ES_S32 i;
    ES_VOID* inputBuf = NULL;
    ES_U32 bufSize;
    ES_U32 allocSize;
    outputs->bufferCnt = desc->bufferCntOutput;
    outputs->buffers = malloc(sizeof(ES_DEV_BUF_S) * outputs->bufferCnt);
    for (i = 0; i < desc->bufferCntOutput; i++) {
        outputs->buffers[i].size = desc->bufferSize[i + desc->bufferCntCfg + desc->bufferCntInput];
        outputs->buffers[i].offset = 0;
        bufSize = outputs->buffers[i].size;

        allocSize = ALIGN(bufSize, SIZE_2M);
        ret = ES_SYS_MemAlloc(&outputs->buffers[i].memFd, DSP_MEM_CACHE_MODE, DSP_MEM_MMB, DSP_MEM_ZONE, allocSize);
        if (ret != ES_DSP_SUCCESS) {
            DSP_LOG_ERR("ES_SYS_MemAlloc outputs failed, ret: 0x%x!\n", ret);
            return ret;
        }

        DSP_LOG_MSG("outputs->buffers[%d].memFd:0x%llx offset=0x%llx size=0x%llx\n", i, outputs->buffers[i].memFd,
                    outputs->buffers[i].offset, outputs->buffers[i].size);
    }

    return ES_DSP_SUCCESS;
}

void prepareTaskBuffer(ES_DSP_TASK_S* task, ES_DEVICE_BUFFER_GROUP_S* params, ES_DEVICE_BUFFER_GROUP_S* inputs,
                       ES_DEVICE_BUFFER_GROUP_S* outputs)
{
    task->bufferCntCfg = params->bufferCnt;
    task->bufferCntInput = inputs->bufferCnt;
    task->bufferCntOutput = outputs->bufferCnt;

    ES_S32 i;
    ES_S32 offset = 0;
    for (i = 0; i < params->bufferCnt; i++) {
        task->dspBuffers[offset] = params->buffers[i];
        offset++;
    }

    for (i = 0; i < inputs->bufferCnt; i++) {
        task->dspBuffers[offset] = inputs->buffers[i];
        offset++;
    }

    for (i = 0; i < outputs->bufferCnt; i++) {
        task->dspBuffers[offset] = outputs->buffers[i];
        offset++;
    }

    DSP_LOG_MSG("prepareTaskBuffer cfg_cnt=%d input_cnt=%d output_cnt=%d\n", task->bufferCntCfg, task->bufferCntInput,
                task->bufferCntOutput);
}

void freeDeviceBuffers(ES_DEVICE_BUFFER_GROUP_S* params, ES_DEVICE_BUFFER_GROUP_S* inputs,
                       ES_DEVICE_BUFFER_GROUP_S* outputs)
{
    ES_S32 i;
    for (i = 0; i < params->bufferCnt; ++i) {
        ES_SYS_MemFree(params->buffers[i].memFd);
    }
    free(params->buffers);

    for (i = 0; i < inputs->bufferCnt; ++i) {
        ES_SYS_MemFree(inputs->buffers[i].memFd);
    }
    free(inputs->buffers);

    for (i = 0; i < outputs->bufferCnt; ++i) {
        ES_SYS_MemFree(outputs->buffers[i].memFd);
    }
    free(outputs->buffers);
}

ES_S32 dumpDspOutput(ES_DEVICE_BUFFER_GROUP_S* outputs, const ES_CHAR* outputsDir)
{
    ES_S32 ret;
    ES_CHAR dump_file[256];
    ES_VOID* outputBuf = NULL;
    ES_U32 allocSize;
    DSP_LOG_MSG("start dump %d tensors to local files:\n", outputs->bufferCnt);
    for (ES_S32 i = 0; i < outputs->bufferCnt; ++i) {
        memset(dump_file, 0, sizeof(dump_file));
        snprintf(dump_file, sizeof(dump_file), "%s/output_%d.bin", outputsDir, i);
        allocSize = ALIGN(outputs->buffers[i].size, SIZE_2M);

        outputBuf = ES_SYS_Mmap(outputs->buffers[i].memFd, allocSize, DSP_MEM_CACHE_MODE);
        if (!outputBuf) {
            DSP_LOG_ERR("ES_SYS_Mmap failed, ret: 0x%x!\n", ret);
            return ret;
        }

        DSP_LOG_MSG("size=0x%llx, addr=%p.\n", outputs->buffers[i].size, outputBuf);
        ret = writeBin(dump_file, outputs->buffers[i].size, outputBuf);
        if (ret != ES_DSP_SUCCESS) {
            DSP_LOG_ERR("write data to file:%s failed.\n", dump_file);
            return ret;
        }
        ret = ES_SYS_Munmap(outputBuf, allocSize);
        if (ES_DSP_SUCCESS != ret) {
            DSP_LOG_ERR("ES_SYS_Munmap of outputs failed, ret: 0x%x!\n", ret);
            return ret;
        }

        DSP_LOG_MSG("dump tensor(%d) to file:%s.\n", i, dump_file);
    }

    return ES_DSP_SUCCESS;
}

ES_S32 prepareDmaBuffers(ES_S32 dspFd, ES_DEVICE_BUFFER_GROUP_S* devBuf, ES_U32 bufCnt)
{
    ES_S32 ret;
    for (ES_U32 i = 0; i < bufCnt; i++) {
        ret = ES_DSP_LL_PrepareDMABuffer(dspFd, devBuf->buffers[i]);
        if (ret != ES_DSP_SUCCESS) {
            DSP_LOG_ERR("prepare dma buffer failed, ret=0x%x\n", ret);
            return ret;
        }
    }

    return ES_DSP_SUCCESS;
}

ES_S32 unPrepareDmaBuffers(ES_S32 dspFd, ES_DEVICE_BUFFER_GROUP_S* devBuf, ES_U32 bufCnt)
{
    ES_S32 ret;
    for (ES_U32 i = 0; i < bufCnt; i++) {
        ret = ES_DSP_LL_UnprepareDMABuffer(dspFd, devBuf->buffers[i].memFd);
        if (ret != ES_DSP_SUCCESS) {
            DSP_LOG_ERR("unprepare dma buffer failed, ret=0x%x\n", ret);
            return ret;
        }
    }

    return ES_DSP_SUCCESS;
}
