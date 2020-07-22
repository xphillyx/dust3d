#ifndef DUST3D_H
#define DUST3D_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
# define DUST3D_API                     __stdcall
# ifdef DUST3D_EXPORTING
#  define DUST3D_DLL                    __declspec(dllexport)
# else
#  define DUST3D_DLL                    __declspec(dllimport)
# endif
#else
# define DUST3D_API                     
# define DUST3D_DLL
#endif

#define DUST3D_OK                   0
#define DUST3D_ERROR                1
#define DUST3D_UNSUPPORTED          2

typedef struct _dust3d dust3d;

DUST3D_DLL void         DUST3D_API dust3dInitialize(void);
DUST3D_DLL dust3d *     DUST3D_API dust3dOpenFromMemory(const char *documentType, const char *buffer, int size);
DUST3D_DLL dust3d *     DUST3D_API dust3dOpen(const char *fileName);
DUST3D_DLL void         DUST3D_API dust3dSetUserData(dust3d *ds3, void *userData);
DUST3D_DLL void *       DUST3D_API dust3dGetUserData(dust3d *ds3);
DUST3D_DLL int          DUST3D_API dust3dGenerateMesh(dust3d *ds3);
DUST3D_DLL const char * DUST3D_API dust3dGetMeshAsObj(dust3d *ds3);
DUST3D_DLL void         DUST3D_API dust3dClose(dust3d *ds3);
DUST3D_DLL int          DUST3D_API dust3dError(dust3d *ds3);
DUST3D_DLL const char * DUST3D_API dust3dVersion(void);

#ifdef __cplusplus
}
#endif

#endif
