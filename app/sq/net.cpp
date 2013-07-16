/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <new.h>
#include <err.h>
#include <squirrel.h> 
#include <sqstdblob.h>
#include <debug.h>

#include <dev/class/netif.h>

#include <lwip/api.h>
#include <lwip/ip_addr.h>
#include <lwip/sockets.h>

#define LOCAL_TRACE 0

class Socket;

#define LK_SOCKET_TYPE_TAG        0x08040000
#define LK_SOCKADDR_TYPE_TAG      0x08020000

#define SETUP_SOCKET(v) \
	Socket *self = NULL; \
	if(SQ_FAILED(sq_getinstanceup(v, 1, (SQUserPointer*) &self, (SQUserPointer) LK_SOCKET_TYPE_TAG))) \
		return sq_throwerror(v, _SC("invalid type tag")); \
	if(!self) \
		return sq_throwerror(v, _SC("the socket is invalid"));

#define _DECL_SOCKET_FUNC(name, nparams, typecheck) {_SC(#name), _socket_##name, nparams, typecheck}

class Socket
{
	private:

	int fd;
	bool closed;

	public:

	Socket(int domain, int type, int protocol);
	~Socket(void);

	int accept(struct sockaddr *addr, socklen_t *addrlen);
	int bind(const struct sockaddr *name, socklen_t namelen);
	int shutdown(int how);
	int getpeername(struct sockaddr *name, socklen_t *namelen);
	int getsockname(struct sockaddr *name, socklen_t *namelen);
	int getsockopt(int level, int optname, void *optval, socklen_t *optlen);
	int setsockopt(int level, int optname, const void *optval, socklen_t optlen);
	int close(void);
	int connect(const struct sockaddr *name, socklen_t namelen);
	int listen(int backlog);
	int recv(void *mem, size_t len, int flags);
	int read(void *mem, size_t len);
	int recvfrom(void *mem, size_t len, int flags,
			struct sockaddr *from, socklen_t *fromlen);
	int send(const void *dataptr, size_t size, int flags);
	int sendto(const void *dataptr, size_t size, int flags,
			const struct sockaddr *to, socklen_t tolen);
	int write(const void *dataptr, size_t size);
	int ioctl(long cmd, void *argp);
	int fcntl(int cmd, int val);
};

Socket::Socket(int domain, int type, int protocol)
{
	fd = lwip_socket(domain, type, protocol);
	closed = false;
}

int Socket::connect(const struct sockaddr *name, socklen_t namelen)
{
	int ret = lwip_connect(fd, name, namelen);
	if (ret)
		return ret;

	int flags = lwip_fcntl(fd, F_GETFL, 0);
	lwip_fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	return ret;
}

int Socket::read(void *buf, size_t len)
{
	return lwip_read(fd, buf, len);
}

int Socket::write(const void *buf, size_t len)
{
	return lwip_write(fd, buf, len);
}

int Socket::close(void)
{
	if (!closed) {
		closed = true;
		return lwip_close(fd);
	}

	return 0;
}

Socket::~Socket(void)
{
	close();
}

static SQInteger _socket_releasehook(SQUserPointer p, SQInteger size)
{
	Socket *self = (Socket*) p;
	self->~Socket();

	sq_free(self, sizeof(Socket));
	return 1;
}

static SQInteger _socket_constructor(HSQUIRRELVM v)
{
	SQInteger domain, type, protocol;
	SQInteger nargs = sq_gettop(v);

	if (nargs >= 2)
		sq_getinteger(v, 2, &domain);
	else
		domain = PF_INET;

	if (nargs >= 3)
		sq_getinteger(v, 3, &type);
	else
		type = SOCK_STREAM;

	if (nargs >= 4)
		sq_getinteger(v, 4, &protocol);
	else
		protocol = IPPROTO_TCP;

	Socket *s = new (sq_malloc(sizeof(Socket)))
			Socket(domain, type, protocol);

	if (SQ_FAILED(sq_setinstanceup(v, 1, s))) {
		s->~Socket();
		sq_free(s, sizeof(Socket));

		return sq_throwerror(v, _SC("cannot create socket"));
	}

	sq_setreleasehook(v, 1, _socket_releasehook);
	return 0;
}

static SQInteger _socket__cloned(HSQUIRRELVM v)
{
	return sq_throwerror(v, _SC("this object cannot be cloned"));
}

static SQInteger _socket_connect(HSQUIRRELVM v)
{
	SETUP_SOCKET(v);

	const SQChar *hostname;
	SQInteger port;
	SQObjectType type;

	int res;
	err_t err;
	ip_addr_t ip_addr;
	struct sockaddr_in addr;	

	switch (sq_gettype(v, 2)) {
		case OT_TABLE:
			sq_pushstring(v, "addr", -1);
			sq_get(v, 2);

			if (sq_gettype(v, -1) != OT_STRING)
				return sq_throwerror(v, _SC("expected string value in table slot \"addr\""));

			sq_pushstring(v, "port", -1);
			sq_get(v, 2);

			type = sq_gettype(v, -1);
			if (type != OT_INTEGER && type != OT_FLOAT)
				return sq_throwerror(v, _SC("expected numeric value in table slot \"port\""));
			break;

		case OT_ARRAY:
			// TODO: check that array size == 2	

			sq_pushinteger(v, 0);
			sq_get(v, 2);

			if (sq_gettype(v, -1) != OT_STRING)
				return sq_throwerror(v, _SC("expected string value at array index 0"));

			sq_pushinteger(v, 1);
			sq_get(v, 2);

			type = sq_gettype(v, -1);
			if (type != OT_INTEGER && type != OT_FLOAT)
				return sq_throwerror(v, _SC("expected numeric value at array index 1"));
			break;

		default:
			// should never get here due to arg check
			break;
	}

	sq_getstring(v, -2, &hostname);
	sq_getinteger(v, -1, &port);

	LTRACEF("hostname=%s port=%u\n", hostname, port);

	err = netconn_gethostbyname(hostname, &ip_addr);
	if (err != ERR_OK) {
		sq_pushinteger(v, err);
		return 1;
	}

	LTRACEF("Resolved host to: %u.%u.%u.%u\n",
			ip4_addr1_16(&ip_addr),
			ip4_addr2_16(&ip_addr),
			ip4_addr3_16(&ip_addr),
			ip4_addr4_16(&ip_addr));

	addr.sin_family = AF_INET;
	addr.sin_len = sizeof(addr);
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = ip_addr.addr;

	sq_pop(v, 2); // hostname and port

	res = self->connect((struct sockaddr *) &addr, sizeof(addr));

	sq_pushinteger(v, res);
	return 1;
}

static SQInteger _socket_write(HSQUIRRELVM v)
{
	SETUP_SOCKET(v);

	SQInteger len;
	int res;

	switch (sq_gettype(v, 2)) {
		case OT_INSTANCE: {
			SQUserPointer ptr;

			sqstd_getblob(v, 2, &ptr);
			if (!ptr)
				return sq_throwerror(v, _SC("Blob user pointer is null."));

			len = sqstd_getblobsize(v, 2);

			res = self->write(ptr, len);
		}
		break;

		case OT_STRING: {
			const SQChar *str;

			sq_getstring(v, 2, &str);
			len = sq_getsize(v, 2);

			res = self->write(str, len);
		}
		break;

		default:
			return sq_throwerror(v, _SC("Unexpected object type."));
	}

	sq_pushinteger(v, res);
	return 1;
}

static SQInteger _socket_read(HSQUIRRELVM v)
{
	SETUP_SOCKET(v);

	SQUserPointer data;
	SQInteger size;
	int res;

	sq_getinteger(v, 2, &size);
	data = sq_getscratchpad(v, size);
	
	res = self->read(data, size);
	if (res <= 0) {
		TRACEF("res=%d\n", res);
		sq_pushinteger(v, res);
		return sq_throwobject(v);
	}

	sq_pushstring(v, (SQChar *) data, res);
	return 1;
}

static SQInteger _socket_close(HSQUIRRELVM v)
{
	SETUP_SOCKET(v);

	self->close();

	return 0;
}

static SQRegFunction _socket_methods[] = {
	_DECL_SOCKET_FUNC(constructor, -1, _SC("xnnn")),
	_DECL_SOCKET_FUNC(connect, 2, _SC("xt|a")),
	_DECL_SOCKET_FUNC(write, -2, _SC("xs|o")),
	_DECL_SOCKET_FUNC(read, 2, _SC("xn")),
	_DECL_SOCKET_FUNC(close, 1, _SC("x")),
	_DECL_SOCKET_FUNC(_cloned, 0, NULL),
	{0, 0, 0, 0}
};

static SQInteger _net_waitForNetwork(HSQUIRRELVM v)
{
	status_t res;
	SQInteger nargs;
	SQInteger timeout;

	nargs = sq_gettop(v);
	if (nargs == 2)
		sq_getinteger(v, 2, &timeout);
	else
		timeout = INFINITE_TIME;
	
	res = class_netstack_wait_for_network(timeout);

	sq_pushinteger(v, res);
	return 1;
}

#define _DECL_FUNC(name, nparams, pmask) {_SC(#name), _net_##name, nparams, pmask}
static SQRegFunction netlib_funcs[]={
#if 0
	_DECL_FUNC(update, 1, _SC(".")),
	_DECL_FUNC(updateRegion, 5, _SC(".nnnn")),
#endif
	_DECL_FUNC(waitForNetwork, -1, _SC(".n")),
	{0, 0, 0, 0}
};
#undef _DECL_FUNC

static void init_socketclass(HSQUIRRELVM v)
{
	sq_pushregistrytable(v);
	sq_pushstring(v, _SC("lk_socket"), -1);

	if (SQ_FAILED(sq_get(v, -2))) {
		sq_pushstring(v, _SC("lk_socket"), -1);

		sq_newclass(v, SQFalse);
		sq_settypetag(v, -1, (SQUserPointer) LK_SOCKET_TYPE_TAG);
		
		SQInteger i = 0;
		while (_socket_methods[i].name != 0) {
			SQRegFunction &f = _socket_methods[i];
			sq_pushstring(v, f.name, -1);
			sq_newclosure(v, f.f, 0);
			sq_setparamscheck(v, f.nparamscheck, f.typemask);
			sq_newslot(v, -3, SQFalse);
			i++;
		}

		sq_newslot(v, -3, SQFalse);

		sq_pushroottable(v);
		sq_pushstring(v, _SC("socket"), -1);
		sq_pushstring(v, _SC("lk_socket"), -1);
		sq_get(v, -4);
		sq_newslot(v, -3, SQFalse);
		sq_pop(v, 1);
	} else {
		sq_pop(v, 1); //result
	}

	sq_pop(v, 1);
}

#define _DECL_INT_CONSTANT(macro) \
	{ \
		sq_pushstring(v, _SC(#macro), -1); \
		sq_pushinteger(v, macro); \
		sq_newslot(v, -3, SQFalse); \
	}

extern "C"
SQInteger lk_register_netlib(HSQUIRRELVM v)
{
	init_socketclass(v);

	SQInteger i = 0;
	while (netlib_funcs[i].name != 0) {
		sq_pushstring(v, netlib_funcs[i].name, -1);
		sq_newclosure(v, netlib_funcs[i].f, 0);
		sq_setparamscheck(v, netlib_funcs[i].nparamscheck, netlib_funcs[i].typemask);
		sq_setnativeclosurename(v, -1, netlib_funcs[i].name);
		sq_newslot(v, -3, SQFalse);
		i++;
	}

	_DECL_INT_CONSTANT(SOCK_STREAM);
	_DECL_INT_CONSTANT(SOCK_DGRAM);
	_DECL_INT_CONSTANT(SOCK_RAW);

	_DECL_INT_CONSTANT(AF_UNSPEC);
	_DECL_INT_CONSTANT(AF_INET);
	_DECL_INT_CONSTANT(PF_INET);
	_DECL_INT_CONSTANT(PF_UNSPEC);

	_DECL_INT_CONSTANT(IPPROTO_IP);
	_DECL_INT_CONSTANT(IPPROTO_TCP);
	_DECL_INT_CONSTANT(IPPROTO_UDP);
	_DECL_INT_CONSTANT(IPPROTO_UDPLITE);

	_DECL_INT_CONSTANT(IPPROTO_UDPLITE);

	return SQ_OK;
}

