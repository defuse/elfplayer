#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/user.h>
#include <sys/types.h>

void printUsage();
void wait_for_stop(pid_t pid);

int main(int argc, char **argv, char **envp)
{
    if (argc < 3) {
        printUsage("Not enough arguments.");
        return EXIT_SUCCESS;
    }

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

            /* XXX get this dynamically */
            if (regs.rip == 0x8048660) {
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
        fprintf(stderr, "Failed waiting for socio9op to stop (%d).\n", errno);
        exit(1);
    }
}
