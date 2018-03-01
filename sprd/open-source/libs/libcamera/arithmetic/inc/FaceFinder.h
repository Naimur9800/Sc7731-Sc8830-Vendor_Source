#ifndef _FACEFINDER_H_
#define _FACEFINDER_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct{
    int face_id;
    int sx;
    int sy;
    int srx;
    int sry;
    int ex;
    int ey;
    int elx;
    int ely;
    int brightness;
    int angle;
    int smile_level;
    int blink_level;
} FaceFinder_Data;

// void *malloc(unsigned int num_bytes);

typedef void* (*MallocFun)(int );
typedef void (*FreeFun)(void *);

int FaceFinder_Init(int width, int height, MallocFun Mfp, FreeFun Ffp);
int FaceFinder_Function(unsigned char *src, FaceFinder_Data** ppDstFaces, int *pDstFaceNum ,int skip);
int FaceFinder_Finalize(FreeFun Ffp);

#ifdef __cplusplus
}
#endif

#endif
