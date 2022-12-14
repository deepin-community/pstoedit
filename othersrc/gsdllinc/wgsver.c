/* Copyright (C) 2001-2003, Ghostgum Software Pty Ltd.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* 
 * This file is part of GSview.  The above notice applies only 
 * to this file.  It does NOT apply to any other file in GSview
 * unless that file includes the above copyright notice.
 */

/* $Id: wgsver.c,v 1.1.2.4 2003/07/14 08:03:09 ghostgum Exp $ */
/* Obtain details of copies of Ghostscript installed under Windows */

/* To compile as a demo program, define DUMP_GSVER */
/* #define DUMP_GSVER */

#include <windows.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include "wgsver.h"
#include "cppcomp.h"

/* Ghostscript may be known in the Windows Registry by
 * the following names.
 */
static const char * const gs_products[] = {
/* #define GS_PRODUCT_AFPL		*/ "AFPL Ghostscript",
/* #define GS_PRODUCT_ALADDIN	*/ "Aladdin Ghostscript",
/* #define GS_PRODUCT_GPL		*/ "GPL Ghostscript",
/* #define GS_PRODUCT_GNU		*/ "GNU Ghostscript",
	0};


/* Get Ghostscript versions for given product.
 * Store results starting at pver + 1 + offset.
 * Returns total number of versions in pver.
 */
static int get_gs_versions_product(int *pver, int offset, 
	HKEY hkeyroot, REGSAM regopenflags,
	const char *gs_productfamily, const char *gsregbase,
	bool verbose)
{
    HKEY hkey;
    DWORD cbData;
    
    char key[256];
    int ver;
    char *p;
    int n = 0;

	if (strlen(gsregbase))
	  sprintf_s(TARGETWITHLEN(key,256) , "Software\\%s\\%s", gsregbase, gs_productfamily);
	else
	  sprintf_s(TARGETWITHLEN(key,256) ,"Software\\%s", gs_productfamily);

#ifdef OS_WIN32_WCE
	const long regtestresult = RegOpenKeyEx(hkeyroot, LPSTRtoLPWSTR(key).c_str(), 0, KEY_READ|regopenflags , &hkey);
#else
	const long regtestresult = RegOpenKeyExA(hkeyroot, key, 0, KEY_READ|regopenflags , &hkey);
#endif
    if (regtestresult == ERROR_SUCCESS) {
	/* Now enumerate the keys */
	if (verbose)	fprintf(stdout," return code for \"%s\" is %d\n", key, regtestresult);
	cbData = sizeof(key) / sizeof(char);
#ifdef OS_WIN32_WCE
	while (RegEnumKeyEx(hkey, n, (LPWSTR)LPSTRtoLPWSTR(key).c_str(), &cbData, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) { 
#else
	while (RegEnumKeyA(hkey, n, key, cbData) == ERROR_SUCCESS) {
#endif
	if (verbose) fprintf(stdout, " enumerate gs versions: \"%s\" is number %d ", key, n+offset);
	    n++;
	    ver = 0;
	    p = key;
		int major = 0;
	    while (*p && (*p!='.')) {
			major *= 10;
			major += (*p - '0');
		  p++;
	    }
		if (*p == '.') {
			int minor = 0;
			p++;
			while (*p && (*p != '.')) {
				minor *= 10;
				minor += (*p - '0');
				p++;
			}
			int pl = 0;
			if (*p == '.') {
				// new scheme nn.nn.PL
				p++;
				while (*p) {
					pl *= 10;
					pl += (*p - '0');
					p++;
				}
			}
			ver = major * 10000 + minor * 100 + pl;
		} else {
		  // not expected, but ...
		  ver = major * 10000;
		}

	    if (n + offset < pver[0]) {  /* the pver[0] item contains the lenght of the pver vector */
								     /* this function is called also just for counting purposes */
			pver[n+offset] = ver;
		}
		if (verbose) fprintf(stdout, "mapped to %d\n", ver);
	}
    } else {
		/* 
		fprintf(stdout," return code for \"%s\" is %d\n", key, regtestresult);
		*/
	}
    return n+offset;
}

/* Query registry to find which versions of Ghostscript are installed.
 * Return version numbers in an integer array.   
 * On entry, the first element in the array must be the array size 
 * in elements.
 * If all is well, TRUE is returned.
 * On exit, the first element is set to the number of Ghostscript
 * versions installed, and subsequent elements to the version
 * numbers of Ghostscript.
 * e.g. on entry {5, 0, 0, 0, 0}, on exit {3, 550, 600, 596, 0}
 * Returned version numbers may not be sorted.
 *
 * If Ghostscript is not installed at all, return FALSE
 * and set pver[0] to 0.
 * If the array is not large enough, return FALSE 
 * and set pver[0] to the number of Ghostscript versions installed.
 */
BOOL get_gs_versions(int *pver, const char *gsregbase, int verbose)
{
    int n=0;
    if (pver == (int *)NULL)
	    return FALSE;
	const char * const * productptr = &gs_products[0];
	while (productptr && *productptr) {
	    n = get_gs_versions_product(pver, n, HKEY_LOCAL_MACHINE, 0,					*productptr, gsregbase, verbose);
		n = get_gs_versions_product(pver, n, HKEY_CURRENT_USER,  0,					*productptr, gsregbase, verbose);
		n = get_gs_versions_product(pver, n, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY,	*productptr, gsregbase, verbose);
		n = get_gs_versions_product(pver, n, HKEY_CURRENT_USER,  KEY_WOW64_64KEY,	*productptr, gsregbase, verbose);
		productptr++;
	}

    if (n >= pver[0]) {
		pver[0] = n;
		return FALSE;	/* too small */
    }

    if (n == 0) {
		pver[0] = 0;
		return FALSE;	/* not installed */
    }
    pver[0] = n;
    return TRUE;
}

 
/*
 * Get a named registry value.
 * Key = hkeyroot\\key, named value = name.
 * name, ptr, plen and return values are the same as in gp_getenv();
 */

static int 
gp_getenv_registry(HKEY hkeyroot, REGSAM regopenflags, const char *key, const char *name, 
    char *ptr, int *plen)
{
    HKEY hkey;
    DWORD cbData, keytype;
    BYTE b;
    LONG rc;
    BYTE *bptr = (BYTE *)ptr;
	/*
	fprintf(stdout,"checking key %s %s\n",key,name);
	*/
#ifdef OS_WIN32_WCE
	if (RegOpenKeyEx(hkeyroot, LPSTRtoLPWSTR(key).c_str(), 0, KEY_READ |  regopenflags , &hkey)	== ERROR_SUCCESS) {
#else
	if (RegOpenKeyExA(hkeyroot, key, 0, KEY_READ |  regopenflags , &hkey)	== ERROR_SUCCESS) {
#endif
	keytype = REG_SZ;
	cbData = *plen;
	if (bptr == (BYTE *)NULL)
	    bptr = &b;	/* Registry API won't return ERROR_MORE_DATA */
			/* if ptr is NULL */
#ifdef OS_WIN32_WCE
	rc = RegQueryValueEx(hkey, LPSTRtoLPWSTR((char *)name).c_str(), 0, &keytype, bptr, &cbData);
#else
	rc = RegQueryValueExA(hkey, (char *)name, 0, &keytype, bptr, &cbData);
#endif
	(void)RegCloseKey(hkey);
	if (rc == ERROR_SUCCESS) {
	    *plen = cbData;
	    return 0;	/* found environment variable and copied it */
	} else if (rc == ERROR_MORE_DATA) {
	    /* buffer wasn't large enough */
	    *plen = cbData;
	    return -1;
	}
    }
    return 1;	/* not found */
}


static BOOL get_gs_string_product(int gs_revision, const char *name, 
    char *ptr, int len, const char *gs_productfamily, const char *gsregbase)
{
    /* If using Win32, look in the registry for a value with
     * the given name.  The registry value will be under the key
     * HKEY_CURRENT_USER\Software\AFPL Ghostscript\N.NN
     * or if that fails under the key
     * HKEY_LOCAL_MACHINE\Software\AFPL Ghostscript\N.NN
     * where "AFPL Ghostscript" is actually gs_productfamily
     * and N.NN is obtained from gs_revision.
     */

	/* new since Rel 9.93.0: We have also a PL 
	  hence all gs_revision is 99300 in this case
	  for older relases we use then 99200
	*/
    
    int code;
    char key[256];
    char dotversion[16];
    int length;
#if 0
	const DWORD version = GetVersion();
	// hope we do not need this anymore
    if (((HIWORD(version) & 0x8000) != 0)
	  && ((HIWORD(version) & 0x4000) == 0)) {
	/* Win32s */
	return FALSE;
    }
#endif

	if (gs_revision < 95300) {
		sprintf_s(TARGETWITHLEN(dotversion, 16), "%d.%02d",
			(int)(gs_revision / 10000), 
			(int)(gs_revision % 10000));
    } else {
		const int major = gs_revision / 10000;
		const int minor = (gs_revision - major * 10000) / 100;
		const int pl    = (gs_revision - major * 10000) % 100;
		sprintf_s(TARGETWITHLEN(dotversion, 16), "%d.%02d.%d",
			major, 
			minor,
			pl
		);
	}
	//fprintf(stdout, "DOT: %s\n", dotversion);

	
	if (strlen(gsregbase))
	  sprintf_s(TARGETWITHLEN(key,256), "Software\\%s\\%s\\%s", gsregbase, gs_productfamily, dotversion);
	else
	  sprintf_s(TARGETWITHLEN(key,256), "Software\\%s\\%s", gs_productfamily, dotversion);

    length = len;
    code = gp_getenv_registry(HKEY_CURRENT_USER, 0, key, name, ptr, &length);
    if ( code == 0 ) return TRUE;	/* found it */

    length = len;
    code = gp_getenv_registry(HKEY_LOCAL_MACHINE, 0, key, name, ptr, &length);
    if ( code == 0 ) return TRUE;	/* found it */

	length = len;
    code = gp_getenv_registry(HKEY_CURRENT_USER, KEY_WOW64_64KEY, key, name, ptr, &length);
    if ( code == 0 ) return TRUE;	/* found it */

    length = len;
    code = gp_getenv_registry(HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY, key, name, ptr, &length);
    if ( code == 0 ) return TRUE;	/* found it */

	return FALSE;
}

BOOL get_gs_string(int gs_revision, const char *name, char *ptr, int len, 
  const char *gsregbase)
{
	const char * const * productptr = &gs_products[0];
	while (productptr && *productptr) {
		if (get_gs_string_product(gs_revision, name, ptr, len, *productptr, gsregbase))
		return TRUE;
		productptr++;
	}
    return FALSE;
}



/* Set the latest Ghostscript EXE or DLL from the registry */
BOOL
find_gs(char *gspath, int len, int minver, BOOL bDLL, const char *gsregbase, int verbose)
{
#if 0
	// win32s no longer supported
    const DWORD version = GetVersion();
	if (((HIWORD(version) & 0x8000) != 0) && ((HIWORD(version) & 0x4000) == 0)) {
	  return FALSE;  // win32s
	}
#endif
	
    int count = 1;
    (void)get_gs_versions(&count, gsregbase, verbose);
	if (count < 1) {
	  return FALSE;
	}
	
	int* ver = new int[count + 1]; // (int *)malloc((count + 1) * sizeof(int));
	if (!ver) {
	    return FALSE;
	}
	
    ver[0] = count+1;
    if (!get_gs_versions(ver, gsregbase, verbose)) {
		delete[] ver; // free(ver);
	    return FALSE;
    }
    
	int maxversion = 10000000;
	const char * gsvmax = getenv("GS_V_MAX");
	if (gsvmax) {
		maxversion = atoi(gsvmax);
	}
	int gsver = 0;
	// find latest/max version
    for (int i=1; i<=ver[0]; i++) {
		if ((ver[i] > gsver) && (ver[i] <= maxversion)) {
			gsver = ver[i];
	    }
    }
	delete[] ver; // free(ver);
    if (gsver < minver) {	// minimum version (e.g. for gsprint)
		return FALSE;
	}
    
	char buf[1000];
	if (!get_gs_string(gsver, "GS_DLL", buf, sizeof(buf), gsregbase)) {
		 return FALSE;
	}
	
    if (bDLL) {
		strncpy_s(gspath, len, buf, len-1);
		return TRUE;
	} else {
		char * p = strrchr(buf, '\\');
		if (p) {
			p++;
			*p = 0;
#ifdef _WIN64
			strncpy_s(p,sizeof(buf)-1-strlen(buf), "gswin64c.exe", sizeof(buf)-1-strlen(buf));
#else
			strncpy_s(p,sizeof(buf)-1-strlen(buf), "gswin32c.exe", sizeof(buf)-1-strlen(buf));
#endif
			strncpy_s(gspath,len, buf, len-1);
			return TRUE;
		}
		return FALSE;
	}
}


#ifdef DUMP_GSVER
#define ENTRYPOINT main(int argc, char *argv[], char *gsregbase)
#else
#define ENTRYPOINT dumpgsvers(const char *gsregbase, int verbose)
#endif

/* This is an example of how you can use the above functions */
int ENTRYPOINT
{
    int ver[10];
    char buf[256];

	if (find_gs(buf, sizeof(buf), 550, TRUE, gsregbase, verbose)) {
		fprintf(stderr, "Latest GS DLL is %s\n", buf);
	}
	
	if (find_gs(buf, sizeof(buf), 550, FALSE, gsregbase, verbose)) {
		fprintf(stderr, "Latest GS EXE is %s\n", buf);
	}

    ver[0] = sizeof(ver) / sizeof(int);
    const BOOL flag = get_gs_versions(ver, gsregbase, verbose);
    fprintf(stderr,"Versions: %d\n", ver[0]);

    if (flag == FALSE) {
	  fprintf(stderr,"get_gs_versions failed, need %d\n", ver[0]);
	  return 1;
    }

    for (int i=1; i <= ver[0]; i++) {
	  fprintf(stderr," %d\n", ver[i]);
	  if (get_gs_string(ver[i], "GS_DLL", buf, sizeof(buf), gsregbase)) {
		fprintf(stderr,"   GS_DLL=%s\n", buf);
	  }
	  if (get_gs_string(ver[i], "GS_LIB", buf, sizeof(buf), gsregbase)) {
		fprintf(stderr,"   GS_LIB=%s\n", buf);
	  }
    }
    return 0;
}

