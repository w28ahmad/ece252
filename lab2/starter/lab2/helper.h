/**
 * @brief  micros and structures for a simple PNG file 
 *
 * Copyright 2018-2020 Yiqing Huang
 *
 * This software may be freely redistributed under the terms of MIT License
 */
#pragma once

/******************************************************************************
 * INCLUDE HEADER FILES
 *****************************************************************************/
#include <stdio.h>

/******************************************************************************
 * DEFINED MACROS 
 *****************************************************************************/

#define PNG_SIG_SIZE    8 /* number of bytes of png image signature data */
#define CHUNK_LEN_SIZE  4 /* chunk length field size in bytes */          
#define CHUNK_TYPE_SIZE 4 /* chunk type field size in bytes */
#define CHUNK_CRC_SIZE  4 /* chunk CRC field size in bytes */
#define DATA_IHDR_SIZE 13 /* IHDR chunk data field size */

#define IMG_URL_1 "http://ece252-1.uwaterloo.ca:2520/image?img=1"
#define IMG_URL_2 "http://ece252-1.uwaterloo.ca:2520/image?img=2"
#define IMG_URL_3 "http://ece252-1.uwaterloo.ca:2520/image?img=3"
#define DUM_URL "https://example.com/"
/******************************************************************************
 * STRUCTURES and TYPEDEFS 
 *****************************************************************************/
typedef unsigned char U8;
typedef unsigned int  U32;

/* A simple PNG file format, three chunks only*/
typedef struct simple_PNG {
    struct chunk *p_IHDR;
    struct chunk *p_IDAT;  /* only handles one IDAT chunk */  
    struct chunk *p_IEND;
} *simple_PNG_p;


/******************************************************************************
 * FUNCTION PROTOTYPES 
 *****************************************************************************/
/* int is_png(U8 *buf, size_t n); */
/* int get_png_height(struct data_IHDR *buf); */
/* int get_png_width(struct data_IHDR *buf); */
/* int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence); */

/* declare your own functions prototypes here */
