/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

/**************************************************************************
 *					                                  *
 *                           (C) 2001 			                  *
 *                   VirCinity IT-Consulting GmbH                          *
 *                         Nobelstrasse 15				  *
 *                       D-70569 Stuttgart				  *
 *                            Germany					  *
 * Author: S. Kufer							  *
 * Date: 5. Juli 2001							  *
 **************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#include "covFiles.h"
#include "covWriteFiles.h"
#include "covReadFiles.h"

#define COV_READ(fd, data, size) read(abs(fd), (data), (size))
#define COV_READ_INT(fd, data, size) read(abs(fd), (data), ((size) * sizeof(int)))
#define COV_READ_FLOAT(fd, data, size) read(abs(fd), (data), (size) * sizeof(float))
#define COV_WRITE(fd, data, size) write(abs(fd), (data), (size))
#define CHECK_FOR_ERRORS 1
#define FAILED -1

#define SWAP_INT(fd, data, size) \
    if ((fd) < 0)                \
    swap_int((data), (size))
#define SWAP_FLOAT(fd, data, size) \
    if ((fd) < 0)                  \
    swap_float((data), (size))

#define WRITE_COVISE 0
#define READ_COVISE 1
#define READ_COVISE_OLD 3
#define READ_SIZE 2
#define READ_SIZE_IMAGE 4
#define READ_DIM 54

/* extern definitions for type constants */
int TYPE_HEXAGON = 7;
int TYPE_HEXAEDER = 7;
int TYPE_PRISM = 6;
int TYPE_PYRAMID = 5;
int TYPE_TETRAHEDER = 4;
int TYPE_QUAD = 3;
int TYPE_TRIANGLE = 2;
int TYPE_BAR = 1;
int TYPE_NONE = 0;
int TYPE_POINT = 10;

static void swap_int(int *d, int num)
{
    unsigned int *data = (unsigned int *)d;
    int i;

    for (i = 0; i < num; i++)
    {

        *data = (((*data) & 0xff000000) >> 24)
                | (((*data) & 0x00ff0000) >> 8)
                | (((*data) & 0x0000ff00) << 8)
                | (((*data) & 0x000000ff) << 24);

        data++;
    }
}

static void swap_float(float *d, int num)
{
    unsigned int *data = (unsigned int *)d;
    int i;
    for (i = 0; i < num; i++)
    {
        *data = (((*data) & 0xff000000) >> 24)
                | (((*data) & 0x00ff0000) >> 8)
                | (((*data) & 0x0000ff00) << 8)
                | (((*data) & 0x000000ff) << 24);
        data++;
    }
}

int covOpenOutFile(char *filename)
{
#ifdef _WIN32
    long my_umask, filemode;
    int fd;
#else
    mode_t my_umask, filemode;
    int fd;
    /* check umask but leave it on old value! */
    /* file modes are all except my umask */
    my_umask = umask(0777);
    umask(my_umask);

    filemode = 0777 & (~my_umask);
#endif

#ifdef _WIN32
    fd = _open(filename, _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC);
#else
    fd = open(filename, O_WRONLY | O_CREAT, filemode | O_TRUNC);
#endif
    if (fd == FAILED)
        return 0;
#ifdef BYTESWAP
    COV_WRITE(fd, "COV_BE", 6);
#else
    COV_WRITE(fd, "COV_LE", 6);
#endif
    return fd;
}

int covOpenInFile(char *filename)
{
    char magic[7];
#ifdef _WIN32
    int fd = _open(filename, _O_RDONLY | _O_BINARY);
#else
    int fd = open(filename, O_RDONLY);
#endif
    if (fd == FAILED)
        return 0;
    COV_READ(fd, magic, 6);
    magic[6] = 0;

#ifdef BYTESWAP
    fd *= -1;
#endif

    if (strcmp(magic, "COV_BE") == 0)
        fd *= -1;
    else if (strcmp(magic, "COV_LE") == 0)
        ;
    else /* old-style, no header */
    {
        lseek(abs(fd), (off_t)0, SEEK_SET);
    }
    return fd;
}

int covCloseOutFile(int fd)
{
    return (close(abs(fd)) != EOF);
}

int covCloseInFile(int fd)
{
    return (close(abs(fd)) != EOF);
}

int covIoAttrib(int fd, int mode, int *num, int *size, char **atNam, char **atVal)
{
    /* count # attribs given and total space requirement */
    int numAttr = 0, i;
    char **nPtr = atNam, **vPtr = atVal;
    if (mode == WRITE_COVISE)
    {
        *size = sizeof(int);
        if (*num == COUNT_ATTR)
        {
            while (nPtr && *nPtr)
            {
                *size += strlen(*nPtr) + strlen(*vPtr) + 2;
                nPtr++;
                vPtr++;
                numAttr++;
            }
        }
        else
        {
            numAttr = *num;
            for (i = 0; i < numAttr; i++)
            {
                *size += strlen(*nPtr) + strlen(*vPtr) + 2;
                nPtr++;
                vPtr++;
            }
        }

        COV_WRITE(fd, size, sizeof(int));
        COV_WRITE(fd, &numAttr, sizeof(int));

        for (i = 0; i < numAttr; i++)
        {
            COV_WRITE(fd, (char *)atNam[i], strlen(atNam[i]) + 1);
            COV_WRITE(fd, (char *)atVal[i], strlen(atVal[i]) + 1);
        }
    }
    else
    {
        char *buf;
        char *an, *at;
        int numattrib = *num;

        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, size, 1);
            SWAP_INT(fd, size, 1);
            *size -= sizeof(int);
            COV_READ_INT(fd, &numattrib, 1);
            SWAP_INT(fd, &numattrib, 1);
            *num = numattrib;
            return CHECK_FOR_ERRORS;
        }

        else if (*size > 0)
        {
            /*         buf=malloc(*size);     */
            buf = (char *)malloc(*size);
            COV_READ(fd, buf, *size);
            an = buf;
            for (i = 0; i < numattrib; i++)
            {
                at = an;
                while (*at)
                    at++;
                at++;
                strcpy(atNam[i], an);
                strcpy(atVal[i], at);
                an = at;
                while (*an)
                    an++;
                an++;
            }
            free(buf);
        }
    }
    return CHECK_FOR_ERRORS;
}

int covWriteAttrib(int fd, int num, char **atNam, char **atVal)
{
    int dummy;
    return covIoAttrib(fd, WRITE_COVISE, &num, &dummy, atNam, atVal);
}

int covReadAttributes(int fd, char **atNam, char **atVal, int num, int size)
{
    return covIoAttrib(fd, READ_COVISE, &num, &size, atNam, atVal);
}

int covReadNumAttributes(int fd, int *num, int *size)
{
    int ret = covIoAttrib(fd, READ_SIZE, num, size, NULL, NULL);
    return ret;
}

int covReadDescription(int fd, char *name)
{
    do
    {
        COV_READ(fd, name, 1);
    } while (name[0] == '\0');
    COV_READ(fd, name + 1, 5);
    return CHECK_FOR_ERRORS;
}

/*  *********************************
                  GEOMET
    *********************************/

int covIoGeometryBegin(int fd, int mode, int *has_geometry, int *has_colors, int *has_normals, int *has_texture)
{
    int isfalse = 0;
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "GEOTEX", 6);
    if (mode != WRITE_COVISE)
    {
        /* has_geometry==true */
        COV_READ_INT(fd, has_geometry, 1);
        SWAP_INT(fd, has_geometry, 1);
        COV_READ_INT(fd, has_colors, 1);
        SWAP_INT(fd, has_colors, 1);
        COV_READ_INT(fd, has_normals, 1);
        SWAP_INT(fd, has_normals, 1);
        if (mode != READ_COVISE_OLD)
            COV_READ_INT(fd, has_texture, 1);
        SWAP_INT(fd, has_texture, 1);
        /* int types are ignored */
        COV_READ_INT(fd, &isfalse, 1);
        SWAP_INT(fd, &isfalse, 1);
        COV_READ_INT(fd, &isfalse, 1);
        SWAP_INT(fd, &isfalse, 1);
        COV_READ_INT(fd, &isfalse, 1);
        SWAP_INT(fd, &isfalse, 1);
        if (mode != READ_COVISE_OLD)
            COV_READ_INT(fd, &isfalse, 1);
        SWAP_INT(fd, &isfalse, 1);
    }
    else
    {
        COV_WRITE(fd, has_geometry, sizeof(int)); /* has_geometry==true */
        COV_WRITE(fd, has_colors, sizeof(int));
        COV_WRITE(fd, has_normals, sizeof(int));
        COV_WRITE(fd, has_texture, sizeof(int));
        COV_WRITE(fd, &isfalse, sizeof(int)); /* int types are ignored */
        COV_WRITE(fd, &isfalse, sizeof(int));
        COV_WRITE(fd, &isfalse, sizeof(int));
        COV_WRITE(fd, &isfalse, sizeof(int));
    }
    return CHECK_FOR_ERRORS;
}

int covWriteGeometryBegin(int fd, int has_colors, int has_normals, int has_texture)
{
    int istrue, isfalse;
    isfalse = 0, istrue = !isfalse;
    return covIoGeometryBegin(fd, WRITE_COVISE, &istrue, &has_colors, &has_normals, &has_texture);
}

int covReadGeometryBegin(int fd, int *has_geometry, int *has_colors, int *has_normals, int *has_texture)
{
    return covIoGeometryBegin(fd, READ_COVISE, has_geometry, has_colors, has_normals, has_texture);
}

int covReadOldGeometryBegin(int fd, int *has_geometry, int *has_colors, int *has_normals)
{
    int dummy;
    return covIoGeometryBegin(fd, READ_COVISE_OLD, has_geometry, has_colors, has_normals, &dummy);
}

int covWriteGeometryEnd(int fd, char **atNam, char **atVal, int numAttr)
{
    return covWriteAttrib(fd, numAttr, atNam, atVal);
}

/*
int covReadGeometryEnd( char **atNam,char **atVal, int numAttr, int size)
{
return covReadAttributes( READ_COVISE, atNam, atVal, &numAttr, &size );
}*/

/*  *********************************
                  SETELE
    *********************************/

int covIoSetBegin(int fd, int mode, int *numElem)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "SETELE", 6);
    if (mode != WRITE_COVISE)
        COV_READ_INT(fd, numElem, 1);
    SWAP_INT(fd, numElem, 1);
    else COV_WRITE(fd, numElem, sizeof(int));
    return CHECK_FOR_ERRORS;
}

int covWriteSetBegin(int fd, int numElem)
{
    return covIoSetBegin(fd, WRITE_COVISE, &numElem);
}

int covReadSetBegin(int fd, int *numElem)
{
    return covIoSetBegin(fd, READ_COVISE, numElem);
}

int covWriteSetEnd(int fd, char **atNam, char **atVal, int numAttr)
{
    return covWriteAttrib(fd, numAttr, atNam, atVal);
}

/*
int covReadSetEnd( char **atNam,char **atVal, int numAttr)
{
return 1;
}*/

/*  *********************************
                  UNSGRD
    *********************************/

int covIoUNSGRD(int fd, int mode, int *numElem, int *numConn, int *numVert,
                int *el, int *cl, int *tl,
                float *x, float *y, float *z,
                char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "UNSGRD", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, numElem, 1);
            SWAP_INT(fd, numElem, 1);
            COV_READ_INT(fd, numConn, 1);
            SWAP_INT(fd, numConn, 1);
            COV_READ_INT(fd, numVert, 1);
            SWAP_INT(fd, numVert, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_INT(fd, el, *numElem);
        SWAP_INT(fd, el, *numElem);
        COV_READ_INT(fd, tl, *numElem);
        SWAP_INT(fd, tl, *numElem);
        COV_READ_INT(fd, cl, *numConn);
        SWAP_INT(fd, cl, *numConn);
        COV_READ_FLOAT(fd, x, *numVert);
        SWAP_FLOAT(fd, x, *numVert);
        COV_READ_FLOAT(fd, y, *numVert);
        SWAP_FLOAT(fd, y, *numVert);
        COV_READ_FLOAT(fd, z, *numVert);
        SWAP_FLOAT(fd, z, *numVert);
    }
    else
    {
        COV_WRITE(fd, numElem, sizeof(int));
        COV_WRITE(fd, numConn, sizeof(int));
        COV_WRITE(fd, numVert, sizeof(int));
        COV_WRITE(fd, el, *numElem * sizeof(int));
        COV_WRITE(fd, tl, *numElem * sizeof(int));
        COV_WRITE(fd, cl, *numConn * sizeof(int));
        COV_WRITE(fd, x, *numVert * sizeof(float));
        COV_WRITE(fd, y, *numVert * sizeof(float));
        COV_WRITE(fd, z, *numVert * sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteUNSGRD(int fd, int numElem, int numConn, int numVert,
                   int *el, int *cl, int *tl,
                   float *x, float *y, float *z,
                   char **atNam, char **atVal, int numAttr)
{
    return covIoUNSGRD(fd, WRITE_COVISE, &numElem, &numConn, &numVert, el, cl, tl,
                       x, y, z, atNam, atVal, &numAttr);
}

int covReadSizeUNSGRD(int fd, int *numElem, int *numConn, int *numVert)
{
    return covIoUNSGRD(fd, READ_SIZE, numElem, numConn, numVert, NULL, NULL, NULL,
                       NULL, NULL, NULL, NULL, NULL, NULL);
}

int covReadUNSGRD(int fd, int numElem, int numConn, int numVert,
                  int *el, int *cl, int *tl,
                  float *x, float *y, float *z)
{
    return covIoUNSGRD(fd, READ_COVISE, &numElem, &numConn, &numVert, el, cl, tl,
                       x, y, z, NULL, NULL, NULL);
}

/*  *********************************
                  POINTS
    *********************************/

int covIoPOINTS(int fd, int mode, int *numElem, float *x, float *y, float *z, char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "POINTS", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, numElem, 1);
            SWAP_INT(fd, numElem, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_FLOAT(fd, x, *numElem);
        SWAP_FLOAT(fd, x, *numElem);
        COV_READ_FLOAT(fd, y, *numElem);
        SWAP_FLOAT(fd, y, *numElem);
        COV_READ_FLOAT(fd, z, *numElem);
        SWAP_FLOAT(fd, z, *numElem);
    }
    else
    {
        COV_WRITE(fd, numElem, sizeof(int));
        COV_WRITE(fd, x, *numElem * sizeof(float));
        COV_WRITE(fd, y, *numElem * sizeof(float));
        COV_WRITE(fd, z, *numElem * sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWritePOINTS(int fd, int numElem, float *x, float *y, float *z, char **atNam, char **atVal, int numAttr)
{
    return covIoPOINTS(fd, WRITE_COVISE, &numElem, x, y, z, atNam, atVal, &numAttr);
}

int covReadSizePOINTS(int fd, int *numElem)
{
    return covIoPOINTS(fd, READ_SIZE, numElem, NULL, NULL, NULL, NULL, NULL, NULL);
}

int covReadPOINTS(int fd, int numElem, float *x, float *y, float *z)
{
    return covIoPOINTS(fd, READ_COVISE, &numElem, x, y, z, NULL, NULL, NULL);
}

/*  *********************************
                  DOTEXT
    *********************************/

int covIoDOTEXT(int fd, int mode, int *numElem, char *data, char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "DOTEXT", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, numElem, 1);
            SWAP_INT(fd, numElem, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ(fd, data, *numElem);
    }
    else
    {
        COV_WRITE(fd, numElem, sizeof(int));
        COV_WRITE(fd, data, *numElem);
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteDOTEXT(int fd, int numElem, char *data, char **atNam, char **atVal, int numAttr)
{
    return covIoDOTEXT(fd, WRITE_COVISE, &numElem, data, atNam, atVal, &numAttr);
}

int covReadSizeDOTEXT(int fd, int *numElem)
{
    return covIoDOTEXT(fd, READ_SIZE, numElem, NULL, NULL, NULL, NULL);
}

int covReadDOTEXT(int fd, int numElem, char *data)
{
    return covIoDOTEXT(fd, READ_COVISE, &numElem, data, NULL, NULL, NULL);
}

/*  ***********************************************************
                  common function for all line types
    ***********************************************************/

int covIoMetaLines(int fd, int mode, char *type, int *numObjects, int *objectList, int *numCorners, int *cornerList,
                   int *numPoints, float *x, float *y, float *z, char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, type, 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, numObjects, 1);
            SWAP_INT(fd, numObjects, 1);
            COV_READ_INT(fd, numCorners, 1);
            SWAP_INT(fd, numCorners, 1);
            COV_READ_INT(fd, numPoints, 1);
            SWAP_INT(fd, numPoints, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_INT(fd, objectList, *numObjects);
        SWAP_INT(fd, objectList, *numObjects);
        COV_READ_INT(fd, cornerList, *numCorners);
        SWAP_INT(fd, cornerList, *numCorners);
        COV_READ_FLOAT(fd, x, *numPoints);
        SWAP_FLOAT(fd, x, *numPoints);
        COV_READ_FLOAT(fd, y, *numPoints);
        SWAP_FLOAT(fd, y, *numPoints);
        COV_READ_FLOAT(fd, z, *numPoints);
        SWAP_FLOAT(fd, z, *numPoints);
    }
    else
    {
        COV_WRITE(fd, numObjects, sizeof(int));
        COV_WRITE(fd, numCorners, sizeof(int));
        COV_WRITE(fd, numPoints, sizeof(int));
        COV_WRITE(fd, objectList, *numObjects * sizeof(int));
        COV_WRITE(fd, cornerList, *numCorners * sizeof(int));
        COV_WRITE(fd, x, *numPoints * sizeof(float));
        COV_WRITE(fd, y, *numPoints * sizeof(float));
        COV_WRITE(fd, z, *numPoints * sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

/*  *********************************
                  POLYGN
    *********************************/

int covWritePOLYGN(int fd, int numPolygons, int *polyList, int numCorners, int *cornerList,
                   int numPoints, float *x, float *y, float *z, char **atNam, char **atVal, int numAttr)
{
    return covIoMetaLines(fd, WRITE_COVISE, "POLYGN", &numPolygons, polyList, &numCorners, cornerList,
                          &numPoints, x, y, z, atNam, atVal, &numAttr);
}

int covReadSizePOLYGN(int fd, int *numPolygons, int *numCorners, int *numPoints)
{
    return covIoMetaLines(fd, READ_SIZE, "POLYGN", numPolygons, NULL, numCorners, NULL,
                          numPoints, NULL, NULL, NULL, NULL, NULL, NULL);
}

int covReadPOLYGN(int fd, int numPolygons, int *polyList, int numCorners, int *cornerList, int numPoints, float *x, float *y, float *z)
{
    return covIoMetaLines(fd, READ_COVISE, "POLYGN", &numPolygons, polyList, &numCorners, cornerList,
                          &numPoints, x, y, z, NULL, NULL, NULL);
}

/*  *********************************
                  LINES
    *********************************/

int covWriteLINES(int fd, int numLines, int *lineList, int numCorners, int *cornerList,
                  int numPoints, float *x, float *y, float *z, char **atNam, char **atVal, int numAttr)
{
    return covIoMetaLines(fd, WRITE_COVISE, "LINES", &numLines, lineList, &numCorners, cornerList,
                          &numPoints, x, y, z, atNam, atVal, &numAttr);
}

int covReadSizeLINES(int fd, int *numLines, int *numCorners, int *numPoints)
{
    return covIoMetaLines(fd, READ_SIZE, "LINES", numLines, NULL, numCorners, NULL,
                          numPoints, NULL, NULL, NULL, NULL, NULL, NULL);
}

int covReadLINES(int fd, int numLines, int *lineList, int numCorners, int *cornerList,
                 int numPoints, float *x, float *y, float *z)
{
    return covIoMetaLines(fd, READ_COVISE, "LINES", &numLines, lineList, &numCorners, cornerList,
                          &numPoints, x, y, z, NULL, NULL, NULL);
}

/*  *********************************
                  TRIANG
    *********************************/

int covWriteTRIANG(int fd, int numStrips, int *stripList, int numCorners, int *cornerList,
                   int numPoints, float *x, float *y, float *z, char **atNam, char **atVal, int numAttr)
{
    return covIoMetaLines(fd, WRITE_COVISE, "TRIANG", &numStrips, stripList, &numCorners, cornerList,
                          &numPoints, x, y, z, atNam, atVal, &numAttr);
}

int covReadSizeTRIANG(int fd, int *numStrips, int *numCorners, int *numPoints)
{
    return covIoMetaLines(fd, READ_SIZE, "TRIANG", numStrips, NULL, numCorners, NULL,
                          numPoints, NULL, NULL, NULL, NULL, NULL, NULL);
}

int covReadTRIANG(int fd, int numStrips, int *stripList, int numCorners, int *cornerList,
                  int numPoints, float *x, float *y, float *z)
{
    return covIoMetaLines(fd, READ_COVISE, "TRIANG", &numStrips, stripList, &numCorners, cornerList,
                          &numPoints, x, y, z, NULL, NULL, NULL);
}

/*  *********************************
                  UNIGRD
    *********************************/

int covIoUNIGRD(int fd, int mode, int *xsize, int *ysize, int *zsize, float *xmin, float *xmax, float *ymin,
                float *ymax, float *zmin, float *zmax, char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "UNIGRD", 6);
    if (mode != WRITE_COVISE)
    {
        COV_READ_INT(fd, xsize, 1);
        SWAP_INT(fd, xsize, 1);
        COV_READ_INT(fd, ysize, 1);
        SWAP_INT(fd, ysize, 1);
        COV_READ_INT(fd, zsize, 1);
        SWAP_INT(fd, zsize, 1);
        COV_READ_FLOAT(fd, xmin, 1);
        SWAP_FLOAT(fd, xmin, 1);
        COV_READ_FLOAT(fd, xmax, 1);
        SWAP_FLOAT(fd, xmax, 1);
        COV_READ_FLOAT(fd, ymin, 1);
        SWAP_FLOAT(fd, ymin, 1);
        COV_READ_FLOAT(fd, ymax, 1);
        SWAP_FLOAT(fd, ymax, 1);
        COV_READ_FLOAT(fd, zmin, 1);
        SWAP_FLOAT(fd, zmin, 1);
        COV_READ_FLOAT(fd, zmax, 1);
        SWAP_FLOAT(fd, zmax, 1);
    }
    else
    {
        COV_WRITE(fd, xsize, sizeof(int));
        COV_WRITE(fd, ysize, sizeof(int));
        COV_WRITE(fd, zsize, sizeof(int));
        COV_WRITE(fd, xmin, sizeof(float));
        COV_WRITE(fd, xmax, sizeof(float));
        COV_WRITE(fd, ymin, sizeof(float));
        COV_WRITE(fd, ymax, sizeof(float));
        COV_WRITE(fd, zmin, sizeof(float));
        COV_WRITE(fd, zmax, sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteUNIGRD(int fd, int xsize, int ysize, int zsize, float xmin, float xmax, float ymin, float ymax, float zmin, float zmax,
                   char **atNam, char **atVal, int numAttr)
{
    return covIoUNIGRD(fd, WRITE_COVISE, &xsize, &ysize, &zsize, &xmin, &xmax, &ymin,
                       &ymax, &zmin, &zmax, atNam, atVal, &numAttr);
}

int covReadUNIGRD(int fd, int *xsize, int *ysize, int *zsize, float *xmin, float *xmax, float *ymin, float *ymax, float *zmin, float *zmax)
{
    return covIoUNIGRD(fd, READ_COVISE, xsize, ysize, zsize, xmin, xmax, ymin,
                       ymax, zmin, zmax, NULL, NULL, NULL);
}

/*  *********************************
                  RCTGRD
    *********************************/

int covIoRCTGRD(int fd, int mode, int *xsize, int *ysize, int *zsize, float *x, float *y, float *z,
                char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "RCTGRD", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, xsize, 1);
            SWAP_INT(fd, xsize, 1);
            COV_READ_INT(fd, ysize, 1);
            SWAP_INT(fd, ysize, 1);
            COV_READ_INT(fd, zsize, 1);
            SWAP_INT(fd, zsize, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_FLOAT(fd, x, *xsize);
        SWAP_FLOAT(fd, x, *xsize);
        COV_READ_FLOAT(fd, y, *ysize);
        SWAP_FLOAT(fd, y, *ysize);
        COV_READ_FLOAT(fd, z, *zsize);
        SWAP_FLOAT(fd, z, *zsize);
    }
    else
    {
        COV_WRITE(fd, xsize, sizeof(int));
        COV_WRITE(fd, ysize, sizeof(int));
        COV_WRITE(fd, zsize, sizeof(int));
        COV_WRITE(fd, x, *xsize * sizeof(int));
        COV_WRITE(fd, y, *ysize * sizeof(int));
        COV_WRITE(fd, z, *zsize * sizeof(int));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteRCTGRD(int fd, int xsize, int ysize, int zsize, float *x, float *y, float *z, char **atNam, char **atVal, int numAttr)
{
    return covIoRCTGRD(fd, WRITE_COVISE, &xsize, &ysize, &zsize, x, y, z, atNam, atVal, &numAttr);
}

int covReadSizeRCTGRD(int fd, int *xsize, int *ysize, int *zsize)
{
    return covIoRCTGRD(fd, READ_SIZE, xsize, ysize, zsize, NULL, NULL, NULL, NULL, NULL, NULL);
}

int covReadRCTGRD(int fd, int xsize, int ysize, int zsize, float *x, float *y, float *z)
{
    return covIoRCTGRD(fd, READ_COVISE, &xsize, &ysize, &zsize, x, y, z, NULL, NULL, NULL);
}

/*  *********************************
                  STRGRD
    *********************************/

int covIoSTRGRD(int fd, int mode, int *xsize, int *ysize, int *zsize, float *x, float *y, float *z,
                char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "STRGRD", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, xsize, 1);
            SWAP_INT(fd, xsize, 1);
            COV_READ_INT(fd, ysize, 1);
            SWAP_INT(fd, ysize, 1);
            COV_READ_INT(fd, zsize, 1);
            SWAP_INT(fd, zsize, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_FLOAT(fd, x, (*xsize) * (*ysize) * (*zsize));
        SWAP_FLOAT(fd, x, (*xsize) * (*ysize) * (*zsize));
        COV_READ_FLOAT(fd, y, (*xsize) * (*ysize) * (*zsize));
        SWAP_FLOAT(fd, y, (*xsize) * (*ysize) * (*zsize));
        COV_READ_FLOAT(fd, z, (*xsize) * (*ysize) * (*zsize));
        SWAP_FLOAT(fd, z, (*xsize) * (*ysize) * (*zsize));
    }
    else
    {
        COV_WRITE(fd, xsize, sizeof(int));
        COV_WRITE(fd, ysize, sizeof(int));
        COV_WRITE(fd, zsize, sizeof(int));
        COV_WRITE(fd, x, (*xsize) * (*ysize) * (*zsize) * sizeof(float));
        COV_WRITE(fd, y, (*xsize) * (*ysize) * (*zsize) * sizeof(float));
        COV_WRITE(fd, z, (*xsize) * (*ysize) * (*zsize) * sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteSTRGRD(int fd, int xsize, int ysize, int zsize, float *x, float *y, float *z,
                   char **atNam, char **atVal, int numAttr)
{
    return covIoSTRGRD(fd, WRITE_COVISE, &xsize, &ysize, &zsize, x, y, z, atNam, atVal, &numAttr);
}

int covReadSizeSTRGRD(int fd, int *xsize, int *ysize, int *zsize)
{
    return covIoSTRGRD(fd, READ_SIZE, xsize, ysize, zsize, NULL, NULL, NULL, NULL, NULL, NULL);
}

int covReadSTRGRD(int fd, int xsize, int ysize, int zsize, float *x, float *y, float *z)
{
    return covIoSTRGRD(fd, READ_COVISE, &xsize, &ysize, &zsize, x, y, z, NULL, NULL, NULL);
}

/*  *********************************
                  USTSDT
    *********************************/

int covIoUSTSDT(int fd, int mode, int *numElem, float *x, char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "USTSDT", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, numElem, 1);
            SWAP_INT(fd, numElem, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_FLOAT(fd, x, *numElem);
        SWAP_FLOAT(fd, x, *numElem);
    }
    else
    {
        COV_WRITE(fd, numElem, sizeof(int));
        COV_WRITE(fd, x, *numElem * sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteUSTSDT(int fd, int numElem, float *x, char **atNam, char **atVal, int numAttr)
{
    return covIoUSTSDT(fd, WRITE_COVISE, &numElem, x, atNam, atVal, &numAttr);
}

int covReadSizeUSTSDT(int fd, int *numElem)
{
    return covIoUSTSDT(fd, READ_SIZE, numElem, NULL, NULL, NULL, NULL);
}

int covReadUSTSDT(int fd, int numElem, float *x)
{
    return covIoUSTSDT(fd, READ_COVISE, &numElem, x, NULL, NULL, NULL);
}

/*  *********************************
                  USTTDT
    *********************************/

int covIoUSTTDT(int fd, int mode, int *numElem, int *type, float *x, char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "USTTDT", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, numElem, 1);
            SWAP_INT(fd, numElem, 1);
            COV_READ_INT(fd, type, 1);
            SWAP_INT(fd, type, *numElem);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_FLOAT(fd, x, (*numElem) * (*type));
        SWAP_FLOAT(fd, x, (*numElem) * (*type));
    }
    else
    {
        COV_WRITE(fd, numElem, sizeof(int));
        COV_WRITE(fd, type, sizeof(int));
        COV_WRITE(fd, x, (*numElem) * (*type) * sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteUSTTDT(int fd, int numElem, int type, float *x, char **atNam, char **atVal, int numAttr)
{
    return covIoUSTTDT(fd, WRITE_COVISE, &numElem, &type, x, atNam, atVal, &numAttr);
}

int covReadSizeUSTTDT(int fd, int *numElem, int *type)
{
    return covIoUSTTDT(fd, READ_SIZE, numElem, type, NULL, NULL, NULL, NULL);
}

int covReadUSTTDT(int fd, int numElem, int type, float *x)
{
    return covIoUSTTDT(fd, READ_COVISE, &numElem, &type, x, NULL, NULL, NULL);
}

/*  *********************************
                  USTVDT
    *********************************/

int covIoUSTVDT(int fd, int mode, int *numElem,
                float *x, float *y, float *z,
                char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "USTVDT", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, numElem, 1);
            SWAP_INT(fd, numElem, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_FLOAT(fd, x, *numElem);
        SWAP_FLOAT(fd, x, *numElem);
        COV_READ_FLOAT(fd, y, *numElem);
        SWAP_FLOAT(fd, y, *numElem);
        COV_READ_FLOAT(fd, z, *numElem);
        SWAP_FLOAT(fd, z, *numElem);
    }
    else
    {
        COV_WRITE(fd, numElem, sizeof(int));
        COV_WRITE(fd, x, *numElem * sizeof(float));
        COV_WRITE(fd, y, *numElem * sizeof(float));
        COV_WRITE(fd, z, *numElem * sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteUSTVDT(int fd, int numElem,
                   float *x, float *y, float *z,
                   char **atNam, char **atVal, int numAttr)
{
    return covIoUSTVDT(fd, WRITE_COVISE, &numElem, x, y, z, atNam, atVal, &numAttr);
}

int covReadSizeUSTVDT(int fd, int *numElem)
{
    return covIoUSTVDT(fd, READ_SIZE, numElem, NULL, NULL, NULL, NULL, NULL, NULL);
}

int covReadUSTVDT(int fd, int numElem, float *x, float *y, float *z)
{
    return covIoUSTVDT(fd, READ_COVISE, &numElem, x, y, z, NULL, NULL, NULL);
}

/*  *********************************
                  STRSDT
    *********************************/

int covIoSTRSDT(int fd, int mode, int *numElem, float *data, int *xsize, int *ysize, int *zsize,
                char **atNam, char **atVal, int *numAttr)
{
    (void)numElem;
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "STRSDT", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, xsize, 1);
            SWAP_INT(fd, xsize, 1);
            COV_READ_INT(fd, ysize, 1);
            SWAP_INT(fd, ysize, 1);
            COV_READ_INT(fd, zsize, 1);
            SWAP_INT(fd, zsize, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_FLOAT(fd, data, (*xsize) * (*ysize) * (*zsize));
        SWAP_FLOAT(fd, data, (*xsize) * (*ysize) * (*zsize));
    }
    else
    {
        COV_WRITE(fd, xsize, sizeof(int));
        COV_WRITE(fd, ysize, sizeof(int));
        COV_WRITE(fd, zsize, sizeof(int));
        COV_WRITE(fd, data, (*xsize) * (*ysize) * (*zsize) * sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteSTRSDT(int fd, int numElem, float *data, int xsize, int ysize, int zsize,
                   char **atNam, char **atVal, int numAttr)
{
    return covIoSTRSDT(fd, WRITE_COVISE, &numElem, data, &xsize, &ysize, &zsize,
                       atNam, atVal, &numAttr);
}

int covReadSizeSTRSDT(int fd, int *numElem, int *xsize, int *ysize, int *zsize)
{
    return covIoSTRSDT(fd, READ_SIZE, numElem, NULL, xsize, ysize, zsize, NULL, NULL, NULL);
}

int covReadSTRSDT(int fd, int numElem, float *data, int xsize, int ysize, int zsize)
{
    return covIoSTRSDT(fd, READ_COVISE, &numElem, data, &xsize, &ysize, &zsize, NULL, NULL, NULL);
}

/*  *********************************
                  STRVDT
    *********************************/

int covIoSTRVDT(int fd, int mode, int *numElem, float *data_x, float *data_y, float *data_z, int *xsize, int *ysize, int *zsize,
                char **atNam, char **atVal, int *numAttr)
{
    (void)numElem;
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "STRVDT", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, xsize, 1);
            SWAP_INT(fd, xsize, 1);
            COV_READ_INT(fd, ysize, 1);
            SWAP_INT(fd, ysize, 1);
            COV_READ_INT(fd, zsize, 1);
            SWAP_INT(fd, zsize, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_FLOAT(fd, data_x, (*xsize) * (*ysize) * (*zsize));
        SWAP_FLOAT(fd, data_x, (*xsize) * (*ysize) * (*zsize));
        COV_READ_FLOAT(fd, data_y, (*xsize) * (*ysize) * (*zsize));
        SWAP_FLOAT(fd, data_y, (*xsize) * (*ysize) * (*zsize));
        COV_READ_FLOAT(fd, data_z, (*xsize) * (*ysize) * (*zsize));
        SWAP_FLOAT(fd, data_z, (*xsize) * (*ysize) * (*zsize));
    }
    else
    {
        COV_WRITE(fd, xsize, sizeof(int));
        COV_WRITE(fd, ysize, sizeof(int));
        COV_WRITE(fd, zsize, sizeof(int));
        COV_WRITE(fd, data_x, (*xsize) * (*ysize) * (*zsize) * sizeof(float));
        COV_WRITE(fd, data_y, (*xsize) * (*ysize) * (*zsize) * sizeof(float));
        COV_WRITE(fd, data_z, (*xsize) * (*ysize) * (*zsize) * sizeof(float));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteSTRVDT(int fd, int numElem, float *data_x, float *data_y, float *data_z, int xsize, int ysize, int zsize,
                   char **atNam, char **atVal, int numAttr)
{
    return covIoSTRVDT(fd, WRITE_COVISE, &numElem, data_x, data_y, data_z, &xsize, &ysize, &zsize,
                       atNam, atVal, &numAttr);
}

int covReadSizeSTRVDT(int fd, int *numElem, int *xsize, int *ysize, int *zsize)
{
    return covIoSTRVDT(fd, READ_SIZE, numElem, NULL, NULL, NULL, xsize, ysize, zsize, NULL, NULL, NULL);
}

int covReadSTRVDT(int fd, int numElem, float *data_x, float *data_y, float *data_z, int xsize, int ysize, int zsize)
{
    return covIoSTRVDT(fd, READ_COVISE, &numElem, data_x, data_y, data_z, &xsize, &ysize, &zsize,
                       NULL, NULL, NULL);
}

/*  *********************************
                  RGBADT
    *********************************/

int covIoRGBADT(int fd, int mode, int *numElem, int *colors,
                char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "RGBADT", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, numElem, 1);
            SWAP_INT(fd, numElem, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_INT(fd, colors, *numElem);
        SWAP_INT(fd, colors, *numElem);
    }
    else
    {
        COV_WRITE(fd, numElem, sizeof(int));
        COV_WRITE(fd, colors, *numElem * sizeof(int));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteRGBADT(int fd, int numElem, int *colors,
                   char **atNam, char **atVal, int numAttr)
{
    return covIoRGBADT(fd, WRITE_COVISE, &numElem, colors, atNam, atVal, &numAttr);
}

int covReadSizeRGBADT(int fd, int *numElem)
{
    return covIoRGBADT(fd, READ_SIZE, numElem, NULL, NULL, NULL, NULL);
}

int covReadRGBADT(int fd, int numElem, int *colors)
{
    return covIoRGBADT(fd, READ_COVISE, &numElem, colors, NULL, NULL, NULL);
}

/*  *********************************
                  IMAGE
    *********************************/

int covIoIMAGE(int fd, int mode, int *PixelImageWidth, int *PixelImageHeight, int *PixelImageSize,
               int *PixelImageFormatId, int *PixelImageBufferLength, char *PixelImageBuffer,
               char **ImageatNam, char **ImageatVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "IMAGE", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, PixelImageWidth, 1);
            SWAP_INT(fd, PixelImageWidth, 1);
            COV_READ_INT(fd, PixelImageHeight, 1);
            SWAP_INT(fd, PixelImageHeight, 1);
            COV_READ_INT(fd, PixelImageSize, 1);
            SWAP_INT(fd, PixelImageSize, 1);
            COV_READ_INT(fd, PixelImageFormatId, 1);
            SWAP_INT(fd, PixelImageFormatId, 1);
            COV_READ_INT(fd, PixelImageBufferLength, 1);
            SWAP_INT(fd, PixelImageBufferLength, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ(fd, PixelImageBuffer, *PixelImageBufferLength);
    }
    else
    {
        COV_WRITE(fd, PixelImageWidth, sizeof(int));
        COV_WRITE(fd, PixelImageHeight, sizeof(int));
        COV_WRITE(fd, PixelImageSize, sizeof(int));
        COV_WRITE(fd, PixelImageFormatId, sizeof(int));
        *PixelImageBufferLength = (*PixelImageHeight) * (*PixelImageWidth) * (*PixelImageSize);
        COV_WRITE(fd, PixelImageBufferLength, sizeof(int));
        COV_WRITE(fd, PixelImageBuffer, *PixelImageBufferLength);
        covWriteAttrib(fd, *numAttr, ImageatNam, ImageatVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteIMAGE(int fd, int PixelImageWidth, int PixelImageHeight, int PixelImageSize,
                  int PixelImageFormatId, char *PixelImageBuffer,
                  char **ImageatNam, char **ImageatVal, int numAttr)
{
    int PixImageBufferLength;
    return covIoIMAGE(fd, WRITE_COVISE, &PixelImageWidth, &PixelImageHeight, &PixelImageSize,
                      &PixelImageFormatId, &PixImageBufferLength, PixelImageBuffer, ImageatNam, ImageatVal, &numAttr);
}

int covReadSizeIMAGE(int fd, int *PixelImageWidth, int *PixelImageHeight, int *PixelImageSize,
                     int *PixelImageFormatId, int *PixImageBufferLength)
{
    return covIoIMAGE(fd, READ_SIZE, PixelImageWidth, PixelImageHeight, PixelImageSize,
                      PixelImageFormatId, PixImageBufferLength, NULL, NULL, NULL, NULL);
}

int covReadIMAGE(int fd, int PixelImageWidth, int PixelImageHeight, int PixelImageSize,
                 int PixelImageFormatId, int PixImageBufferLength, char *PixelImageBuffer)
{
    return covIoIMAGE(fd, READ_COVISE, &PixelImageWidth, &PixelImageHeight, &PixelImageSize,
                      &PixelImageFormatId, &PixImageBufferLength, PixelImageBuffer, NULL, NULL, NULL);
}

/*  *********************************
                  TEXTUR
    *********************************/

int covIoTEXTUR(int fd, int mode, int *PixelImageWidth, int *PixelImageHeight, int *PixelImageSize,
                int *PixelImageFormatId, int *PixImageBufferLength, char *PixelImageBuffer,
                char **ImageatNam, char **ImageatVal, int *numImageAttr,
                int *NumberOfBorderPixels, int *NumberOfComponents, int *Level,
                int *NumberOfCoordinates, int *NumberOfVertices, int *VertexIndices,
                float **Coords, char **TextatNam, char **TextatVal, int *numTextAttr)
{
    (void)PixImageBufferLength;
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "TEXTUR", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, NumberOfBorderPixels, 1);
            SWAP_INT(fd, NumberOfBorderPixels, 1);
            COV_READ_INT(fd, NumberOfComponents, 1);
            SWAP_INT(fd, NumberOfComponents, 1);
            COV_READ_INT(fd, Level, 1);
            SWAP_INT(fd, Level, 1);
            COV_READ_INT(fd, NumberOfCoordinates, 1);
            SWAP_INT(fd, NumberOfCoordinates, 1);
            COV_READ_INT(fd, NumberOfVertices, 1);
            SWAP_INT(fd, NumberOfVertices, 1);
            return CHECK_FOR_ERRORS;
        }
        COV_READ_INT(fd, VertexIndices, *NumberOfVertices);
        SWAP_INT(fd, VertexIndices, *NumberOfVertices);
        COV_READ_FLOAT(fd, Coords[0], *NumberOfCoordinates);
        SWAP_FLOAT(fd, Coords[0], *NumberOfCoordinates);
        COV_READ_FLOAT(fd, Coords[1], *NumberOfCoordinates);
        SWAP_FLOAT(fd, Coords[1], *NumberOfCoordinates);
    }
    else
    {
        covWriteIMAGE(fd, *PixelImageWidth, *PixelImageHeight, *PixelImageSize,
                      *PixelImageFormatId, PixelImageBuffer, ImageatNam, ImageatVal, *numImageAttr);
        COV_WRITE(fd, NumberOfBorderPixels, sizeof(int));
        COV_WRITE(fd, NumberOfComponents, sizeof(int));
        COV_WRITE(fd, Level, sizeof(int));
        COV_WRITE(fd, NumberOfCoordinates, sizeof(int));
        COV_WRITE(fd, NumberOfVertices, sizeof(int));
        COV_WRITE(fd, VertexIndices, *NumberOfVertices * sizeof(int));
        COV_WRITE(fd, Coords[0], *NumberOfCoordinates * sizeof(float));
        COV_WRITE(fd, Coords[1], *NumberOfCoordinates * sizeof(float));
        covWriteAttrib(fd, *numTextAttr, TextatNam, TextatVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteTEXTUR(int fd, int PixelImageWidth, int PixelImageHeight, int PixelImageSize,
                   int PixelImageFormatId, char *PixelImageBuffer,
                   char **ImageatNam, char **ImageatVal, int numImageAttr,
                   int NumberOfBorderPixels, int NumberOfComponents, int Level,
                   int NumberOfCoordinates, int NumberOfVertices, int *VertexIndices,
                   float **Coords, char **TextatNam, char **TextatVal, int numTextAttr)
{
    int PixImageBufferLength;
    return covIoTEXTUR(fd, WRITE_COVISE, &PixelImageWidth, &PixelImageHeight, &PixelImageSize,
                       &PixelImageFormatId, &PixImageBufferLength, PixelImageBuffer,
                       ImageatNam, ImageatVal, &numImageAttr,
                       &NumberOfBorderPixels, &NumberOfComponents, &Level,
                       &NumberOfCoordinates, &NumberOfVertices, VertexIndices,
                       Coords, TextatNam, TextatVal, &numTextAttr);
}

int covReadSizeTEXTUR(int fd, int *NumberOfBorderPixels, int *NumberOfComponents, int *Level,
                      int *NumberOfCoordinates, int *NumberOfVertices)
{
    return covIoTEXTUR(fd, READ_SIZE, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                       NumberOfBorderPixels, NumberOfComponents, Level,
                       NumberOfCoordinates, NumberOfVertices, NULL, NULL, NULL, NULL, NULL);
}

int covReadTEXTUR(int fd, int NumberOfBorderPixels, int NumberOfComponents, int Level,
                  int NumberOfCoordinates, int NumberOfVertices, int *VertexIndices,
                  float **Coords)
{
    return covIoTEXTUR(fd, READ_COVISE, NULL, NULL, NULL,
                       NULL, NULL, NULL, NULL, NULL, NULL,
                       &NumberOfBorderPixels, &NumberOfComponents, &Level,
                       &NumberOfCoordinates, &NumberOfVertices, VertexIndices,
                       Coords, NULL, NULL, NULL);
}

/*  *********************************
                  INTARR
    *********************************/

int covIoINTARR(int fd, int mode, int *numDim, int *numElem, int *dim_array, int *data,
                char **atNam, char **atVal, int *numAttr)
{
    if (mode == WRITE_COVISE)
        COV_WRITE(fd, "INTARR", 6);
    if (mode != WRITE_COVISE)
    {
        if (mode == READ_DIM)
        {
            COV_READ_INT(fd, numDim, 1);
            SWAP_INT(fd, numDim, 1);
            return CHECK_FOR_ERRORS;
        }
        if (mode == READ_SIZE)
        {
            COV_READ_INT(fd, numElem, 1);
            SWAP_INT(fd, numElem, 1);
            COV_READ_INT(fd, dim_array, (*numDim));
            SWAP_INT(fd, dim_array, (*numDim));
            return CHECK_FOR_ERRORS;
        }

        COV_READ_INT(fd, data, (*numElem));
        SWAP_INT(fd, data, (*numElem));
    }
    else
    {
        COV_WRITE(fd, numDim, sizeof(int));
        COV_WRITE(fd, numElem, sizeof(int));
        COV_WRITE(fd, dim_array, (*numDim) * sizeof(int));
        COV_WRITE(fd, data, (*numElem) * sizeof(int));
        covWriteAttrib(fd, *numAttr, atNam, atVal);
    }
    return CHECK_FOR_ERRORS;
}

int covWriteINTARR(int fd, int numDim, int numElem, int *dim_array, int *data,
                   char **atNam, char **atVal, int numAttr)
{
    return covIoINTARR(fd, WRITE_COVISE, &numDim, &numElem, dim_array, data, atNam, atVal, &numAttr);
}

int covReadDimINTARR(int fd, int *numDim)
{
    return covIoINTARR(fd, READ_DIM, numDim, NULL, NULL, NULL, NULL, NULL, NULL);
}

int covReadSizeINTARR(int fd, int numDim, int *sizes, int *numElem)
{
    return covIoINTARR(fd, READ_SIZE, &numDim, numElem, sizes, NULL, NULL, NULL, NULL);
}

int covReadINTARR(int fd, int numDim, int numElem, int *dim_array, int *data)
{
    return covIoINTARR(fd, READ_COVISE, &numDim, &numElem, dim_array, data, NULL, NULL, NULL);
}
