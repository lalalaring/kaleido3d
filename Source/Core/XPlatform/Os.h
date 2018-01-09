#ifndef __Os_h__
#define __Os_h__

#if K3DPLATFORM_OS_LINUX
#include <netinet/in.h>
#endif


/**
 * This module provides facilities on OS like:
 * File,Directory,Socket,Threading
 */
namespace k3d 
{
    namespace os
    {
        class K3D_CORE_API File : public k3d::IIODevice
        {
        public:
            File();
            explicit File(const char* fileName);

            ~File();

            bool Open(k3d::IOFlag flag);
            bool Open(const char* fileName, k3d::IOFlag flag);

            I64 GetSize();

            bool IsEOF();

            size_t Read(char* ptr, size_t len);
            size_t Write(const void* ptr, size_t len);

            bool Seek(size_t offset);
            bool Skip(size_t offset);

            void Flush();
            void Close();

            U64 LastModified() const;

            static File* CreateIOInterface();

        private:
#if K3DPLATFORM_OS_WINDOWS
            void* m_hFile;
#else
            int m_fd;
#endif
            bool m_EOF;
            I64 m_CurOffset;
            const char* m_pFileName;
        };

        class K3D_CORE_API MemMapFile : public k3d::IIODevice
        {
        public:
            MemMapFile();
            ~MemMapFile();

            I64 GetSize();
            //---------------------------------------------------------
            bool Open(const char* fileName, k3d::IOFlag mode);
            size_t Read(char* data_ptr, size_t len);
            size_t Write(const void*, size_t);
            bool Seek(size_t offset);
            bool Skip(size_t offset);
            bool IsEOF();
            void Flush();
            void Close();
            //---------------------------------------------------------

            //---------------------------------------------------------
            /// FileData
            /// \brief FileData
            /// \return data const pointer
            U8* FileData() { return m_pData; }

            template<class T>
            /// Convert FileBlocks To Class
            /// \brief ConvertToClass
            /// \param address_of_file
            /// \param object_offset
            /// \return Object Pointer
            static T* ConvertToClass(U8*& address_of_file, U32 object_offset)
            {
                U8* objectptr = address_of_file + object_offset;
                address_of_file += sizeof(T) + object_offset;
                return reinterpret_cast<T*>(objectptr);
            }
            //---------------------------------------------------------

            /// General Interface For GetIODevice
            /// \brief CreateIOInterface
            /// \return An IIODevice Pointer
            static MemMapFile* CreateIOInterface();

        private:
#if K3DPLATFORM_OS_WINDOWS
            void* m_FileHandle;
            void* m_FileMappingHandle;
#else
            int m_Fd;
#endif
            size_t m_szFile;
            U8* m_pData;
            U8* m_pCur;
        };

        class K3D_CORE_API LibraryLoader
        {
        public:
            LibraryLoader(const char* libPath);
            virtual ~LibraryLoader();

            void* ResolveSymbol(const char* entryName);

        private:
            struct LibraryPrivate* d;
        };

        extern K3D_CORE_API k3d::String Join(k3d::String const& Path0, k3d::String const&Path1);
        extern K3D_CORE_API k3d::String Join(k3d::String const& Path0, k3d::String const&Path1, k3d::String const&Path2);
        extern K3D_CORE_API bool MakeDir(const char* name);
        extern K3D_CORE_API bool Exists(const char* name);
        extern K3D_CORE_API bool Copy(const char* src, const char* target);
        extern K3D_CORE_API bool Remove(const char* name);
        typedef void(*PFN_FileProcessRoutine)(const char* path, bool isDir);
        extern K3D_CORE_API bool Walk(const char* srcPath, PFN_FileProcessRoutine);


        extern K3D_CORE_API int
            Exec(const char* cmd, char* const* argv);
        extern K3D_CORE_API void
            Sleep(U32 ms);
        extern K3D_CORE_API U32
            GetCpuCoreNum();
        extern K3D_CORE_API float*
            GetCpuUsage();
        extern K3D_CORE_API U32
            GetGpuCount();
        /**
        * Get Gpu load by ID
        */
        extern K3D_CORE_API float
            GetGpuUsage(int GpuId);

        extern K3D_CORE_API U64 GetTicks();

        enum class ThreadPriority
        {
            Low,
            Normal,
            High,
            RealTime
        };

        enum class ThreadStatus
        {
            Ready,
            Running,
            Finish
        };

        struct MutexPrivate;
        class K3D_CORE_API Mutex
        {
        public:
            Mutex();
            ~Mutex();

            void Lock();
            void UnLock();
            friend class ConditionVariable;

            struct AutoLock
            {
                AutoLock()
                {
                    m_Mutex = new Mutex;
                    m_Mutex->Lock();
                }

                explicit AutoLock(Mutex* mutex, bool lostOwnerShip = false)
                    : m_OnwerShipGot(lostOwnerShip)
                    , m_Mutex(mutex)
                {
                }

                ~AutoLock()
                {
                    m_Mutex->UnLock();
                    if (m_OnwerShipGot) {
                        delete m_Mutex;
                        m_Mutex = nullptr;
                    }
                }

            private:
                bool m_OnwerShipGot;
                Mutex* m_Mutex;
            };

        private:
            MutexPrivate* m_Impl;
        };

        struct ConditionVariablePrivate;
        class K3D_CORE_API ConditionVariable
        {
        public:
            ConditionVariable();
            ~ConditionVariable();

            void Wait(Mutex* mutex);
            void Wait(Mutex* mutex, U32 milliseconds);
            void Notify();
            void NotifyAll();

            ConditionVariable(const ConditionVariable&) = delete;
            ConditionVariable(const ConditionVariable&&) = delete;

        protected:
            ConditionVariablePrivate* m_Impl;
        };

#define __INTERNAL_THREAD_ROUTINE_RETURN void *

        namespace __internal
        {
            struct ThreadClosure
            {
                void* operator new(size_t Size)
                {
                    return __k3d_malloc__(Size);
                }
                void operator delete(void * Ptr)
                {
                    __k3d_free__(Ptr, 0);
                }
            };

            template <class F> struct ThreadClosure0 : ThreadClosure
            {
                F Function;
                static __INTERNAL_THREAD_ROUTINE_RETURN StartRoutine(void* C)
                {
                    ThreadClosure0* Self = static_cast<ThreadClosure0*>(C);
                    Self->Function();
                    delete Self;
                    return 0;
                }

                ThreadClosure0(const F& f) : Function(f) {}
            };

            template <class F, class X> struct ThreadClosure1 : ThreadClosure
            {
                F Function;
                X Arg1;

                static __INTERNAL_THREAD_ROUTINE_RETURN StartRoutine(void* C)
                {
                    ThreadClosure1* Self = static_cast<ThreadClosure1*>(C);
                    Self->Function(Self->Arg1);
                    delete Self;
                    return 0;
                }

                ThreadClosure1(const F&f, const X&x) : Function(f), Arg1(x) {}
            };
        }

        class K3D_CORE_API Thread
        {
        public:
            // static functions
            static void SleepForMilliSeconds(U32 millisecond);
            // static void Yield();
            static U32 GetId();

        public:
            typedef void* Handle;

            Thread();

            template <class F> explicit Thread(F f, k3d::String const& Name,
                ThreadPriority Priority = ThreadPriority::Normal)
                : m_ThreadName(Name)
                , m_ThreadPriority(Priority)
                , m_StackSize(2048)
                , m_ThreadStatus(ThreadStatus::Ready)
                , m_ThreadHandle(nullptr)
            {
                typedef __internal::ThreadClosure0<F> closure_t;
                InternalStart(closure_t::StartRoutine, new closure_t(f));
            }

            explicit Thread(k3d::String const& name,
                ThreadPriority priority = ThreadPriority::Normal);

            virtual ~Thread();

            void SetPriority(ThreadPriority prio);

            void Start();

            void Join();
            void Terminate();

            ThreadStatus GetThreadStatus();
            k3d::String GetName();

        public:
            static k3d::String GetCurrentThreadName();
            static void SetCurrentThreadName(k3d::String const& name);

        private:
            typedef __INTERNAL_THREAD_ROUTINE_RETURN(*ThrRoutine)(void*);
            void InternalStart(ThrRoutine Routine, __internal::ThreadClosure* Closure);

            static __INTERNAL_THREAD_ROUTINE_RETURN RunOnThread(void*);

            k3d::String                 m_ThreadName;
            ThreadPriority              m_ThreadPriority;
            U32                         m_StackSize;
            ThreadStatus                m_ThreadStatus;
            Handle                      m_ThreadHandle;
            ThrRoutine                  m_ThreadFunc;
            __internal::ThreadClosure*  m_ThreadClosure;
        };

        class IpAddressImpl;
        class SocketImpl;

        class K3D_CORE_API IpAddress
        {
        public:
            enum Type
            {
                V4,
                V6
            };

            explicit IpAddress(String const& Ip);
            ~IpAddress();

            Type GetType() const;

            I32 GetPort() const;

            void SetAddrPort(I32 Port);

            static IpAddress* GetHostIp(String const& HostName);

        private:
            friend class Socket;
            friend class SocketImpl;

            IpAddressImpl* d;
        };

        enum class SockStatus
        {
            Connected,
            DisConnected,
            Error
        };

        enum class SockType
        {
            TCP,
            UDP,
            RAW
        };

        enum class SoToOpt : U32
        {
            Receive = 0,
            Send = 1,
            /*Connect = 2*/
        };

        class K3D_CORE_API Socket
        {
        public:
            explicit Socket(SockType const& type);

            virtual ~Socket();

            bool IsValid();

            void SetTimeOutOpt(SoToOpt opt, U32 milleseconds);
            void SetBlocking(bool block);

            virtual U64 Receive(void* pData, U64 recvLen);
            virtual U64 Send(const char* pData, U64 sendLen);
            virtual U64 Send(String const& buffer);

        protected:
            void Create();
            void Bind(IpAddress const& ipAddr);
            void Listen(int maxConn);
            virtual void Connect(IpAddress const& ipAddr);
            virtual void Close();
            virtual Socket* Accept(IpAddress const& ipAddr);
            //SocketHandle GetHandle() { return m_SockFd; }
            virtual I32 GetError();
            virtual void OnHandleError(int Code);
            virtual void OnCreated(void* RawSocketHandle) {}
        private:
            Socket(SockType const& Type, void* RawSocketHandle);
            SocketImpl* d;
        };
    }
}

//KTYPE_ISTYPE_TEMPLATE(Os::File, IIODevice);
//KTYPE_ISTYPE_TEMPLATE(Os::MemMapFile, IIODevice);

#endif
