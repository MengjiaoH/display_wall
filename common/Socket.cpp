// ======================================================================== //
// Copyright 2019 Ingo Wald                                                 //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

/*! iw: note this code is literally copied (and then adapted) from
    embree's original common/sys/network.h, under following license */

// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "Socket.h"
#include <sys/types.h>
#ifndef WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif
#include <string>

////////////////////////////////////////////////////////////////////////////////
/// Platforms supporting Socket interface
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
//#include <io.h>
#include <winsock2.h>
typedef int socklen_t;
#define SHUT_RDWR 0x2
#else 
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h> 
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket ::close
#endif

/*! ignore if not supported */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0 
#endif

#define THROW_RUNTIME_ERROR(a) throw std::runtime_error(a)

namespace dw2 {
  namespace sock {

    void initialize() {
#ifdef _WIN32
      static bool initialized = false;
      static std::mutex initMutex;
      std::lock_guard<std::mutex> lock(initMutex);
      WSADATA wsaData;
      short version = MAKEWORD(1,1);
      if (WSAStartup(version,&wsaData) != 0)
        THROW_RUNTIME_ERROR("Winsock initialization failed");
      initialized = true;
#endif
    }

    struct buffered_socket_t 
    {
      buffered_socket_t (SOCKET fd, size_t isize = 64*1024, size_t osize = 64*1024)
        : fd(fd), 
          ibuf(new char[isize]), isize(isize), istart(0), iend(0),
          obuf(new char[osize]), osize(osize), oend(0) {
      }

      ~buffered_socket_t () {
        delete[] ibuf; ibuf = nullptr;
        delete[] obuf; obuf = nullptr;
      }
      
      SOCKET fd;               //!< file descriptor of the socket
      char* ibuf;
      size_t isize;
      size_t istart,iend;
      char* obuf;
      size_t osize;
      size_t oend;
    };

    struct AutoCloseSocket
    {
      SOCKET sock;
      AutoCloseSocket (SOCKET sock) : sock(sock) {}
      ~AutoCloseSocket () {
        if (sock != INVALID_SOCKET) {
          closesocket(sock);
        }
      }
    };

    int getPortOf(socket_t handle)
    {
      buffered_socket_t *socket = (buffered_socket_t *)handle;
      struct sockaddr_in sin;
      
      socklen_t len = sizeof(sin);
      if (getsockname(socket->fd, (struct sockaddr *)&sin, &len) == -1) {
        perror("getsockname");
        return -1;
      }
      else {
        return ntohs(sin.sin_port);
      }
    }
  
    SOCKET getFileDescriptor(socket_t sock)
    {
      buffered_socket_t* hsock = (buffered_socket_t*)sock;
      return hsock->fd;
    }

    socket_t connect(const char* host, unsigned short port) 
    {
      initialize();

      /*! create a new socket */
      SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd == INVALID_SOCKET) THROW_RUNTIME_ERROR("cannot create socket");
      AutoCloseSocket auto_close(sockfd);
      
      /*! perform DNS lookup */
      struct hostent* server = ::gethostbyname(host);
      if (server == nullptr) THROW_RUNTIME_ERROR("server "+std::string(host)+" not found");
      
      /*! perform connection */
      struct sockaddr_in serv_addr;
      memset((char*)&serv_addr, 0, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = (unsigned short) htons(port);
      memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
      
      if (::connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
        THROW_RUNTIME_ERROR("connection to "+std::string(host)+":"+std::to_string((long long)port)+" failed");

      /*! enable TCP_NODELAY */
#ifdef TCP_NODELAY
      { int flag = 1; ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(int)); }
#endif

      /*! we do not want SIGPIPE to be thrown */
#ifdef SO_NOSIGPIPE
      { int flag = 1; setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (const char*) &flag, sizeof(int)); }
#endif
      
      auto_close.sock = INVALID_SOCKET;
      buffered_socket_t *s = new buffered_socket_t(sockfd);
      return (socket_t)s;
    }
    
    socket_t bind(unsigned short port)
    {
      initialize();

      /*! create a new socket */
      SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd == INVALID_SOCKET) THROW_RUNTIME_ERROR("cannot create socket");
      AutoCloseSocket auto_close(sockfd);

      /* When the server completes, the server socket enters a time-wait state during which the local
      address and port used by the socket are believed to be in use by the OS. The wait state may
      last several minutes. This socket option allows bind() to reuse the port immediately. */
#ifdef SO_REUSEADDR
      { int flag = true; ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int)); }
#endif
      
      /*! bind socket to port */
      struct sockaddr_in serv_addr;
      memset((char *) &serv_addr, 0, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = (unsigned short) htons(port);
      serv_addr.sin_addr.s_addr = INADDR_ANY;

      if (::bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        THROW_RUNTIME_ERROR("binding to port "+std::to_string((long long)port)+" failed");
      
      /*! listen to port, up to 5 pending connections */
      if (::listen(sockfd,5) < 0)
        THROW_RUNTIME_ERROR("listening on socket failed");

      auto_close.sock = INVALID_SOCKET;
      buffered_socket_t *s = new buffered_socket_t(sockfd,serv_addr.sin_addr.s_addr);
      return (socket_t)s;
    }
    
    socket_t listen(socket_t hsock)
    {
      SOCKET sockfd = ((buffered_socket_t*) hsock)->fd;
            
      /*! accept incoming connection */
      struct sockaddr_in addr;
      socklen_t len = sizeof(addr);
      SOCKET fd = ::accept(sockfd, (struct sockaddr *) &addr, &len);
      if (fd == INVALID_SOCKET) THROW_RUNTIME_ERROR("cannot accept connection");

      /*! enable TCP_NODELAY */
#ifdef TCP_NODELAY
      { int flag = 1; ::setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(char*)&flag,sizeof(int)); }
#endif

      /*! we do not want SIGPIPE to be thrown */
#ifdef SO_NOSIGPIPE
      { int flag = 1; setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&flag, sizeof(int)); }
#endif

      return (socket_t) new buffered_socket_t(fd); 
    }
    
    void read(socket_t hsock_i, void* data_i, size_t bytes)
    {
      char* data = (char*)data_i;
      while (bytes) {
        size_t readBytes = read_some(hsock_i, data, bytes);
        data += readBytes;
        bytes -= readBytes;
      }
    }

    size_t read_some(socket_t hsock_i, void* data, const size_t bytes)
    {
      buffered_socket_t* hsock = (buffered_socket_t*) hsock_i;
#if BUFFERING
      if (hsock->istart == hsock->iend) {
        size_t n = ::recv(hsock->fd,hsock->ibuf,hsock->isize,MSG_NOSIGNAL);
        if      (n == 0) throw Disconnect();
        else if (n  < 0) THROW_RUNTIME_ERROR("error reading from socket");
        hsock->istart = 0;
        hsock->iend = n;
      }
      size_t bsize = hsock->iend-hsock->istart;
      if (bytes < bsize) bsize = bytes;
      memcpy(data,hsock->ibuf+hsock->istart,bsize);
      hsock->istart += bsize;
      return bsize;
#else
      size_t n = ::recv(hsock->fd,(char*)data,bytes,MSG_NOSIGNAL);
      if      (n == 0) throw Disconnect();
      else if (n  < 0) THROW_RUNTIME_ERROR("error reading from socket");
      return n;
#endif
    }

    void write(socket_t hsock_i, const void* data_i, size_t bytes)
    {
      buffered_socket_t* hsock = (buffered_socket_t*) hsock_i;
#if BUFFERING
      const char* data = (const char*) data_i;
      while (bytes) {
        if (hsock->oend == hsock->osize) flush(hsock_i);
        size_t bsize = hsock->osize-hsock->oend;
        if (bytes < bsize) bsize = bytes;
        memcpy(hsock->obuf+hsock->oend,data,bsize);
        data += bsize;
        hsock->oend += bsize;
        bytes -= bsize;
      }
#else
      ::send(hsock->fd, (char*)data_i,bytes,MSG_NOSIGNAL);
#endif
    }

    void flush(socket_t hsock_i)
    {
#if BUFFERING
      buffered_socket_t* hsock = (buffered_socket_t*) hsock_i;
      char* data = hsock->obuf;
      size_t bytes = hsock->oend;
      while (bytes > 0) {
        size_t n = ::send(hsock->fd,data,(int)bytes,MSG_NOSIGNAL);
        if (n < 0) THROW_RUNTIME_ERROR("error writing to socket");
        bytes -= n;
        data += n;
      } 
      hsock->oend = 0;
#endif
    }
    
    void close(socket_t hsock_i) {
      buffered_socket_t* hsock = (buffered_socket_t*) hsock_i;
      ::shutdown(hsock->fd,SHUT_RDWR);
      closesocket(hsock->fd);
      delete hsock;
    }

// ////////////////////////////////////////////////////////////////////////////////
// /// All Platforms
// ////////////////////////////////////////////////////////////////////////////////

//     bool read_bool(socket_t socket) 
//     {
//       bool value = 0;
//       read(socket,&value,sizeof(bool));
//       return value;
//     }

//     char read_char(socket_t socket) 
//     {
//       char value = 0;
//       read(socket,&value,sizeof(char));
//       return value;
//     }
    
//     int read_int(socket_t socket) 
//     {
//       int value = 0;
//       read(socket,&value,sizeof(int));
//       return value;
//     }
    
//     float read_float(socket_t socket) 
//     {
//       float value = 0.0f;
//       read(socket,&value,sizeof(float));
//       return value;
//     }
    
//     std::string read_string(socket_t socket) 
//     {
//       int bytes = read_int(socket);
//       char* str = new char[bytes+1];
//       read(socket,str,bytes);
//       str[bytes] = 0x00;
//       std::string s(str);
//       delete[] str;
//       return s;
//     }

//     void write(socket_t socket, bool value) {
//       write(socket,&value,sizeof(bool));
//     }

//     void write(socket_t socket, char value) {
//       write(socket,&value,sizeof(char));
//     }
    
//     void write(socket_t socket, int value) {
//       write(socket,&value,sizeof(int));
//     }
    
//     void write(socket_t socket, float value) {
//       write(socket,&value,sizeof(float));
//     }
    
//     void write(socket_t socket, const std::string& str) {
//       write(socket,(int)str.size());
//       write(socket,str.c_str(),str.size());
//     }
    
  }

  std::string getHostName()
  {
#if 0
    const static size_t BUF_SIZE = 1024;
    char buffer[BUF_SIZE];
    gethostname(buffer, BUF_SIZE);
    std::string host(buffer);
	  return host + ".sci.utah.edu";
#else
    // const static size_t BUF_SIZE = 1024;
    // char buffer[BUF_SIZE];
    // gethostname(buffer, BUF_SIZE);
    // std::string hostname(buffer);
    // hostname = hostname +".sci.utah.edu";
    
    // struct addrinfo hints, *infoptr; // So no need to use memset global variables
    
    // memset(&hints, 0, sizeof hints);
    // // hints.ai_family = AF_INET; // AF_INET means IPv4 only addresses
    // hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    // hints.ai_socktype = SOCK_STREAM;

    // int result = getaddrinfo(hostname.c_str(), NULL , &hints, &infoptr);
    // if (result) {
    //     fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
    //     exit(1);
    // }

    // struct addrinfo *p;
    // char host[256];

    // for (p = infoptr; p != NULL; p = p->ai_next) {
    //     getnameinfo(p->ai_addr, p->ai_addrlen, host, sizeof (host), NULL, 0, NI_NUMERICHOST);
    //     // puts(host);
    // }
    // std::string IP(host);

    // freeaddrinfo(infoptr);
    std::string ipAddress="Unable to get IP Address";
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    int success = 0;
    // retrieve the current interfaces - returns 0 on success
    success = getifaddrs(&interfaces);
    if (success == 0) {
        // Loop through linked list of interfaces
        temp_addr = interfaces;
        while(temp_addr != NULL) {
            if(temp_addr->ifa_addr->sa_family == AF_INET) {
                // Check if interface is en0 which is the wifi connection on the iPhone
                if(strcmp(temp_addr->ifa_name, "enp11s0")==0 || strcmp(temp_addr->ifa_name, "eno2")==0 || strcmp(temp_addr->ifa_name, "eno1")==0 
                  || strcmp(temp_addr->ifa_name, "ens1f0") == 0 || strcmp(temp_addr->ifa_name, "enp10s0") == 0){
                    ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
                }
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    // Free memory
    freeifaddrs(interfaces);
    std::cout << "ip address " << ipAddress << "\n";
    return ipAddress;
    // return IP;
#endif
  }
  
}
