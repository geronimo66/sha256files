/****************************************************************************************************************************
 * Copyleft (c) 2022 by Marco Gerodetti                                                                                     *
 *                                                                                                                          *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public        *
 * License as published by the Free Software Foundation, version 2. This program is distributed WITHOUT ANY WARRANTY;       *
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public     *
 * License for more details <http://www.gnu.org/licenses/gpl-2.0.html>.                                                     *
 *                                                                                                                          *
 *   written in simple C++, POSIX compatibility for Environmental requirements, alternativ for windows to                   *
 *   using c++ ISO/IEC 14882:2011, POSIX c-libs IEEE Std 1003.1, https://pubs.opengroup.org/onlinepubs/9699919799/).        *
 *                                                                                                                          *
 * design paradigms:                                                                                                        *
 *   POSIX libs & environment using              => high compatibility, no 3rd party libs                                   *
 *   Few Sourcefile & libs                       => easy to handle, few complexity                                          *
 ****************************************************************************************************************************/

//  Unpublished Version, NOT To USE

#ifndef io_hpp
#define io_hpp

#include <stdio.h>
#include "base.hpp"
#include "sha256.hpp"

VERSION( io_hpp, 0, 2, 0, 3);

#ifndef _WIN32 // UNIX/LINUX
    #include <sys/socket.h>
    #include <netinet/in.h>
    #define GetLastNetworkError errno
    #define __MSG_TO_WAIT MSG_WAITALL
    #define SOCKET int
#else // Windows
    #include <Ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    #define GetLastNetworkError WSAGetLastError()
    #define __MSG_TO_WAIT 0
    #define SHUT_RDWR 2
    typedef u64 off_t;
#endif


/* IP Socket handling                                                                              */
class SocketAdress : Independent {
private:
    struct sockaddr_in sock = {0};
public:
    SocketAdress() noexcept {
        memClean( sock);
        sock.sin_family = AF_INET;
        sock.sin_port = 0;
        sock.sin_addr.s_addr = 0;
    }
    SocketAdress(const IP_Def& ai) noexcept : SocketAdress() {
        sock.sin_port = htons(ai.m_port);
        sock.sin_addr.s_addr = ai.m_ip.m_v;
    }
    socklen_t addressLen = sizeof(sock);
    sockaddr* address = (sockaddr*)&sock;
};

template<typename T> class PipeEndRx {
protected:
    virtual const u64 doReadFromPipe(ArrayIndex<T> &buffer) = 0;
public:
    virtual const u64 readFromPipe(DArray<T> &buffer) {
        buffer.reset();
        const u64 count_readen = doReadFromPipe(buffer);
        return buffer.update_contend_end(count_readen);
    }
};

template<typename T> class PipeEndTx {
public:
    virtual const void writeToPipe(ArrayIndex<T> buffer) = 0;
};

class PipeEndUdp {
protected:
    PipeEndUdp(IP localAdress) : local(*localAdress) { }
    ~PipeEndUdp() { Close(); }

    mutable volatile SOCKET privateSocket = 0;

    SocketAdress const local;

    bool Open() const {
        if (privateSocket > 0)
            return false;
        privateSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (privateSocket <= 0)
            throw new _Exception(GetLastNetworkError, "Socket Error");
        return true;
    }

    void Close() const noexcept {
        if (privateSocket > 0)
            shutdown(privateSocket, SHUT_RDWR);
        privateSocket = 0;
    }
};

template<typename T> class PipeEndUdpRx : public PipeEndRx<T>, public PipeEndUdp {
private:
    const bool& m_stillSocketOpen;

    mutable SocketAdress lastSeenRemoteAdr;

    void Bind() const {
        if (this->Open())
            if (bind(this->privateSocket, this->local.address, this->local.addressLen) < 0)
                throw new _Exception(GetLastNetworkError, "IO Error");
    }
protected:
    const u64 doReadFromPipe(ArrayIndex<T> &buffer) final {
        const u64 bytes = narrow::to_u64(( buffer.end() - buffer.begin()) * sizeof(T));
        if (!this->privateSocket)
            Bind();
        const u64 gotBytes = (
            recvfrom(privateSocket,
                buffer.begin(),
                bytes,
                __MSG_TO_WAIT,
                lastSeenRemoteAdr.address,
                &lastSeenRemoteAdr.addressLen)
            );
        if (!m_stillSocketOpen)
            this->Close();
        return gotBytes / sizeof(T);
    }
public:
    PipeEndUdpRx(IP localAdress, bool& _stillOpen)
        : PipeEndUdp(localAdress)
        , m_stillSocketOpen(_stillOpen) {}
};

template<typename T> class PipeEndUdpTx : public PipeEndTx<T>, public PipeEndUdp {
private:
    const SocketAdress remote;
public:
    PipeEndUdpTx(IP localAdress, IP remoteAdress)
        : PipeEndUdp(localAdress)
        , remote(*remoteAdress) {}
    const void
        writeToPipe(ArrayIndex<T> buffer) final {
        this->Open();
        const size_t bytes = (buffer.end() - buffer.begin()) * sizeof(T);
        if ( sendto(this->privateSocket,
            buffer.begin(),
            bytes,
            0,
            remote.address,
            remote.addressLen)
            < 1)
            throw new _Exception(GetLastNetworkError, "ErrPoint200005");
    }
};


class File : Independent {
protected:
    bool file_ptr_is_foreign;
    FILE* f = nullptr;
public:
    File() noexcept : file_ptr_is_foreign(false), f(nullptr) { }
    File(const char* file_name, const char* mode) : file_ptr_is_foreign(false), f(nullptr) { open(file_name, mode); }
    File(FILE* p_f)                                    noexcept : file_ptr_is_foreign(true), f(p_f) { }
    File(const File& p_f)                              noexcept : file_ptr_is_foreign(true), f(p_f.f) { }
    int open(const char* file_name, const char* mode, bool throwErrors = true) {
        close();
        if ( file_ptr_is_foreign)
            throw new _Exception(9999, "internal error, using foreign file handler");
#pragma warning(suppress : 4996)
        f = fopen(file_name, mode);
        if (throwErrors && errno)
            throw new _Exception( errno, file_name);
        return errno;
    }
    void close() {
        if ( !file_ptr_is_foreign && f)
            fclose(f);
    }
    ~File() {
        if (!file_ptr_is_foreign)
            close();
    }
    inline bool is_open()                               const noexcept { return f != nullptr; }
    inline auto getc()                                  const noexcept { return ::fgetc(f); }
    inline auto eof()                                   const noexcept { return ::feof(f); }
    inline auto tell()                                  const noexcept { return ::ftell(f); }
    inline u64  seek(off_t offset, int whence)          const noexcept { return ::fseek(f, offset, whence); }
    inline auto putc(const char c)                      const noexcept { return ::fputc(c, f); }
    inline auto puts(const char * ca)                   const noexcept { return ::fputs(ca, f); }
};

class PipeEndFile : protected File {
protected:
    PipeEndFile(const char* file_name, const char* mode) : File(file_name, mode) {}
    PipeEndFile(const File &f) : File(f) {}
};

template<typename T> class PipeEndFileRx : public PipeEndRx<T>, public PipeEndFile {
protected:
    const u64 doReadFromPipe(ArrayIndex<T> &buffer) final {
        const u64 count = narrow::to_u64( buffer.end() - buffer.begin());
        const u64 readenCount = narrow::to_u64( fread(
            buffer.begin(),
            sizeof(T),
            count,
            this->f));
        return sizeof(T) * readenCount;
    }
public:
    PipeEndFileRx(const char* file_name) : PipeEndFile(file_name, "wb") {}
    PipeEndFileRx(const File &f) : PipeEndFile(f) {}
};

template<typename T> class PipeEndFileTx : public PipeEndTx<T>, public PipeEndFile {
public:
    PipeEndFileTx(const char* file_name) : PipeEndFile(file_name, "rb") {}
    PipeEndFileTx(const File &f) : PipeEndFile(f) {}
    const void
        writeToPipe(ArrayIndex<T> buffer) final {
        const size_t count = buffer.end() - buffer.begin();
        fwrite(
            buffer.begin(),
            sizeof(T),
            count,
            this->f);
    }
};

Sha256& operator << (Sha256& dest, const File &f) {
    DArrayContainer<u8, 64> m_ar;
    PipeEndFileRx<u8> file(f);
    do {
        file.readFromPipe( m_ar);
        dest.add_block(m_ar.reader());
    } while (m_ar.past_count() > 0);
    return dest;
}

/* description:    Executes an shell-like program and catches them stdout stream
    return value:   none
    error:           error Code                                                                                      */
template<size_t SIZE1> int
extShellProcessCatchingOutput(DString &outStream, const char(&cmd)[SIZE1]) {
    errno = 0;
    if (FILE* executeingTool = popen(cmd, "r")) {
        outStream << '>' << cmd << ':' << executeingTool;
        pclose(executeingTool);
        errno = 0;
    }
    return errno;
}

#endif
