/*	$NetBSD$	*/

/*-
 * Copyright (c)2005 YAMAMOTO Takashi,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/queue.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define	DEBUG

#if defined(DEBUG)
#define	DFPRINTF(...)	fprintf(__VA_ARGS__)
#else
#define	DFPRINTF(...)	/* nothing */
#endif

#define	MAXID	102400

struct buf {
	TAILQ_ENTRY(buf) b_s;
	TAILQ_ENTRY(buf) b_q;
	int b_type;
#define	B_L	1
#define	B_H	2
#define	B_R	4
	int b_flags;
#define B_S	8
	int b_irr;
	int b_lastts;
};

struct buf bufs[MAXID];

TAILQ_HEAD(, buf) q_q;
TAILQ_HEAD(, buf) q_s;

int nlirs_max = 2;
int nbufs_max = 3;
int nlirs;
int nbufs;

char *
fgetln(FILE *fp, size_t *len);

char *
fparseln(FILE *fp, size_t *size, size_t *lineno, const char str[3],
		    int flags);

void buf_print(struct buf *, char *);

void
buf_print(struct buf *bp, char *s)
{

	DFPRINTF(stderr, "%d(%s,%s,%d)%s", (bp - bufs),
	    (bp->b_type == B_L) ? "L" :
	    (bp->b_type == (B_H | B_R)) ? "HR" :
	    (bp->b_type == B_H) ? "H" :
	    (bp->b_type == 0) ? "free" :
	    "unknown",
	    (bp->b_flags & B_S) ? "S" : "",
	    bp->b_irr,
	    s);
}

void
dump()
{
#if defined(DEBUG)
	struct buf *bp;
	int i;

	DFPRINTF(stderr, "S: ");
	TAILQ_FOREACH(bp, &q_s, b_s) {
		buf_print(bp, " ");
	}
	DFPRINTF(stderr, "\n");

	DFPRINTF(stderr, "Q: ");
	TAILQ_FOREACH(bp, &q_q, b_q) {
		buf_print(bp, " ");
	}
	DFPRINTF(stderr, "\n");

#if 0
	for (i = 0; i < 256; i++) {

		bp = &bufs[i];
		if (bufs->b_type == 0) {
			continue;
		}
	}
#endif

	DFPRINTF(stderr, "nlirs=%d, nbufs=%d\n", nlirs, nbufs);
#endif /* defined(DEBUG) */
}

void
reclaim()
{
	struct buf *bp;

	if (nbufs <= nbufs_max) {
		return;
	}

	bp = TAILQ_FIRST(&q_q);
	buf_print(bp, ": reclaim\n");
	assert(bp->b_type == (B_H | B_R));
	TAILQ_REMOVE(&q_q, bp, b_q);
	bp->b_type &= ~B_R;
	nbufs--;
}

void
prune()
{

	while (1) {
		struct buf *bp;

		bp = TAILQ_FIRST(&q_s);
		if (bp->b_type == B_L) {
			break;
		}
		buf_print(bp, ": prune\n");
		TAILQ_REMOVE(&q_s, bp, b_s);
		assert(bp->b_flags & B_S);
		bp->b_flags &= ~B_S;
		if ((bp->b_type & B_R) == 0) {
			bp->b_type &= ~B_H;
		}
	}
}

void
reclaim_l()
{
	struct buf *bp;

	if (nlirs <= nlirs_max) {
		return;
	}

	bp = TAILQ_FIRST(&q_s);
	buf_print(bp, ": reclaim_l\n");
	assert(bp->b_type == B_L);
	assert(bp->b_flags & B_S);
	bp->b_type = B_H | B_R;
	bp->b_flags &= ~B_S;
	TAILQ_REMOVE(&q_s, bp, b_s);
	TAILQ_INSERT_TAIL(&q_q, bp, b_q);
	nlirs--;
	prune();
}

void
init(int n)
{

	TAILQ_INIT(&q_q);
	TAILQ_INIT(&q_s);
	memset(&bufs, 0, sizeof(bufs));
	nbufs_max = n;
#if 0
	nlirs_max = nbufs_max * 2 / 3;
#else
	nlirs_max = nbufs_max * 90 / 100;
#endif
}

struct object {int dummy;};
int ts = 1;

void
fault(struct object *dummy, int i)
{
	struct buf *bp;

	DFPRINTF(stderr, "----------\n");
	dump();

	DFPRINTF(stderr, "---------- ts %d\n", ts);

	bp = &bufs[i];	
	buf_print(bp, ": access\n");
	if (bp->b_type == 0) {
		bp->b_irr = -1;
	} else {
		bp->b_irr = ts - bp->b_lastts - 1;
	}
	bp->b_lastts = ts;

	if (bp->b_type == B_L) {
		assert(bp->b_flags & B_S);
		TAILQ_REMOVE(&q_s, bp, b_s);
		TAILQ_INSERT_TAIL(&q_s, bp, b_s);
		prune();
		goto done;
	}
	if (bp->b_type == (B_H | B_R)) {
		if (bp->b_flags & B_S) {
			TAILQ_REMOVE(&q_s, bp, b_s);
			TAILQ_REMOVE(&q_q, bp, b_q);
			bp->b_type = B_L;
			nlirs++;
			reclaim_l();
		} else {
			TAILQ_REMOVE(&q_q, bp, b_q);
			TAILQ_INSERT_TAIL(&q_q, bp, b_q);
		}
		TAILQ_INSERT_TAIL(&q_s, bp, b_s);
		bp->b_flags |= B_S;
		goto done;
	}
	nbufs++;
	reclaim();
	if ((bp->b_flags & (B_R | B_L)) == 0) {
		printf("%d\n", i);
	}
	if (bp->b_type == 0) {
		buf_print(bp, ": new\n");
		if (nlirs < nlirs_max) {
			bp->b_type = B_L;
			TAILQ_INSERT_TAIL(&q_s, bp, b_s);
			bp->b_flags |= B_S;
			nlirs++;
		} else {
			bp->b_type = B_H;
		}
	}
	if (bp->b_type == B_H) {
		if (bp->b_flags & B_S) {
			TAILQ_REMOVE(&q_s, bp, b_s);
			bp->b_type = B_L;
			nlirs++;
			reclaim_l();
		} else {
			bp->b_type |= B_R;
			TAILQ_INSERT_TAIL(&q_q, bp, b_q);
		}
		TAILQ_INSERT_TAIL(&q_s, bp, b_s);
		bp->b_flags |= B_S;
	}
done:
	ts++;
}

void
test(void)
{
	struct object obj;
	memset(&obj, 0, sizeof(obj));
	char *ln;

	for (;; free(ln)) {
		int i;
		int ch;

		ln = fparseln(stdin, NULL, NULL, NULL, 0); 
		if (ln == NULL) {
			break;
		}
		ch = *ln;
		if (ch == '\0') {
			break;
		}
#if 1
		if (ch == 'd') {
			dump();
			continue;
		}
#endif
		i = atoi(ln);
		fault(&obj, i);
	}
}

int
main(int argc, char *argv[])
{

	init(atoi(argv[1]));
	test();
	exit(0);
}

char *
fgetln(FILE *fp, size_t *len)
{
	static char *buf = NULL;
	static size_t bufsiz = 0;
	char *ptr;


	if (buf == NULL) {
		bufsiz = BUFSIZ;
		if ((buf = malloc(bufsiz)) == NULL)
			return NULL;
	}

	if (fgets(buf, bufsiz, fp) == NULL)
		return NULL;

	*len = 0;
	while ((ptr = strchr(&buf[*len], '\n')) == NULL) {
		size_t nbufsiz = bufsiz + BUFSIZ;
		char *nbuf = realloc(buf, nbufsiz);

		if (nbuf == NULL) {
			int oerrno = errno;
			free(buf);
			errno = oerrno;
			buf = NULL;
			return NULL;
		} else
			buf = nbuf;

		*len = bufsiz;
		if (fgets(&buf[bufsiz], BUFSIZ, fp) == NULL)
			return buf;

		bufsiz = nbufsiz;
	}

	*len = (ptr - buf) + 1;
	return buf;
}

#define FPARSELN_UNESCESC	0x01
#define FPARSELN_UNESCCONT	0x02
#define FPARSELN_UNESCCOMM	0x04
#define FPARSELN_UNESCREST	0x08
#define FPARSELN_UNESCALL	0x0f

static int isescaped(const char *, const char *, int);

/* isescaped():
 *	Return true if the character in *p that belongs to a string
 *	that starts in *sp, is escaped by the escape character esc.
 */
static int
isescaped(const char *sp, const char *p, int esc)
{
	const char     *cp;
	size_t		ne;

	/* No escape character */
	if (esc == '\0')
		return 1;

	/* Count the number of escape characters that precede ours */
	for (ne = 0, cp = p; --cp >= sp && *cp == esc; ne++)
		continue;

	/* Return true if odd number of escape characters */
	return (ne & 1) != 0;
}


/* fparseln():
 *	Read a line from a file parsing continuations ending in \
 *	and eliminating trailing newlines, or comments starting with
 *	the comment char.
 */
char *
fparseln(FILE *fp, size_t *size, size_t *lineno, const char str[3],
    int flags)
{
	static const char dstr[3] = { '\\', '\\', '#' };
	char	*buf = NULL, *ptr, *cp, esc, con, nl, com;
	size_t	s, len = 0;
	int	cnt = 1;

	if (str == NULL)
		str = dstr;

	esc = str[0];
	con = str[1];
	com = str[2];

	/*
	 * XXX: it would be cool to be able to specify the newline character,
	 * but unfortunately, fgetln does not let us
	 */
	nl  = '\n';

	while (cnt) {
		cnt = 0;

		if (lineno)
			(*lineno)++;

		if ((ptr = fgetln(fp, &s)) == NULL)
			break;

		if (s && com) {		/* Check and eliminate comments */
			for (cp = ptr; cp < ptr + s; cp++)
				if (*cp == com && !isescaped(ptr, cp, esc)) {
					s = cp - ptr;
					cnt = s == 0 && buf == NULL;
					break;
				}
		}

		if (s && nl) {		/* Check and eliminate newlines */
			cp = &ptr[s - 1];

			if (*cp == nl)
				s--;	/* forget newline */
		}

		if (s && con) {		/* Check and eliminate continuations */
			cp = &ptr[s - 1];

			if (*cp == con && !isescaped(ptr, cp, esc)) {
				s--;	/* forget escape */
				cnt = 1;
			}
		}

		if (s == 0 && buf != NULL)
			continue;

		if ((cp = realloc(buf, len + s + 1)) == NULL) {
			free(buf);
			return NULL;
		}
		buf = cp;

		(void) memcpy(buf + len, ptr, s);
		len += s;
		buf[len] = '\0';
	}

	if ((flags & FPARSELN_UNESCALL) != 0 && esc && buf != NULL &&
	    strchr(buf, esc) != NULL) {
		ptr = cp = buf;
		while (cp[0] != '\0') {
			int skipesc;

			while (cp[0] != '\0' && cp[0] != esc)
				*ptr++ = *cp++;
			if (cp[0] == '\0' || cp[1] == '\0')
				break;

			skipesc = 0;
			if (cp[1] == com)
				skipesc += (flags & FPARSELN_UNESCCOMM);
			if (cp[1] == con)
				skipesc += (flags & FPARSELN_UNESCCONT);
			if (cp[1] == esc)
				skipesc += (flags & FPARSELN_UNESCESC);
			if (cp[1] != com && cp[1] != con && cp[1] != esc)
				skipesc = (flags & FPARSELN_UNESCREST);

			if (skipesc)
				cp++;
			else
				*ptr++ = *cp++;
			*ptr++ = *cp++;
		}
		*ptr = '\0';
		len = strlen(buf);
	}

	if (size)
		*size = len;
	return buf;
}
