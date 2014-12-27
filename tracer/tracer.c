#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/user.h>
#include <sys/types.h>
#include <fcntl.h>

#include <libelf.h>
#include <gelf.h>

void printUsage();
void wait_for_stop(pid_t pid);
unsigned long long get_elf_entry(const char *path);

int main(int argc, char **argv, char **envp)
{
    if (argc < 3) {
        printUsage("Not enough arguments.");
        return EXIT_SUCCESS;
    }

    unsigned long long entrypoint = get_elf_entry(argv[2]);

    pid_t pid;
    pid = fork();
    if (pid < 0) {
        printf("Fork failed.\n");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        /* We are the child. */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        execve(argv[2], argv+2, envp);
        /* Apparently the execve causes a SIGTRAP to be sent. */
    } else {
        /* We are the parent. */

        /* Wait for execve to finish. */
        wait_for_stop(pid);

        //long ret = 0;

        //ret = ptrace(PTRACE_ATTACH, pid, 0, 0);
        //if (ret != 0) {
        //    perror("Failed ptrace() attach.");
        //    exit(EXIT_FAILURE);
        //}

        //wait_for_stop(pid);

        struct user_regs_struct regs;

        //ptrace(PTRACE_SYSCALL, pid, 0, 0);
        //wait_for_stop(pid);
        //long syscall = ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*ORIG_EAX);
        //printf("EAX: %ld\n", syscall);
        //ptrace(PTRACE_SYSCALL, pid, 0, 0);
        //wait_for_stop(pid);
        //ptrace(PTRACE_GETREGS, pid, NULL, &regs);
        //printf("EAX: %lx\n", regs.rax);

        int hit_entrypoint = 0;

        FILE *output = fopen(argv[1], "w");
        if (output == NULL) {
            perror("Error opening output file.");
            return EXIT_FAILURE;
        }

        while (ptrace(PTRACE_SINGLESTEP, pid, 0, 0) == 0) {
            wait_for_stop(pid);

            if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) != 0) {
                perror("Error getting regs.");
            }

            if (regs.rip == entrypoint) {
                hit_entrypoint = 1;
            }

            if (hit_entrypoint) {
                fprintf(output, "%llx\n", regs.rip);
                // XXX
            }
        }
        fclose(output);
        perror("wat");
    }

    return EXIT_SUCCESS;
}

void printUsage(const char *error)
{
    if (error != NULL) {
        printf("Error: %s\n", error);
    }
    printf("Usage: ./tracer <output_file> <binary> <arguments>\n");
}

void wait_for_stop(pid_t pid)
{
tryagain:
    if (waitpid(pid, NULL, WUNTRACED) != pid) {
        if (errno == EINTR) {
            /* XXX HACK: We got interrupted by a signal or something, try again. */
            goto tryagain;
        }
        fprintf(stderr, "Failed waiting for process to stop (%d).\n", errno);
        exit(1);
    }
}

unsigned long long get_elf_entry(const char *path)
{
    Elf *e;
    Elf_Kind ek;
    GElf_Ehdr ehdr;
    unsigned long long entrypoint;

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "Elf library initialization failed.\n");
        exit(1);
    }

    int fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        fprintf(stderr, "Couldn't open the ELF at %s.\n", path);
        exit(1);
    }

    e = elf_begin(fd, ELF_C_READ, NULL);
    if (e == NULL) {
        fprintf(stderr, "elf_begin() failed.\n");
        exit(1);

    }

    ek = elf_kind(e);

    if (ek != ELF_K_ELF) {
        fprintf(stderr, "Bad ELF file.\n");
        exit(1);
    }

    if (gelf_getehdr(e, &ehdr) == NULL) {
        fprintf(stderr, "Couldn't get the ELF header.\n");
        exit(1);
    }

    if (gelf_getclass(e) != ELFCLASS32) {
        fprintf(stderr, "Not a 32-bit ELF.\n");
        exit(1);
    }

    entrypoint = ehdr.e_entry;

    elf_end(e);
    close(fd);

    return entrypoint;
}
