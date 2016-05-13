#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <dirent.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>


#define color_printf(fmt, ...) printf("\033[1;31m" fmt "\033[0m", ##__VA_ARGS__)

bool nepenthe_enable_debug = false;


static void _signal_handler(int signal);
void _dump_process_status(pid_t pid);
void _dump_maps(pid_t pid);
void _dump_fds(pid_t pid);

static void __attribute__((constructor)) _enable_debug(void)
{
	char *enable_debug = getenv("D_DEBUG");
	if (enable_debug) {
		nepenthe_enable_debug = true;
	}

	if (nepenthe_enable_debug) {
		color_printf("======== Enable_debug: ========\n");


		/* initialize signals */

		signal(SIGINT, _signal_handler);
		signal(SIGQUIT, _signal_handler);
		signal(SIGILL, _signal_handler);
		signal(SIGABRT, _signal_handler);
		signal(SIGBUS, _signal_handler);
		signal(SIGFPE, _signal_handler);
		/* signal (SIGKILL, _signal_handler); */
		signal(SIGSEGV, _signal_handler);
		signal(SIGTERM, _signal_handler);
	}
}


void _signal_handler(int signal)
{
	/* dump signal */
	color_printf("\n======== Signal: ========\n"
	       "Cause: ");
	switch (signal) {
	case SIGHUP:
		printf("SIGHUP");
		break;
	case SIGINT:
		printf("SIGINT");
		break;
	case SIGQUIT:
		printf("SIGQUIT");
		break;
	case SIGILL:
		printf("SIGILL");
		break;
	case SIGTRAP:
		printf("SIGTRAP");
		break;
	case SIGABRT:
		printf("SIGABRT");
		break;
	case SIGBUS:
		printf("SIGBUS");
		break;
	case SIGFPE:
		printf("SIGFPE");
		break;
	case SIGKILL:
		printf("SIGKILL");
		break;
	case SIGUSR1:
		printf("SIGUSR1");
		break;
	case SIGSEGV:
		printf("SIGSEGV");
		break;
	case SIGUSR2:
		printf("SIGUSR2");
		break;
	case SIGPIPE:
		printf("SIGPIPE");
		break;
	case SIGALRM:
		printf("SIGALRM");
		break;
	case SIGTERM:
		printf("SIGTERM");
		break;
	case SIGSTKFLT:
		printf("SIGSTKFLT");
		break;
	case SIGCHLD:
		printf("SIGCHLD");
		break;
	case SIGCONT:
		printf("SIGCONT");
		break;
	case SIGSTOP:
		printf("SIGSTOP");
		break;
	case SIGTSTP:
		printf("SIGTSTP");
		break;
	case SIGTTIN:
		printf("SIGTTIN");
		break;
	case SIGTTOU:
		printf("SIGTTOU");
		break;
	case SIGURG:
		printf("SIGURG");
		break;
	case SIGXCPU:
		printf("SIGXCPU");
		break;
	case SIGXFSZ:
		printf("SIGXFSZ");
		break;
	case SIGVTALRM:
		printf("SIGVTALRM");
		break;
	case SIGPROF:
		printf("SIGPROF");
		break;
	case SIGWINCH:
		printf("SIGWINCH");
		break;
	case SIGIO:
		printf("SIGIO or SIGPOLL");
		break;
	case SIGPWR:
		printf("SIGPWR");
		break;
	case SIGSYS:
		printf("SIGSYS");
		break;
	default:
		printf("%d", signal);
	}
	printf("\n");

	/* backtrace */
	if (nepenthe_enable_debug) {
		void *buffer[128];
		int nptrs = backtrace(buffer, sizeof(buffer));
		color_printf("======== Backtrace: ========\n");
		char **strings = backtrace_symbols(buffer, nptrs);
		if (strings != NULL) {
			int i;
			for (i = 0; i < nptrs; i++)
				printf("%s\n", strings[i]);
			free(strings);
		}
	}

	/* pid dependent information */
	pid_t pid = getpid();

	if (nepenthe_enable_debug) {
		_dump_process_status(pid);
		_dump_maps(pid);
		_dump_fds(pid);
	}

	exit(1);
}


void _dump_process_status(pid_t pid)
{

	color_printf("======= Process status: =======\n");

	/* dump process status */
	char status_name[512];
	if (pid) {
		sprintf(status_name, "/proc/%d/status", pid);
	} else {
		sprintf(status_name, "/proc/self/status");
	}
	FILE *fh = fopen(status_name, "r");
	if (!fh) {
		printf("cant open %s\n", status_name);
	} else {
		char line[1024];
		while (fgets(line, sizeof(line) - 1, fh)) {
			printf("%s", line);
		}
		fclose(fh);
	}
}


void _dump_maps(pid_t pid)
{
	color_printf("======= Memory maps: =======\n");

	/* dump maps */
	char file_name[512];
	if (pid) {
		sprintf(file_name, "/proc/%d/maps", pid);
	} else {
		sprintf(file_name, "/proc/self/maps");
	}
	FILE *fh = fopen(file_name, "r");
	if (!fh) {
		printf("cant open %s\n", file_name);
	} else {
		char line[1024];
		int total_size = 0;
		while (fgets(line, sizeof(line) - 1, fh)) {
			char *pt = strtok(line, "-");
			unsigned long begin_addr = strtoul(pt, NULL, 16);
			pt = strtok(NULL, " \t");
			unsigned long end_addr = strtoul(pt, NULL, 16);
			unsigned long size = end_addr - begin_addr;
			total_size += size;
			pt = strtok(NULL, "\n");
			printf("0x%08lX-0x%08lX %8luMB %s\n", begin_addr,
			       end_addr, (size + 512 * 1024) / (1024 * 1024),
			       pt);
		}
		fclose(fh);
		printf("=> total size %d MB\n",
		       (total_size + 512 * 1024) / (1024 * 1024));
	}
}

void _dump_fds(pid_t pid)
{
	color_printf("======== File descriptors: ========\n");

	/* dump fds */
	char dir_name[512];
	if (pid) {
		sprintf(dir_name, "/proc/%d/fd", pid);
	} else {
		sprintf(dir_name, "/proc/self/fd");
	}
	DIR *dh = opendir(dir_name);
	if (!dh) {
		printf("cant open %s\n", dir_name);
	} else {
		struct dirent *dp;
		int total_fd = 0;
		while ((dp = readdir(dh)) != NULL) {
			if ((strcmp(".", dp->d_name) == 0) ||
			    (strcmp("..", dp->d_name) == 0))
				continue;
			char fdinfo_name[256];
			char link_name[1024];
			sprintf(fdinfo_name, "%s/%s", dir_name, dp->d_name);
			size_t len = readlink(fdinfo_name, link_name,
					      sizeof(link_name) - 1);
			if ((len > 0) && (len < sizeof(link_name) - 1)) {
				link_name[len] = '\0';
				printf("%s\t%s\n", dp->d_name, link_name);
			}
			total_fd++;
		}
		closedir(dh);
		printf("=> total number of links %d\n", total_fd);
	}
}
