#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <ev.h>

#include "log.h"
#include "core.h"

#define DEFAULT_APPLE_P2P_DEVICE "apple_p2p"
#define FAILED_DUMP "failed.pcap"

static void daemonize() {
	pid_t pid;
	long x;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* On success: The child process becomes session leader */
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	/* Catch, ignore and handle signals */
	/* TODO: Implement a working signal handler */
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
		close(x);
	}

	/* Open the log file */
	openlog("apple_p2p_config", LOG_PID, LOG_DAEMON);
}

int main(int argc, char *argv[]) {
	int c;
	int daemon = 0;
	int dump = 0;
	int chan_num = 149;
	struct apple_p2p_chan chan;
	int log_level = LOG_INFO;
	int filter_rssi = 1;
	int no_monitor_mode = 0;

	char wlan[PATH_MAX];
	char host[IFNAMSIZ] = DEFAULT_APPLE_P2P_DEVICE;

	struct ev_loop *loop;

	struct daemon_state state;

	while ((c = getopt(argc, argv, "Dc:dvi:h:a:t:fN")) != -1) {
		switch (c) {
			case 'D':
				daemon = 1;
				break;
			case 'c':
				chan_num = atoi(optarg);
				break;
			case 'd':
				dump = 1;
				break;
			case 'f':
				filter_rssi = 0;
				break;
			case 'v':
				if (log_level < LOG_TRACE)
					log_level++; /* increase log level with every occurence of option */
				break;
			case 'i':
				strcpy(wlan, optarg);
				break;
			case 'h':
				strcpy(host, optarg);
				break;
			case 'N':
				no_monitor_mode = 1;
				break;
			case '?':
				if (optopt == 'i')
					fprintf(stderr, "Option -%c needs to specify a wireless interface.\n", optopt);
				return EXIT_FAILURE;
			default:
				abort();
		}
	}

	log_set_level(log_level);

	if (daemon)
		daemonize();

	switch (chan_num) {
		case 6:
			chan = CHAN_OPCLASS_6;
			break;
		case 44:
			chan = CHAN_OPCLASS_44;
			break;
		case 149:
			chan = CHAN_OPCLASS_149;
			break;
		default:
			log_error("Unsupported channel %d (use 6, 44, or 149)", chan_num);
			return EXIT_FAILURE;
	}

	state.io.wlan_no_monitor_mode = no_monitor_mode;
	if (apple_p2p_init(&state, wlan, host, chan, dump ? FAILED_DUMP : 0) < 0) {
		log_error("could not initialize core");
		return EXIT_FAILURE;
	}
	state.apple_p2p_state.filter_rssi = filter_rssi;
	log_error("main 111");
	if (state.io.wlan_ifindex)
		log_info("WLAN device: %s (addr %s)", state.io.wlan_ifname, ether_ntoa(&state.io.if_ether_addr));
	if (state.io.host_ifindex)
		log_info("Host device: %s", state.io.host_ifname);
	log_error("main 2222");
	loop = EV_DEFAULT;
	log_error("main 3333");
	apple_p2p_schedule(loop, &state);
	log_error("main 4444");
	ev_run(loop, 0);
	log_error("main 5555");
	apple_p2p_free(&state);

	return EXIT_SUCCESS;
}
