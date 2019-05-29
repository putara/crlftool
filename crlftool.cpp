#if 0
crlftool.exe: crlftool.obj
 link /nologo /dynamicbase:no /ltcg /machine:x86 /map /merge:.rdata=.text /nxcompat /opt:icf /opt:ref /release /subsystem:console /out:crlftool.exe crlftool.obj kernel32.lib user32.lib
 @-peman.exe /vwsc crlftool.exe

crlftool.obj: crlftool.cpp
 cl /nologo /c /GF /GL /GR- /GS- /Gy /MD /O1ib1 /W4 /WX /Zl /fp:fast /D "_STATIC_CPPLIB" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" crlftool.cpp

!IF 0
#else

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRTDBG_MAP_ALLOC

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>

#define for         if (0); else for

#ifdef _MSC_VER
#define INLINE      __forceinline
#define NOINLINE    __declspec(noinline)
#else
#define INLINE      inline
#define NOINLINE
#endif

#define TARGET_WIN32GUI

#if defined(_WINDOWS) && defined(TARGET_WIN32GUI)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#else
#undef TARGET_WIN32GUI
#endif


#ifdef _MSC_VER
#include <tchar.h>
#include <crtdbg.h>
#include <intrin.h>

#ifdef TARGET_WIN32GUI
#pragma comment(linker, "/subsystem:windows")
#pragma comment(linker, "/defaultlib:shlwapi.lib")
#pragma comment(linker, "/defaultlib:shell32.lib")
#else
#pragma comment(linker, "/defaultlib:msvcrt.lib")
#pragma comment(linker, "/defaultlib:oldnames.lib")
#endif

#pragma warning(disable: 4127 4480)
#else
typedef char        _TCHAR;
#define _T(x)       (x)
#endif


#ifdef TARGET_WIN32GUI
template <typename T>
inline T* __cdecl MemAlloc(size_t num) throw()
{
    _ASSERT(SIZE_MAX / sizeof(T) >= num);
    return static_cast<T*>(::LocalAlloc(LPTR, sizeof(T) * num));
}
inline void __cdecl MemFree(__inout_opt void *p) throw()
{
    ::LocalFree(p);
}
#else
template <typename T>
inline T* __cdecl MemAlloc(size_t num) throw()
{
    _ASSERT(SIZE_MAX / sizeof(T) >= num);
    return static_cast<T*>(::calloc(num, sizeof(T)));
}
inline void __cdecl MemFree(__inout_opt void *p) throw()
{
    ::free(p);
}
#endif  //SUPPORT_WINALLOC

#ifdef _DEBUG
#define trace(...)              Debug::Trace(__VA_ARGS__)
#else
#define trace(...)
#endif

namespace Debug
{
void Trace(__in __format_string const char* format, ...);
}

template <typename T> inline T Min(T x, T y) { return x < y ? x : y; }
template <typename T> inline T Max(T x, T y) { return x > y ? x : y; }

template <class T>
void IUnknown_SafeAssign(T*& dst, T* src) throw()
{
    if (src != NULL) {
        src->AddRef();
    }
    if (dst != NULL) {
        dst->Release();
    }
    dst = src;
}

template <class T>
void IUnknown_SafeRelease(T*& unknown) throw()
{
    if (unknown != NULL) {
        unknown->Release();
        unknown = NULL;
    }
}

HRESULT StreamRead(__in IStream* stream, __out_bcount_full(cb) void* pv, __in ULONG cb) throw()
{
    ULONG cbRead = 0;
    HRESULT hr = stream->Read(pv, cb, &cbRead);
    if (SUCCEEDED(hr)) {
        hr = (cb == cbRead) ? S_OK : E_FAIL;
    }
    return hr;
}

HRESULT StreamWrite(__in IStream* stream, __in_bcount(cb) const void* pv, __in ULONG cb) throw()
{
    ULONG cbWritten = 0;
    HRESULT hr = stream->Write(pv, cb, &cbWritten);
    if (SUCCEEDED(hr)) {
        hr = (cb == cbWritten) ? S_OK : E_FAIL;
    }
    return hr;
}

HRESULT StreamSeek(__in IStream* stream, int dist, int origin, __out_opt ULONGLONG* newPos) throw()
{
    LARGE_INTEGER x;
    x.QuadPart = dist;
    return stream->Seek(x, origin, reinterpret_cast<ULARGE_INTEGER*>(newPos));
}

HRESULT StreamReset(__in IStream* stream) throw()
{
    return StreamSeek(stream, 0, STREAM_SEEK_SET, NULL);
}

const size_t MAX_BUFFER = 512;

HRESULT ConvertCrLfToLfU(IStream* src, IStream* dst)
{
    char buffer[MAX_BUFFER];
    HRESULT hr = S_FALSE;
    ULONG cb = 0;
    while (SUCCEEDED(hr = src->Read(buffer, MAX_BUFFER, &cb)) && cb > 0) {
        if (cb > 1 && buffer[cb - 1] == '\r') {
            cb--;
            LARGE_INTEGER x;
            x.QuadPart = -1;
            StreamSeek(src, -1, STREAM_SEEK_CUR, NULL);
        }
        const char* p = buffer;
        char* q = buffer;
        ULONG c = cb;
        while (c-- > 0) {
            if (*p == '\r' && *(p + 1) == '\n') {
                p++;
                continue;
            }
            *q++ = *p++;
        }
        cb = static_cast<ULONG>(q - buffer);
        if (FAILED(hr = StreamWrite(dst, buffer, cb))) {
            return hr;
        }
    }
    return S_OK;
}

HRESULT ConvertLfToCrLfU(IStream* src, IStream* dst)
{
    char srcbuffer[MAX_BUFFER];
    char dstbuffer[MAX_BUFFER * 2];
    ULONG cb;
    HRESULT hr;
    while (SUCCEEDED(hr = src->Read(srcbuffer, MAX_BUFFER, &cb)) && cb > 0) {
        const char* p = srcbuffer;
        char* q = dstbuffer;
        ULONG c = cb;
        bool cr = false;
        while (c-- > 0) {
            const char ch = *p++;
            if (cr == false && ch == '\n') {
                *q++ = '\r';
            }
            *q++ = ch;
            cr = ch == '\r';
        }
        cb = static_cast<uint32_t>(q - dstbuffer);
        if (FAILED(hr = StreamWrite(dst, dstbuffer, cb))) {
            return hr;
        }
    }
    return S_OK;
}

HRESULT AddNewLineU(IStream* src, IStream* dst)
{
    char buffer[MAX_BUFFER];
    HRESULT hr = S_FALSE;
    ULONG cb = 0;
    int crlf = -1;
    while (SUCCEEDED(hr = src->Read(buffer, MAX_BUFFER, &cb)) && cb > 0) {
        if (crlf < 0) {
            if (cb > 1 && buffer[cb - 1] == '\r') {
                cb--;
                LARGE_INTEGER x;
                x.QuadPart = -1;
                src->Seek(x, STREAM_SEEK_CUR, NULL);
            }
            const char* p = buffer;
            ULONG c = cb;
            while (c-- > 0) {
                if (*p == '\r' && *(p + 1) == '\n') {
                    crlf = 1;
                    break;
                }
                if (*p == '\n') {
                    crlf = 0;
                    break;
                }
                p++;
            }
        }
        if (FAILED(hr = StreamWrite(dst, buffer, cb))) {
            return hr;
        }
    }
    if (crlf < 0) {
        crlf = 0;
    }
    if (crlf == 1) {
        hr = StreamSeek(src, -2, STREAM_SEEK_END, NULL);
        if (SUCCEEDED(hr)) {
            hr = StreamRead(src, buffer, 2);
        }
        if (SUCCEEDED(hr)) {
            if (buffer[0] != '\r' || buffer[1] != '\n') {
                hr = ::IStream_Write(dst, "\r\n", 2);
            }
        }
    } else {
        hr = StreamSeek(src, -1, STREAM_SEEK_END, NULL);
        if (SUCCEEDED(hr)) {
            hr = StreamRead(src, buffer, 1);
        }
        if (SUCCEEDED(hr)) {
            if (buffer[0] != '\r') {
                hr = ::IStream_Write(dst, "\n", 1);
            }
        }
    }
    return hr;
}

HRESULT RemoveNewLineU(IStream* src, IStream* dst)
{
    char buffer[MAX_BUFFER];
    HRESULT hr = S_FALSE;
    ULONG cb = 0;
    ULONGLONG cbWritten = 0;
    int crlf = -1;
    bool atline = false;
    while (SUCCEEDED(hr = src->Read(buffer, MAX_BUFFER, &cb)) && cb > 0) {
        if (crlf < 0) {
            if (cb > 1 && buffer[cb - 1] == '\r') {
                cb--;
                LARGE_INTEGER x;
                x.QuadPart = -1;
                src->Seek(x, STREAM_SEEK_CUR, NULL);
            }
            const char* p = buffer;
            ULONG c = cb;
            while (c-- > 0) {
                if (*p == '\r' && *(p + 1) == '\n') {
                    crlf = 1;
                    break;
                }
                if (*p == '\n') {
                    crlf = 0;
                    break;
                }
                p++;
            }
        }
        if (crlf >= 0) {
            if (atline) {
                if (crlf == 1) {
                    if (FAILED(hr = StreamWrite(dst, "\r\n", 2))) {
                        return hr;
                    }
                    cbWritten += 2;
                } else {
                    if (FAILED(hr = StreamWrite(dst, "\n", 1))) {
                        return hr;
                    }
                    cbWritten += 1;
                }
                atline = false;
            }
            if (crlf == 1) {
                if (cb >= 2 && buffer[cb - 2] == '\r' && buffer[cb - 1] == '\n') {
                    cb -= 2;
                    atline = true;
                } else if (buffer[cb - 1] == '\r') {
                    cb -= 1;
                    LARGE_INTEGER x;
                    x.QuadPart = -1;
                    src->Seek(x, STREAM_SEEK_CUR, NULL);
                }
            } else {
                if (buffer[cb - 1] == '\n') {
                    cb -= 1;
                    atline = true;
                }
            }
        }
        if (FAILED(hr = StreamWrite(dst, buffer, cb))) {
            return hr;
        }
        cbWritten += cb;
    }
    return hr;
}

#ifdef TARGET_WIN32GUI
#define wcschr StrChrW
#endif

#include "option.hpp"

#ifdef TARGET_WIN32GUI

void message(__in const wchar_t* msg)
{
    ::MessageBoxW(NULL, msg, L"CRLF Tool", MB_OK);
}

void error(__in __format_string const wchar_t* format, ...)
{
    wchar_t buffer[1025];
    va_list argPtr;
    va_start(argPtr, format);
    ::wvnsprintfW(buffer, ARRAYSIZE(buffer), format, argPtr);
    ::MessageBoxW(NULL, buffer, L"CRLF Tool", MB_OK | MB_ICONEXCLAMATION);
    va_end(argPtr);
}

#else

void message(__in const wchar_t* msg)
{
    ::wprintf(L"%s\n", msg);
}

void error(__in __format_string const wchar_t* format, ...)
{
    va_list argPtr;
    va_start(argPtr, format);
    ::vwprintf(format, argPtr);
    va_end(argPtr);
}

#endif

void usage()
{
    message(L"crlftool.exe [-c|-l|-a|-r] infile.ext");
}

int Main(int argc, __in_ecount(argc + 1) const wchar_t* const* argv)
{
    option opt;
    int cmd = 0;
    for (int ch; (ch = opt.getopt(argc, argv, L"clar")) != -1; ) {
        switch (ch) {
        case L'c':
            cmd = 1;
            break;
        case L'l':
            cmd = 2;
            break;
        case L'a':
            cmd = 3;
            break;
        case L'r':
            cmd = 4;
            break;
        case L'?':
        default:
            usage();
            return 1;
        }
    }
    if (opt.optind >= argc || cmd == 0) {
        usage();
        return 1;
    }

    wchar_t infile[MAX_PATH];
    wchar_t* p = NULL;
    DWORD ret = ::GetFullPathNameW(argv[opt.optind], ARRAYSIZE(infile), infile, &p);
    if (ret == 0 || ret >= ARRAYSIZE(infile)) {
        error(L"Cannot access file '%s', error %u", argv[opt.optind], ::GetLastError());
        return 2;
    }
    if (p > infile) {
        p--;
    }
    *p = 0;
    wchar_t outfile[MAX_PATH];
    ret = ::GetTempFileNameW(infile, L"~crt", 0, outfile);
    if (ret == 0) {
        error(L"Cannot create a temporary file, error %u", ::GetLastError());
        return 2;
    }
    *p = L'\\';

    IStream* instream = NULL;
    IStream* outstream = NULL;
    HRESULT hr;
    if (FAILED(hr = ::SHCreateStreamOnFileW(infile, STGM_READ | STGM_SHARE_DENY_WRITE, &instream))) {
        error(L"Cannot open file '%s'\n", infile);
        ::DeleteFileW(outfile);
        return 3;
    }
    if (FAILED(hr = ::SHCreateStreamOnFileEx(outfile, STGM_WRITE | STGM_CREATE | STGM_SHARE_DENY_WRITE, FILE_ATTRIBUTE_NORMAL, TRUE, NULL, &outstream))) {
        error(L"Cannot create a temporary file, error 0x%08lX", hr);
        IUnknown_SafeRelease(instream);
        ::DeleteFileW(outfile);
        return 3;
    }
    switch (cmd) {
    case 1:
        hr = ConvertLfToCrLfU(instream, outstream);
        break;
    case 2:
        hr = ConvertCrLfToLfU(instream, outstream);
        break;
    case 3:
        hr = AddNewLineU(instream, outstream);
        break;
    case 4:
        hr = RemoveNewLineU(instream, outstream);
        break;
    }
    IUnknown_SafeRelease(outstream);
    IUnknown_SafeRelease(instream);
    if (SUCCEEDED(hr)) {
        BOOL result = ::ReplaceFileW(infile, outfile, NULL, REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, NULL, NULL);
        if (result == FALSE) {
            error(L"Cannot overwrite file, error %u", ::GetLastError());
            ::DeleteFileW(outfile);
            return 6;
        }
    } else {
        error(L"Something's gone heywire, error 0x%08lX", hr);
        ::DeleteFileW(outfile);
        return 5;
    }
    return 0;
}

#ifdef TARGET_WIN32GUI

DECLSPEC_NORETURN void entry()
{
    int argc = 0;
    const wchar_t* const* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argv == NULL) {
        ::ExitProcess(~0U);
    } else {
        ::ExitProcess(Main(argc, argv));
    }
}

#ifdef _MSC_VER
#pragma comment(linker, "/entry:entry")
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    entry();
}
#endif

#else

int wmain(int argc, wchar_t** argv)
{
    return Main(argc, argv);
}

#endif

namespace Debug
{

void Trace(__in __format_string const char* format, ...)
{
    char buffer[1025];
    va_list argPtr;
    va_start(argPtr, format);
#ifdef TARGET_WIN32GUI
    wvsprintfA(buffer, format, argPtr);
    OutputDebugStringA(buffer);
#else
    vsprintf(buffer, format, argPtr);
    printf("DBG: %s\n", buffer);
#endif
    va_end(argPtr);
}

}


#endif /*
!ENDIF
#*/
