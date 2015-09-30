/*	$NetBSD: xdr.h,v 1.22 2005/02/03 04:39:33 perry Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 *	from: @(#)xdr.h 1.19 87/04/22 SMI
 *	@(#)xdr.h	2.2 88/07/29 4.0 RPCSRC
 */

/*
 * xdr.h, External Data Representation Serialization Routines.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifndef _RPC_XDR_H_
#define _RPC_XDR_H_
#include "xdr_porting_layer.h"
/*
 * XDR provides a conventional way for converting between C data
 * types and an external bit-string representation.  Library supplied
 * routines provide for the conversion on built-in C data types.  These
 * routines and utility routines defined here are used to help implement
 * a type encode/decode routine for each user-defined type.
 *
 * Each data type provides a single procedure which takes two arguments:
 *
 *	bool_t
 *	xdrproc(xdrs, argresp)
 *		XDR *xdrs;
 *		<type> *argresp;
 *
 * xdrs is an instance of a XDR handle, to which or from which the data
 * type is to be converted.  argresp is a pointer to the structure to be
 * converted.  The XDR handle contains an operation field which indicates
 * which of the operations (ENCODE, DECODE * or FREE) is to be performed.
 *
 * XDR_DECODE may allocate space if the pointer argresp is null.  This
 * data can be freed with the XDR_FREE operation.
 *
 * We write only one procedure per data type to make it easy
 * to keep the encode and decode procedures for a data type consistent.
 * In many cases the same code performs all operations on a user defined type,
 * because all the hard work is done in the component type routines.
 * decode as a series of calls on the nested data types.
 */

/*
 * Xdr operations.  XDR_ENCODE causes the type to be encoded into the
 * stream.  XDR_DECODE causes the type to be extracted from the stream.
 * XDR_FREE can be used to release the space allocated by an XDR_DECODE
 * request.
 */
enum xdr_op {
	XDR_ENCODE = 0,
	XDR_DECODE = 1,
	XDR_FREE = 2
};

/*
 * This is the number of bytes per unit of external data.
 */
#define BYTES_PER_XDR_UNIT	(4)
#define RNDUP(x)  ((((x) + BYTES_PER_XDR_UNIT - 1) / BYTES_PER_XDR_UNIT) \
		    * BYTES_PER_XDR_UNIT)

/*
 * The XDR handle.
 * Contains operation which is being applied to the stream,
 * an operations vector for the paticular implementation (e.g. see xdr_mem.c),
 * and two private fields for the use of the particular impelementation.
 */
typedef struct __rpc_xdr {
	enum xdr_op x_op;	/* operation; fast additional param */
	const struct xdr_ops {
		/* get a long from underlying stream */
		bool_t(*x_getlong) (struct __rpc_xdr *, long *);
		/* put a long to " */
		bool_t(*x_putlong) (struct __rpc_xdr *, const long *);
		/* get some bytes from " */
		bool_t(*x_getbytes) (struct __rpc_xdr *, char *, u_int);
		/* put some bytes to " */
		bool_t(*x_putbytes) (struct __rpc_xdr *, const char *, u_int);
		/* returns bytes off from beginning */
		u_int(*x_getpostn) (struct __rpc_xdr *);
		/* lets you reposition the stream */
		bool_t(*x_setpostn) (struct __rpc_xdr *, u_int);
		/* buf quick ptr to buffered data */
		int32_t *(*x_inline) (struct __rpc_xdr *, u_int);
		/* free privates of this xdr_stream */
		void (*x_destroy) (struct __rpc_xdr *);
		 bool_t(*x_control) (struct __rpc_xdr *, int, void *);

		void *(*x_alloc) (struct __rpc_xdr *, int);
		void (*x_free) (struct __rpc_xdr *, void *, int);

	} *x_ops;
	char *x_public;		/* users' data */
	void *x_private;	/* pointer to private data */
	char *x_base;		/* private used for position info */
	u_int x_handy;		/* extra private word */
	char *x_logbuffer;	/* Log buffer */
	u_int x_logsize;	/* Log buffer size */
	char *x_basiclogbuffer;	/* Log buffer */
	char *x_decodebuffer;	/* Log buffer */
	u_int x_decodeBufsize;	/* Log buffer size */
	char *x_decodeCurbuffer;	/* Log buffer */
	u_int x_decodeCurBufsize;	/* Log buffer size */
} XDR;

/*
 * A xdrproc_t exists for each data type which is to be encoded or decoded.
 *
 * The second argument to the xdrproc_t is a pointer to an opaque pointer.
 * The opaque pointer generally points to a structure of the data type
 * to be decoded.  If this pointer is 0, then the type routines should
 * allocate dynamic storage of the appropriate size and return it.
 *
 * XXX can't actually prototype it, because some take three args!!!
 */
typedef bool_t(*xdrproc_t) (XDR *, void *, ... /* XDR *, void *, u_int */);

typedef u_int(*capi2_proc_t) (void *reqRep, void *rsp);

typedef struct xdr_discrim *(*xdr_lookup) (enum_t dscm);

/*
 * Operations defined on a XDR handle
 *
 * XDR		*xdrs;
 * long		*longp;
 * char *	 addr;
 * u_int	 len;
 * u_int	 pos;
 */
#define XDR_GETLONG(xdrs, longp)			\
	(*(xdrs)->x_ops->x_getlong)(xdrs, longp)
#define xdr_getlong(xdrs, longp)			\
	(*(xdrs)->x_ops->x_getlong)(xdrs, longp)

#define XDR_PUTLONG(xdrs, longp)			\
	(*(xdrs)->x_ops->x_putlong)(xdrs, longp)
#define xdr_putlong(xdrs, longp)			\
	(*(xdrs)->x_ops->x_putlong)(xdrs, longp)

#define XDR_ALLOC(xdrs, sz)			\
	(xdrs->x_ops->x_alloc) ? \
	(*(xdrs)->x_ops->x_alloc)(xdrs, sz) : \
	mem_alloc(sz)

#define XDR_DEALLOC(xdrs, buf, sz)			\
	(xdrs->x_ops->x_free) ? \
	(*(xdrs)->x_ops->x_free)(xdrs, buf, sz) : \
	mem_free(buf, sz)

static __inline int xdr_getint32(XDR *xdrs, int32_t *ip)
{
	long l;

	if (!xdr_getlong(xdrs, &l))
		return 0;
	*ip = (int32_t) l;
	return 1;
}

static __inline int xdr_putint32(XDR *xdrs, int32_t *ip)
{
	long l;

	l = (long)*ip;
	return xdr_putlong(xdrs, &l);
}

#define XDR_GETINT32(xdrs, int32p)	xdr_getint32(xdrs, int32p)
#define XDR_PUTINT32(xdrs, int32p)	xdr_putint32(xdrs, int32p)

#define XDR_GETBYTES(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_getbytes)(xdrs, addr, len)
#define xdr_getbytes(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_getbytes)(xdrs, addr, len)

#define XDR_PUTBYTES(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_putbytes)(xdrs, addr, len)
#define xdr_putbytes(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_putbytes)(xdrs, addr, len)

#define XDR_GETPOS(xdrs)				\
	(*(xdrs)->x_ops->x_getpostn)(xdrs)
#define xdr_getpos(xdrs)				\
	(*(xdrs)->x_ops->x_getpostn)(xdrs)

#define XDR_SETPOS(xdrs, pos)				\
	(*(xdrs)->x_ops->x_setpostn)(xdrs, pos)
#define xdr_setpos(xdrs, pos)				\
	(*(xdrs)->x_ops->x_setpostn)(xdrs, pos)

#define	XDR_INLINE(xdrs, len)				\
	(*(xdrs)->x_ops->x_inline)(xdrs, len)
#define	xdr_inline(xdrs, len)				\
	(*(xdrs)->x_ops->x_inline)(xdrs, len)

#define	XDR_DESTROY(xdrs)				\
	if ((xdrs)->x_ops->x_destroy)			\
		(*(xdrs)->x_ops->x_destroy)(xdrs)
#define	xdr_destroy(xdrs)				\
	if ((xdrs)->x_ops->x_destroy)			\
		(*(xdrs)->x_ops->x_destroy)(xdrs)

#define XDR_CONTROL(xdrs, req, op)			\
	if ((xdrs)->x_ops->x_control)			\
		(*(xdrs)->x_ops->x_control)(xdrs, req, op)
#define xdr_control(xdrs, req, op) XDR_CONTROL(xdrs, req, op)

/*
 * Solaris strips the '_t' from these types -- not sure why.
 * But, let's be compatible.
 */
#define xdr_rpcvers(xdrs, versp) xdr_u_int32(xdrs, versp)
#define xdr_rpcprog(xdrs, progp) xdr_u_int32(xdrs, progp)
#define xdr_rpcproc(xdrs, procp) xdr_u_int32(xdrs, procp)
#define xdr_rpcprot(xdrs, protp) xdr_u_int32(xdrs, protp)
#define xdr_rpcport(xdrs, portp) xdr_u_int32(xdrs, portp)

/*
 * Support struct for discriminated unions.
 * You create an array of xdrdiscrim structures, terminated with
 * a entry with a null procedure pointer.  The xdr_union routine gets
 * the discriminant value and then searches the array of structures
 * for a matching value.  If a match is found the associated xdr routine
 * is called to handle that part of the union.  If there is
 * no match, then a default routine may be called.
 * If there is no match and no default routine it is an error.
 */
#define NULL_xdrproc_t ((xdrproc_t)0)
struct xdr_discrim {
	int value;
	const char *debugStr;
	xdrproc_t proc;
	u_int unsize;
	capi2_proc_t capi2_proc;
	xdrproc_t ptr_proc;
	u_int32_t maxMsgSize;
};

/*
 * In-line routines for fast encode/decode of primitive data types.
 * Caveat emptor: these use single memory cycles to get the
 * data from the underlying buffer, and will fail to operate
 * properly if the data is not aligned.  The standard way to use these
 * is to say:
 *	if ((buf = XDR_INLINE(xdrs, count)) == NULL)
 *		return (0);
 *	<<< macro calls >>>
 * where ``count'' is the number of bytes of data occupied
 * by the primitive data types.
 *
 * N.B. and frozen for all time: each data type here uses 4 bytes
 * of external representation.
 */
#define IXDR_GET_INT32(buf)		((int32_t)ntohl((u_int32_t)*(buf)++))
#define IXDR_PUT_INT32(buf, v)	       (*(buf)++ = (int32_t)htonl((u_int32_t)v))
#define IXDR_GET_U_INT32(buf)		((u_int32_t)IXDR_GET_INT32(buf))
#define IXDR_PUT_U_INT32(buf, v)	IXDR_PUT_INT32((buf), ((int32_t)(v)))

#define IXDR_GET_LONG(buf)		((long)ntohl((u_int32_t)*(buf)++))
#define IXDR_PUT_LONG(buf, v)	       (*(buf)++ = (int32_t)htonl((u_int32_t)v))

#define IXDR_GET_BOOL(buf)		((bool_t)IXDR_GET_LONG(buf))
#define IXDR_GET_ENUM(buf, t)		((t)IXDR_GET_LONG(buf))
#define IXDR_GET_U_LONG(buf)		((u_long)IXDR_GET_LONG(buf))
#define IXDR_GET_SHORT(buf)		((short)IXDR_GET_LONG(buf))
#define IXDR_GET_U_SHORT(buf)		((u_short)IXDR_GET_LONG(buf))

#define IXDR_PUT_BOOL(buf, v)		IXDR_PUT_LONG((buf), (v))
#define IXDR_PUT_ENUM(buf, v)		IXDR_PUT_LONG((buf), (v))
#define IXDR_PUT_U_LONG(buf, v)		IXDR_PUT_LONG((buf), (v))
#define IXDR_PUT_SHORT(buf, v)		IXDR_PUT_LONG((buf), (v))
#define IXDR_PUT_U_SHORT(buf, v)	IXDR_PUT_LONG((buf), (v))

#define DEVELOPMENT_RPC_XDR_DETAIL_LOG

#define MAX_XDR_BASIC_LOG	128

#ifdef DEVELOPMENT_RPC_XDR_DETAIL_LOG
/*argument logging*/
bool_t xdr_u_char_dbg(XDR *xdrs, u_char *p, u_char *s);
bool_t xdr_u_int16_dbg(XDR *xdrs, u_int16_t *p, u_char *s);
bool_t xdr_u_long_dbg(XDR *xdrs, u_long *p, u_char *s);
extern void xdrmem_log(XDR *xdrs, char *str);
extern void xdrmem_log_start(XDR *xdrs, char *str);
extern void xdrmem_log_reset(XDR *xdrs);

#define _xdr_u_char(a, b, c)		xdr_u_char_dbg(a, b, (u_char *)c)
#define _xdr_u_int16_t(a, b, c)		xdr_u_int16_dbg(a, b, (u_char *)c)
#define _xdr_u_long(a, b, c)		xdr_u_long_dbg(a, b, (u_char *)c)

/*xdr log macros and functions*/
#define XDR_LOG(x, y)	xdrmem_log(x, y);
#define XDR_LOG_RESET(x, y)	xdrmem_log_start(x, y);

#else
#define _xdr_u_char(a, b, c)		xdr_u_char(a, b)
#define _xdr_u_int16_t(a, b, c)	xdr_u_int16_t(a, b)
#define XDR_LOG_RESET(x, y)
#define XDR_LOG(x, y)
#endif

typedef struct {
	unsigned int len;
	char *str;
} xdr_string_t;

bool_t xdr_xdr_string_t(XDR *xdrs, xdr_string_t *str);

/*
 * xdr_enum expects fixed enum_t and would not work for compiler optimised enums
 * Macro for serializing/deserialising enum as long and back to enum.
 */

/*
Usage:

XDR_ENUM_FUNC(enumType) : Call this macro in .c file ( defines the function implementation)
XDR_ENUM_DECLARE(enumType) : Declare the enum xdr function ( .h file )
XDR_ENUM_DEF(enumType) : Macro to use when need to pass fucntion pointer.
XDR_ENUM(xdrs, &enum_val, enumType) : Invoke xdr enum function call
*/

/* This macro expands into the enum specific xdr implementation*/
#ifdef DEVELOPMENT_RPC_XDR_DETAIL_LOG

#define XDR_ENUM_FUNC(en) \
	bool_t xdr ## _ ## en(XDR *x, en *v)	\
	{					\
		bool_t r;			\
		int i = *v;			\
		XDR_LOG(x, #en);		\
		r = xdr_int(x, &i);		\
		*v = (en)i;			\
		return r;			\
	}
#else

#define XDR_ENUM_FUNC(en) \
	bool_t xdr ## _ ## en(XDR *x, en *v)	\
	{					\
		bool_t r;			\
		int i = *v;			\
		r = xdr_int(x, &i);		\
		*v = (en)i;			\
		return r;			\
	}
#endif /* DEVELOPMENT_RPC_XDR_DETAIL_LOG */

/* This macros expands to enum function prototype*/
#define XDR_ENUM_DECLARE(en) \
	bool_t xdr ## _ ## en(XDR *x, en *v);

#define XDR_STRUCT_DECLARE	XDR_ENUM_DECLARE

/* This macros expands to function name */
#define XDR_ENUM_DEF(en) ((xdrproc_t)xdr ## _ ## en)

#define XDR_ENUM_ENTRY(en) ((xdrproc_t)xdr ## _ ## en , sizeof(en))

/*
 * usage:
 * XDR_ENUM(xdrs, &(name->alpha_coding), ALPHA_CODING_t);
 */
/* This macros expands to enum xdr function call */
#define XDR_ENUM(x, v, en) xdr ## _ ## en(x , v)

extern bool_t xdr_default_proc(XDR *, void *);

/*
 * These are the "generic" xdr routines.
 */
__BEGIN_DECLS extern bool_t xdr_void(void);
extern bool_t xdr_int(XDR *, int *);
extern bool_t xdr_u_int(XDR *, u_int *);
extern bool_t xdr_long(XDR *, long *);
extern bool_t xdr_u_long(XDR *, u_long *);
extern bool_t xdr_short(XDR *, short *);
extern bool_t xdr_u_short(XDR *, u_short *);
extern bool_t xdr_int16_t(XDR *, int16_t *);
extern bool_t xdr_u_int16_t(XDR *, u_int16_t *);
extern bool_t xdr_int32_t(XDR *, int32_t *);
extern bool_t xdr_u_int32_t(XDR *, u_int32_t *);
#ifdef _LONG_LONG_
extern bool_t xdr_int64_t(XDR *, int64_t *);
extern bool_t xdr_u_int64_t(XDR *, u_int64_t *);
#endif
extern bool_t xdr_bool(XDR *, bool_t *);
extern bool_t xdr_enum(XDR *, enum_t *);
extern bool_t xdr_array(XDR *, char **, u_int *, u_int, u_int, xdrproc_t);
extern bool_t xdr_bytes(XDR *, char **, u_int *, u_int);
extern bool_t xdr_opaque(XDR *, char *, u_int);
extern bool_t xdr_string(XDR *, char **, u_int);
extern bool_t xdr_union(XDR *, enum_t *, char *, const struct xdr_discrim *,
			xdrproc_t, struct xdr_discrim **, xdr_lookup);
extern bool_t xdr_char(XDR *, char *);
extern bool_t xdr_u_char(XDR *, u_char *);
extern bool_t xdr_vector(XDR *, char *, u_int, u_int, xdrproc_t);
extern bool_t xdr_double(XDR *, double *);
extern bool_t xdr_quadruple(XDR *, long double *);
extern bool_t xdr_reference(XDR *, char **, u_int, xdrproc_t);
extern bool_t xdr_pointer(XDR *, char **, u_int, xdrproc_t);

extern bool_t xdr_referenceEx(XDR *, char **, u_int, xdrproc_t, xdrproc_t);
extern bool_t xdr_pointerEx(XDR *, char **, u_int, xdrproc_t, xdrproc_t);

extern bool_t xdr_wrapstring(XDR *, char **);
extern void xdr_free(xdrproc_t, char *);
#ifdef _LONG_LONG_
extern bool_t xdr_hyper(XDR *, longlong_t *);
extern bool_t xdr_u_hyper(XDR *, u_longlong_t *);
extern bool_t xdr_longlong_t(XDR *, longlong_t *);
extern bool_t xdr_u_longlong_t(XDR *, u_longlong_t *);
#endif

typedef char *char_ptr_t;
typedef unsigned char *uchar_ptr_t;
bool_t xdr_uchar_ptr_t(XDR *xdrs, unsigned char **ptr);
bool_t xdr_char_ptr_t(XDR *xdrs, char **ptr);

#define xdr_Boolean		xdr_u_char
#define _xdr_Boolean		_xdr_u_char

#define xdr_u_int8_t		xdr_u_char
#define _xdr_u_int8_t		_xdr_u_char

#define xdr_UInt32		xdr_u_long
#define _xdr_UInt32		_xdr_u_long

#define xdr_Int32		xdr_long

#define xdr_UInt8		xdr_u_char
#define _xdr_UInt8		_xdr_u_char

#define xdr_Int8		xdr_char
#define _xdr_Int8(a, b, c)	xdr_char(a, b)

#define xdr_UInt16		xdr_u_int16_t
#define _xdr_UInt16		_xdr_u_int16_t

bool_t xdr_bitfields(register XDR *xdrs,	/* XDR stream pointer   */
		u_int num_fields,		/* Number of bit fields */
		u_char *widths,			/* Bit field widths     */
		u_long *values			/* Bit field values     */
    );
__END_DECLS

/*
 * Common opaque bytes objects used by many rpc protocols;
 * declared here due to commonality.
 */
#define MAX_NETOBJ_SZ 1024

struct netobj {
	u_int n_len;
	char *n_bytes;
};

typedef struct netobj netobj;
extern bool_t xdr_netobj(XDR *, struct netobj *);

/*
 * These are the public routines for the various implementations of
 * xdr streams.
 */
__BEGIN_DECLS
/* XDR using memory buffers */
extern void xdrmem_create(XDR *, char *, u_int, char *, u_int, enum xdr_op);

/* XDR using stdio library */
#ifdef _STDIO_H_
extern void xdrstdio_create(XDR *, FILE *, enum xdr_op);
#endif

/* XDR pseudo records for tcp */
extern void xdrrec_create(XDR *, u_int, u_int, char *,
			  int (*)(char *, char *, int),
			  int (*)(char *, char *, int));

/* make end of xdr record */
extern bool_t xdrrec_endofrecord(XDR *, int);

/* move to beginning of next record */
extern bool_t xdrrec_skiprecord(XDR *);

/* true if no more input */
extern bool_t xdrrec_eof(XDR *);
extern u_int xdrrec_readbytes(XDR *, caddr_t, u_int);
__END_DECLS

#endif /* !_RPC_XDR_H_ */
