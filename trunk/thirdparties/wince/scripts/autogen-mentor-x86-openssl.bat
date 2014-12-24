; COPY/PASTE in Terminal
; OpenSSL 1.0.1e
; Run in Visual Studio 2008 Command Prompt
; "error C2220: warning treated as error" -> Edit "ce.mak", remove "/WX" from CFLAGS then run nmake -f "ms/ce.mak"
perl configure VC-CE

set OSVERSION=WCE700
set TARGETCPU=X86
set PLATFORM=VC-CE
set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ce7\include;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ce\include;C:\Program Files (x86)\Windows CE Tools\SDKs\GE vPC WEC7 SDK\Include\X86
set LIB=C:\Program Files (x86)\Windows CE Tools\SDKs\GE vPC WEC7 SDK\Lib\x86
set WCECOMPAT=C:\tmp\wcecompat

ms/do_ms

nmake -f "ms/ce.mak" clean
nmake -f "ms/ce.mak"