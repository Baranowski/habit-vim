#include <string.h>
#include <stdlib.h>
#include "sarray.h"

/* auxiliary routines preparatory to suffix sort

    uchar *codetab(const uchar *s) 
	returns a dense code table for characters in
	null-terminated string s.  The map from ascii
	is monotone increasing, with \0 encoded as \0.

    int *scode(const char *s)
	returns an encoding of string s into a 0-terminated
	array of integers.

    uchar *inverse(const uchar *t) returns the inverse
	of code table t

    these functions return 0 if space can't be allocated
*/

int *scode(const char *s0)
{
	int i;
	const uchar *s = (const uchar*)s0;
	uchar *t = codetab((uchar*)s);
	int *r = (int*)malloc((strlen(s)+1)*sizeof(int));
	if(t && r)
		for(i=0; ; i++) {
			r[i] = t[s[i]];
			if(s[i] == 0)
				break;
		}
	free(t);
	return r;
}

uchar *codetab(const uchar *s)
{
	int i, n;
	uchar *t = (uchar*)calloc(256,1);
	if(t) {
		for( ; *s; s++)
			t[*s] = 1;
		for(i=n=1; i<256; i++)
			if(t[i])
				t[i] = n++;
	}
	return t;
}

uchar *inverse(const uchar *t)
{
	int i;
	uchar *r = (uchar*)calloc(256,1);
	if(r==0 || t==0)
		return 0;
	for(i=0; i<256; i++)
		if(t[i])
			r[t[i]] = i;
	return r;
}
