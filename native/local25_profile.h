#pragma once
// Explicit compatibility with lhr0909/stone-age's 2.5 server.
// VER25 alone still enables a private server's TEA and extended-login formats.
#ifndef _SA_VERSION_25
#error STONEAGE_LOCAL_25 requires the VER25 client configuration
#endif
#undef _SA_VERSION
#define _SA_VERSION 'L'
#undef _DEFAULT_PKEY
#undef _RUNNING_KEY
#define _DEFAULT_PKEY "cary"
#define _RUNNING_KEY "cary"
#undef _NEWNET_
#undef _NEW_CLIENT_LOGIN
#undef _VMP_
#undef _NODEBUG_
#ifdef STONEAGE_PROTOCOL_ONLY
#undef new
#undef _STONDEBUG_
#endif
