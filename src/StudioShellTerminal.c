/* Studio argv/run adapter derived from libraries/terminalkit/native/bridge.c.
   Keeps terminalkit unchanged; adds literal argv and asynchronous cleanup. */
/* PTY/ConPTY scalar ABI. Main-thread nonblocking I/O; each handle has one owner. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#define API __declspec(dllexport)
typedef HRESULT (WINAPI *CreatePTY)(COORD,HANDLE,HANDLE,DWORD,HANDLE*);
typedef HRESULT (WINAPI *ResizePTY)(HANDLE,COORD);
typedef void (WINAPI *ClosePTY)(HANDLE);
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif
#else
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <poll.h>
#include <dirent.h>
#ifdef __APPLE__
#include <sys/sysctl.h>
#include <libproc.h>
#endif
#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif
#define API __attribute__((visibility("default")))
#endif

/* Buoy's Linux link-time libc stub omits these POSIX exports. Resolve the
   real host libc functions at runtime; do not replace the toolchain stubs. */
#if defined(__linux__)
#include <dlfcn.h>
#include "StudioGTKPaneViewport.h"
#define getsid(...) ((pid_t(*)(pid_t))dlsym(RTLD_DEFAULT,"getsid"))(__VA_ARGS__)
#define forkpty(...) ((pid_t(*)(int*,char*,const struct termios*,const struct winsize*))dlsym(RTLD_DEFAULT,"forkpty"))(__VA_ARGS__)
#define setpgid(...) ((int(*)(pid_t,pid_t))dlsym(RTLD_DEFAULT,"setpgid"))(__VA_ARGS__)
#define tcsetpgrp(...) ((int(*)(int,pid_t))dlsym(RTLD_DEFAULT,"tcsetpgrp"))(__VA_ARGS__)
#define poll(...) ((int(*)(struct pollfd*,nfds_t,int))dlsym(RTLD_DEFAULT,"poll"))(__VA_ARGS__)
#define ioctl(...) ((int(*)(int,unsigned long,...))dlsym(RTLD_DEFAULT,"ioctl"))(__VA_ARGS__)
#define strtol(...) ((long(*)(const char*,char**,int))dlsym(RTLD_DEFAULT,"strtol"))(__VA_ARGS__)
#endif

typedef struct {
    int exited, eof, started;
    char *arguments[258]; size_t argc, argument_bytes;
    int cleanup_started;
#ifdef _WIN32
    HANDLE cleanup_thread;
#else
    pthread_t cleanup_thread;
    _Atomic int cleanup_done;
#endif
    int64_t code;
    char error[512];
#ifdef _WIN32
    HANDLE child, job, input, output, console, writer, event;
    CRITICAL_SECTION lock;
    unsigned char queue[65536];
    DWORD queued;
    volatile LONG stopping;
    ClosePTY close_pty;
    ResizePTY resize_pty;
#else
    pid_t child;
    int master, status_fd, control_fd;
#endif
} Terminal;
static int fail(Terminal *t,const char *s) {
#ifdef _WIN32
    snprintf(t->error,sizeof(t->error),"%s (Windows error %lu)",s,(unsigned long)GetLastError());
#else
    snprintf(t->error,sizeof(t->error),"%s: %s",s,strerror(errno));
#endif
    return 0;
}
API void *sst_new(void) {
    Terminal *t=calloc(1,sizeof(*t));
    if(!t)return NULL;
#ifdef _WIN32
    InitializeCriticalSection(&t->lock);
#else
    t->master=t->status_fd=t->control_fd=-1;
#endif
    return t;
}
API const char *sst_error(Terminal *t) { return t?t->error:"Cannot allocate terminal"; }
#ifdef _WIN32
static wchar_t *wide(const char *s) {
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,NULL,0);
    if(!n)return NULL;
    wchar_t *p=calloc((size_t)n,sizeof(wchar_t));
    if(p&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,p,n)){free(p);return NULL;}
    return p;
}
static DWORD WINAPI writer_main(void *value) {
    Terminal *t=value;
    unsigned char data[4096];
    while(!InterlockedCompareExchange(&t->stopping,0,0)) {
        DWORD n=0;
        EnterCriticalSection(&t->lock);
        n=t->queued>sizeof(data)?sizeof(data):t->queued;
        memcpy(data,t->queue,n);
        memmove(t->queue,t->queue+n,t->queued-n);
        t->queued-=n;
        LeaveCriticalSection(&t->lock);
        if(!n){WaitForSingleObject(t->event,100);continue;}
        DWORD offset=0;
        while(offset<n&&!InterlockedCompareExchange(&t->stopping,0,0)) {
            DWORD accepted=0;
            if(!WriteFile(t->input,data+offset,n-offset,&accepted,NULL)||!accepted)return 0;
            offset+=accepted;
        }
    }
    return 0;
}
#endif
#ifndef _WIN32
static void close_descriptors_from(int first) {
#ifdef __APPLE__
    /* Darwin has no public closefrom. Query this process's actual descriptor list,
       rather than trusting a soft limit that may have been lowered after open. */
    int needed=proc_pidinfo(getpid(),PROC_PIDLISTFDS,0,NULL,0);
    if(needed>0&&needed<64*1024*1024){
        int capacity=needed+4096;
        struct proc_fdinfo *items=malloc((size_t)capacity);
        if(items){int received=proc_pidinfo(getpid(),PROC_PIDLISTFDS,0,items,capacity);
            if(received>0){int count=received/(int)sizeof(*items);
                for(int i=0;i<count;i++)if(items[i].proc_fd>=first)close(items[i].proc_fd);
                free(items);return;}
            free(items);
        }
    }
#endif
    long maximum=sysconf(_SC_OPEN_MAX);if(maximum<0||maximum>1048576)maximum=1048576;
    struct rlimit limit;if(getrlimit(RLIMIT_NOFILE,&limit)==0&&limit.rlim_max!=RLIM_INFINITY&&limit.rlim_max>(rlim_t)maximum&&limit.rlim_max<=1048576)maximum=(long)limit.rlim_max;
    for(int fd=first;fd<maximum;fd++)close(fd);
}
/* The supervisor keeps the session ID reserved until close: stale handles cannot
   signal an unrelated process after the interactive shell exits. */
static void kill_session(pid_t session,pid_t except) {
#ifdef __APPLE__
    int mib[4]={CTL_KERN,KERN_PROC,KERN_PROC_ALL,0};size_t size=0;
    if(sysctl(mib,4,NULL,&size,NULL,0)==0){
        struct kinfo_proc *items=malloc(size);
        if(items&&sysctl(mib,4,items,&size,NULL,0)==0){
            size_t count=size/sizeof(*items);
            for(size_t i=0;i<count;i++){pid_t pid=items[i].kp_proc.p_pid;if(pid>1&&pid!=except&&getsid(pid)==session)kill(pid,SIGKILL);}
        }
        free(items);
    }
#else
    DIR *directory=opendir("/proc");
    if(directory){struct dirent *entry;while((entry=readdir(directory))){
        char *end=NULL;long value=strtol(entry->d_name,&end,10);
        if(end&&!*end&&value>1&&value<=INT32_MAX&&value!=except&&getsid((pid_t)value)==session)kill((pid_t)value,SIGKILL);
    }closedir(directory);}
#endif
}
#endif
API int64_t sst_argument(Terminal *t,const char *text) {
    if(!t||t->started||!text||t->argc>=257)return 0;
    size_t n=strlen(text);if(t->argument_bytes+n+1>65536)return 0;
    char *copy=malloc(n+1);if(!copy)return 0;memcpy(copy,text,n+1);
    t->arguments[t->argc++]=copy;t->argument_bytes+=n+1;return 1;
}
#ifdef _WIN32
static wchar_t *sst_command_line(Terminal *t) {
    size_t capacity=1;wchar_t *args[258]={0};
    for(size_t i=0;i<t->argc;i++) {
        args[i]=wide(t->arguments[i]);if(!args[i])goto fail;
        size_t n=wcslen(args[i]);if(n>16382||capacity+n*2+3>32767)goto fail;
        capacity+=n*2+3;
    }
    wchar_t *line=calloc(capacity,sizeof(wchar_t));if(!line)goto fail;
    size_t at=0;
    for(size_t i=0;i<t->argc;i++) {
        if(i)line[at++]=L' ';
        line[at++]=L'"';size_t slashes=0;
        for(const wchar_t *c=args[i];;c++) {
            if(*c==L'\\'){slashes++;continue;}
            size_t emit=(*c==L'"'||!*c)?slashes*2:slashes;
            for(size_t j=0;j<emit;j++)line[at++]=L'\\';
            slashes=0;if(!*c)break;if(*c==L'"')line[at++]=L'\\';line[at++]=*c;
        }
        line[at++]=L'"';free(args[i]);args[i]=NULL;
    }
    return line;
fail:
    for(size_t i=0;i<t->argc;i++)free(args[i]);
    return NULL;
}
#endif
API int64_t sst_start(Terminal *t,const char *shell,const char *cwd,int64_t cols,int64_t rows) {
#if defined(__linux__)
    for(size_t i=0;i<7;i++){
        const char *names[]={"getsid","forkpty","setpgid","tcsetpgrp","poll","ioctl","strtol"};
        if(!dlsym(RTLD_DEFAULT,names[i])){if(t)snprintf(t->error,sizeof(t->error),"Host libc lacks %s",names[i]);return 0;}
    }
#endif
    if(!t||t->argc==0||t->started||!shell||!*shell||!cwd||cols<2||cols>500||rows<2||rows>300)return 0;
#ifdef _WIN32
    HMODULE kernel=GetModuleHandleW(L"kernel32.dll");
    CreatePTY create=(CreatePTY)(void*)GetProcAddress(kernel,"CreatePseudoConsole");
    t->close_pty=(ClosePTY)(void*)GetProcAddress(kernel,"ClosePseudoConsole");
    t->resize_pty=(ResizePTY)(void*)GetProcAddress(kernel,"ResizePseudoConsole");
    if(!create||!t->close_pty||!t->resize_pty){snprintf(t->error,sizeof(t->error),"Windows 10 1809 or newer is required for ConPTY");return 0;}
    HANDLE in_read=NULL,out_write=NULL;
    if(!CreatePipe(&in_read,&t->input,NULL,65536))return fail(t,"Create input pipe");
    if(!CreatePipe(&t->output,&out_write,NULL,65536)){CloseHandle(in_read);return fail(t,"Create output pipe");}
    COORD size={(SHORT)cols,(SHORT)rows};
    HRESULT hr=create(size,in_read,out_write,0,&t->console);
    CloseHandle(in_read);CloseHandle(out_write);
    if(FAILED(hr)){snprintf(t->error,sizeof(t->error),"CreatePseudoConsole failed (0x%lx)",(unsigned long)hr);return 0;}
    SIZE_T bytes=0;
    InitializeProcThreadAttributeList(NULL,1,0,&bytes);
    STARTUPINFOEXW startup;memset(&startup,0,sizeof(startup));startup.StartupInfo.cb=sizeof(startup);
    /* Do not preserve redirected standard streams from the host. NULL handles
       are filled from the new pseudoconsole when the client attaches:
       https://learn.microsoft.com/en-us/windows/console/getstdhandle */
    startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
    startup.lpAttributeList=malloc(bytes);
    if(!startup.lpAttributeList)return fail(t,"Allocate startup attributes");
    if(!InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&bytes)){free(startup.lpAttributeList);return fail(t,"Initialize startup attributes");}
    int ok=UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,t->console,sizeof(t->console),NULL,NULL);
    wchar_t *exe=wide(shell),*directory=wide(cwd);
    wchar_t *command=sst_command_line(t);
    PROCESS_INFORMATION process;memset(&process,0,sizeof(process));
    if(!exe||!directory||!command)ok=0;
    if(ok)ok=CreateProcessW(exe,command,NULL,NULL,FALSE,EXTENDED_STARTUPINFO_PRESENT|CREATE_SUSPENDED,NULL,*cwd?directory:NULL,&startup.StartupInfo,&process);
    free(exe);free(command);free(directory);
    DeleteProcThreadAttributeList(startup.lpAttributeList);free(startup.lpAttributeList);
    if(!ok)return fail(t,"Start terminal shell");
    t->child=process.hProcess;t->job=CreateJobObjectW(NULL,NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;memset(&limits,0,sizeof(limits));limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(!t->job||!SetInformationJobObject(t->job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))||!AssignProcessToJobObject(t->job,t->child)){
        TerminateProcess(t->child,1);CloseHandle(process.hThread);return fail(t,"Own terminal process tree");
    }
    t->event=CreateEventW(NULL,FALSE,FALSE,NULL);
    if(!t->event){TerminateJobObject(t->job,1);CloseHandle(process.hThread);return fail(t,"Create terminal input event");}
    t->writer=CreateThread(NULL,0,writer_main,t,0,NULL);
    if(!t->event||!t->writer){TerminateJobObject(t->job,1);CloseHandle(process.hThread);return fail(t,"Start terminal input worker");}
    DWORD resumed=ResumeThread(process.hThread);CloseHandle(process.hThread);
    if(resumed==(DWORD)-1){TerminateJobObject(t->job,1);return fail(t,"Resume terminal shell");}
#else
    int status_pipe[2],control_pipe[2];
    if(pipe(status_pipe)<0)return fail(t,"Create shell status pipe");
    if(pipe(control_pipe)<0){close(status_pipe[0]);close(status_pipe[1]);return fail(t,"Create terminal ownership pipe");}
    struct winsize size;memset(&size,0,sizeof(size));size.ws_col=(unsigned short)cols;size.ws_row=(unsigned short)rows;
    pid_t pid=forkpty(&t->master,NULL,NULL,&size);
    if(pid<0){close(status_pipe[0]);close(status_pipe[1]);close(control_pipe[0]);close(control_pipe[1]);return fail(t,"forkpty");}
    if(pid==0){
        close(status_pipe[0]);close(control_pipe[1]);
        int status_write=fcntl(status_pipe[1],F_DUPFD,10);
        int control_read=fcntl(control_pipe[0],F_DUPFD,10);
        if(status_write<0||control_read<0)_exit(125);
        if(dup2(status_write,3)<0||dup2(control_read,4)<0)_exit(125);
        status_pipe[1]=3;control_pipe[0]=4;
        close_descriptors_from(5);
        /* The host may ignore SIGCHLD (or use SA_NOCLDWAIT). The supervisor
           owns waitpid(shell_pid), so it must retain its child's exit status. */
        signal(SIGCHLD,SIG_DFL);
        signal(SIGTTOU,SIG_IGN);signal(SIGTTIN,SIG_IGN);signal(SIGHUP,SIG_IGN);signal(SIGPIPE,SIG_IGN);
        pid_t shell_pid=fork();
        if(shell_pid==0){
            setpgid(0,0);tcsetpgrp(0,getpid());
            signal(SIGTTOU,SIG_DFL);signal(SIGTTIN,SIG_DFL);signal(SIGHUP,SIG_DFL);signal(SIGPIPE,SIG_DFL);
            if(*cwd&&chdir(cwd)<0){perror("Terminal working directory");_exit(126);}
            setenv("TERM","xterm",1);setenv("COLORTERM","truecolor",1);
            close_descriptors_from(3);
            execv(shell,t->arguments);perror("Terminal shell");_exit(127);
        }
        if(shell_pid<0)_exit(125);
        setpgid(shell_pid,shell_pid);tcsetpgrp(0,shell_pid);
        int reported=0;
        for(;;){
            if(!reported){int status=0;pid_t done=waitpid(shell_pid,&status,WNOHANG);
                if(done==shell_pid){int64_t code=WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);
                    (void)write(status_pipe[1],&code,sizeof(code));close(status_pipe[1]);reported=1;}}
            struct pollfd watch={control_pipe[0],POLLIN|POLLHUP,0};
            if(poll(&watch,1,50)>0&&(watch.revents&(POLLHUP|POLLERR))){kill_session(getpid(),getpid());_exit(0);}
        }
    }
    close(status_pipe[1]);close(control_pipe[0]);
    t->child=pid;t->status_fd=status_pipe[0];t->control_fd=control_pipe[1];
    fcntl(t->status_fd,F_SETFL,O_NONBLOCK);fcntl(t->status_fd,F_SETFD,FD_CLOEXEC);fcntl(t->control_fd,F_SETFD,FD_CLOEXEC);
    int value=fcntl(t->master,F_GETFL,0);
    if(value<0||fcntl(t->master,F_SETFL,value|O_NONBLOCK)<0)return fail(t,"Set PTY nonblocking");
    fcntl(t->master,F_SETFD,FD_CLOEXEC);
#endif
    t->started=1;return 1;
}
API int64_t sst_poll(Terminal *t) {
    if(!t||!t->started)return -1;
    if(t->exited)return 0;
#ifdef _WIN32
    DWORD state=WaitForSingleObject(t->child,0);
    if(state==WAIT_TIMEOUT)return 1;
    if(state!=WAIT_OBJECT_0){fail(t,"Poll terminal shell");return -1;}
    DWORD code=0;if(!GetExitCodeProcess(t->child,&code)){fail(t,"Read shell exit");return -1;}t->code=code;
#else
    ssize_t received=read(t->status_fd,&t->code,sizeof(t->code));
    if(received==(ssize_t)sizeof(t->code)){t->exited=1;return 0;}
    if(received<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))return 1;
    snprintf(t->error,sizeof(t->error),"Terminal supervisor exited without shell status");return -1;
#endif
    t->exited=1;return 0;
}
API int64_t sst_exit(Terminal *t){return t?t->code:-1;}
API int64_t sst_read(Terminal *t,void *buffer,int64_t capacity) {
    if(!t||!t->started||!buffer||capacity<1||capacity>1048576)return -1;
    if(t->eof)return 0;
#ifdef _WIN32
    DWORD available=0;
    if(!PeekNamedPipe(t->output,NULL,0,NULL,&available,NULL)){if(GetLastError()==ERROR_BROKEN_PIPE){t->eof=1;return 0;}fail(t,"Read terminal output");return -1;}
    if(!available)return 0;
    DWORD n=0,want=available<(DWORD)capacity?available:(DWORD)capacity;
    if(!ReadFile(t->output,buffer,want,&n,NULL)){fail(t,"Read terminal output");return -1;}return n;
#else
    ssize_t n=read(t->master,buffer,(size_t)capacity);
    if(n>0)return n;
    if(n==0||(n<0&&errno==EIO)){t->eof=1;return 0;}
    if(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR)return 0;
    fail(t,"Read PTY");return -1;
#endif
}
API int64_t sst_write(Terminal *t,const void *buffer,int64_t length) {
    if(!t||!t->started||t->exited||!buffer||length<0||length>65536)return -1;
#ifdef _WIN32
    EnterCriticalSection(&t->lock);
    DWORD n=(DWORD)length;if(n>sizeof(t->queue)-t->queued)n=(DWORD)sizeof(t->queue)-t->queued;
    memcpy(t->queue+t->queued,buffer,n);t->queued+=n;
    LeaveCriticalSection(&t->lock);SetEvent(t->event);return n;
#else
    ssize_t n=write(t->master,buffer,(size_t)length);
    if(n>=0)return n;
    if(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR)return 0;
    fail(t,"Write PTY");return -1;
#endif
}
API int64_t sst_resize(Terminal *t,int64_t cols,int64_t rows) {
    if(!t||!t->started||cols<2||cols>500||rows<2||rows>300)return 0;
#ifdef _WIN32
    COORD size={(SHORT)cols,(SHORT)rows};return SUCCEEDED(t->resize_pty(t->console,size));
#else
    struct winsize size;memset(&size,0,sizeof(size));size.ws_col=(unsigned short)cols;size.ws_row=(unsigned short)rows;
    if(ioctl(t->master,TIOCSWINSZ,&size)<0)return fail(t,"Resize PTY");
    return 1;
#endif
}
API void sst_close(Terminal *t) {
    if(!t)return;
#ifdef _WIN32
    if(t->job)TerminateJobObject(t->job,1);
    InterlockedExchange(&t->stopping,1);
    if(t->event)SetEvent(t->event);
    if(t->writer){
        /* Cancel repeatedly: a single cancellation can race just before WriteFile. */
        while(WaitForSingleObject(t->writer,20)==WAIT_TIMEOUT)CancelSynchronousIo(t->writer);
        CloseHandle(t->writer);
    }
    if(t->input)CloseHandle(t->input);
    if(t->output)CloseHandle(t->output);
    if(t->console)t->close_pty(t->console);
    if(t->child){WaitForSingleObject(t->child,2000);CloseHandle(t->child);}
    if(t->job)CloseHandle(t->job);
    if(t->event)CloseHandle(t->event);
    DeleteCriticalSection(&t->lock);
#else
    /* Release the master before waiting: Darwin's session-leader teardown can
       otherwise wait for this still-open PTY while our owner waits for it. */
    if(t->master>=0){close(t->master);t->master=-1;}
    if(t->child>0){kill_session(t->child,t->child);kill(t->child,SIGKILL);int status;while(waitpid(t->child,&status,0)<0&&errno==EINTR){}}
    if(t->status_fd>=0)close(t->status_fd);
    if(t->control_fd>=0)close(t->control_fd);
#endif
    for(size_t i=0;i<t->argc;i++)free(t->arguments[i]);
}
API void *sst_alloc(int64_t size){return size>0&&size<=1048576?malloc((size_t)size):NULL;}
API void sst_free(void *p){free(p);}

API void sst_stop(Terminal *t) {
    if(!t)return;
#ifdef _WIN32
    if(t->job)TerminateJobObject(t->job,1);
#else
    if(t->control_fd>=0){close(t->control_fd);t->control_fd=-1;}
    if(t->child>0)kill_session(t->child,t->child);
#endif
}
#ifdef _WIN32
static DWORD WINAPI sst_cleanup_worker(void *value) { sst_close(value);return 0; }
#else
static void *sst_cleanup_worker(void *value) {Terminal *t=value;sst_close(t);atomic_store(&t->cleanup_done,1);return NULL;}
#endif
/* Only free after worker has reaped the PTY supervisor and closed native handles. */
API int64_t sst_release_poll(Terminal *t) {
    if(!t)return 1;
    if(!t->cleanup_started) {
        sst_stop(t);
#ifdef _WIN32
        t->cleanup_thread=CreateThread(NULL,0,sst_cleanup_worker,t,0,NULL);
        if(!t->cleanup_thread)return 0;
#else
        if(pthread_create(&t->cleanup_thread,NULL,sst_cleanup_worker,t)!=0)return 0;
#endif
        t->cleanup_started=1;return 0;
    }
#ifdef _WIN32
    if(WaitForSingleObject(t->cleanup_thread,0)!=WAIT_OBJECT_0)return 0;
    CloseHandle(t->cleanup_thread);
#else
    if(!atomic_load(&t->cleanup_done))return 0;
    pthread_join(t->cleanup_thread,NULL);
#endif
    free(t);return 1;
}
