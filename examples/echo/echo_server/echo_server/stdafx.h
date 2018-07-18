// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include <stdio.h>
#include <tchar.h>


#include <netpp/base/logging.h>
#include <netpp/net/protobuf/codec.h>
#include <netpp/net/protobuf/dispatcher.h>
#include <netpp/net/tcp_server.h>

#if _DEBUG
#pragma comment(lib, "../../../../netpp/lib/x86/netpp13d.lib")
#pragma comment(lib, "../../../../thirdparty/protobuf/lib/x86/libprotobufd.lib")
#pragma comment(lib, "../../../../thirdparty/protobuf/lib/x86/libprotobuf-lited.lib")
#else
#pragma comment(lib, "../../../../netpp/lib/x86/netpp13.lib")
#pragma comment(lib, "../../../../thirdparty/protobuf/lib/x86/libprotobuf.lib")
#pragma comment(lib, "../../../../thirdparty/protobuf/lib/x86/libprotobuf-lite.lib")
#endif



// TODO: reference additional headers your program requires here
