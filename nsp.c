/* nsp - Network Statistics Protocol client

   James Stanley */

#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>

/* all commands for one host, then move to the next */
#define HOST_FIRST 0
/* all hosts for one command, then move to the next */
#define CMD_FIRST 1

/* size of buffer for reading lines from files and network */
#define BUF_SIZ 4096

/* human-readable, machine-readable, or both? */
#define BOTH 0
#define HUMAN 1
#define MACHINE 2

char **command = NULL; /* NOTE: These two never get freed. This isn't a */
char **node = NULL;    /* problem as we exit when we're done with them. */
int commands = 0;
int nodes = 0;
int simultaneity = HOST_FIRST;
int show_host = 0;
int show_cmd = 0;
int readable = HUMAN;
char *argv0;

void usage(void) {
  fprintf(stderr,
  "nsp - Network Statistics Protocol client\n"
  "Usage: nsp [-n node_file] [-N node] [-c command_file] [-C command] \n"
  "           [-aAbhms] [NODE] [COMMANDS]\n"
  "\n"
  "Options:\n"
  "  -a               Always print hostnames before command output\n"
  "  -A               Always print command names before output\n"
  "  -b               Output both machine-readable and human-readable \n"
  "                   responses (see -m)\n"
  "  -c command_file  Read commands from this file (see -n)\n"
  "  -C command       Add this to the command list (see -N)\n"
  "  -h               Show this help\n"
  "  -m               Output machine-readable response only (see -b)\n"
  "  -n node_file     Read node hostnames from this file (see -c)\n"
  "  -N node          Add this to the node list (see -C)\n"
  "  -s               Toggle simultaneity\n"
  "\n"
  "Simultaneity is how to handle ordering of commands. The default is to send\n"
  "all commands to one node, then all commands to the next, and so on.\n"
  "Toggling this will cause nsp to send the first command to each node, and \n"
  "then the second command to each node, etc.\n"
  "The output will be printed in the order the commands were sent.\n"
  "\n"
  "Node files should contain lists of hostnames or IP addresses, one per \n"
  "line, optionally with port numbers, like so:\n"
  "  router3:5000\n"
  "for port 5000 on node router3. If no port number is given, 198 is assumed.\n"
  "\n"
  "Command files should contain lists of commands, one per line.\n"
  "\n"
  "Examples:\n"
  "  nsp node1 uptime load\n"
  "  nsp -n nodes -C uptime -C load\n"
  "  nsp -H -a node1 load\n"
  "\n"
  "Report bugs to James Stanley <james@incoherency.co.uk>\n"
  );
}

/* Reallocates list to be one element longer, adds item as the last valid
   element, and increments *len */
void add_list(char ***list, int *len, char *item) {
  /* reallocate list */
  if(!(*list = realloc(*list, sizeof(char*) * (*len + 1)))) {
    fprintf(stderr, "%s: realloc returned NULL.\n", argv0);
    exit(1);
  }

  /* add item; increment length */
  (*list)[(*len)++] = item;
}

/* Reads lines from filename, adding each one to list with add_list() */
void read_list(char ***list, int *len, const char *filename) {
  FILE *fp;
  char line[BUF_SIZ];
  char *p;
  int linenum = 0;
  int c;

  /* open the file for reading */
  if(!(fp = fopen(filename, "r"))) {
    fprintf(stderr, "%s:%s: unable to open for reading.\n", argv0, filename);
    return;
  }

  /* add each line to the list */
  while(fgets(line, BUF_SIZ, fp)) {
    linenum++;

    if(!(p = strchr(line, '\n')) && strlen(line) == (BUF_SIZ - 1)) {/* line too long */
      fprintf(stderr, "%s:%s:%d: line too long.\n", argv0, filename, linenum);

      /* now get the rest of the line out of the way */
      while(((c = getchar()) != '\n') && (c != EOF));
    } else {/* remove endline and add copy to list */
      if(p) *p = '\0';
      add_list(list, len, strdup(line));
    }
  }

  fclose(fp);
}

/* Outputs a line, with "host: " at the beginning if show_host is non-zero and
   "cmd: " at the beginning if show_cmd is non-zero.
   If 'line' contains a '\n', it will be replaced with a NUL byte, and then
   put back before returning */
void net_output(FILE *out, const char *host, const char *cmd, char *line) {
  char *p;

  /* remove endline */
  if((p = strchr(line, '\n'))) *p = '\0';

  /* output the line */
  if(show_host) fprintf(out, "%s: ", host);
  if(show_cmd) fprintf(out, "%s: ", cmd);

  fprintf(out, "%s\n", line);

  /* restore endline */
  if(p) *p = '\n';
}

/* Runs the given command on the given node and prints the output,
   prefixed with the hostname if show_host is non-zero.
   Warning: If host contains a colon, we will replace it with a NUL byte (but
   put it back before returning)
   If readable is:  We print:
   BOTH             All lines
   HUMAN            All lines except the first unless there is only one
   MACHINE          First line only */
void run_query(char *host, const char *command) {
  struct addrinfo *addr;
  struct addrinfo *a;
  struct addrinfo hints;
  char line1[BUF_SIZ];
  char line[BUF_SIZ];
  char *l1;
  char *port;
  char *p = NULL;
  int fd;
  FILE *net;
  int printfirst = 1;

  /* if there's a colon, remove it and get a pointer to the port number */
  if((p = strchr(host, ':'))) {
    *p = '\0';
    port = p + 1;
  } else port = "198";

  /* setup addrinfo hints */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  /* get the address */
  if(getaddrinfo(host, port, &hints, &addr) != 0) {
    fprintf(stderr, "%s: %s:%s: unable to get address.\n", argv0, host,
            port);
    if(p) *p = ':';
    return;
  }

  /* try to connect to one of the addresses */
  for(a = addr; a; a = a->ai_next) {
    if((fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol)) == -1)
      continue;

    if(connect(fd, a->ai_addr, a->ai_addrlen) == -1) {
      close(fd);
      continue;
    }

    break;
  }

  /* no valid address? */
  if(!a) {
    fprintf(stderr, "%s: %s:%s: unable to connect.\n", argv0, host, port);
    if(p) *p = ':';
    return;
  }

  /* fd is now connected to host, get a nice stdio handle */
  if(!(net = fdopen(fd, "r+"))) {
    fprintf(stderr, "%s: unable to get stdio handle for fd %d.\n", argv0, fd);
    close(fd);
    if(p) *p = ':';
    return;
  }

  /* send our command */
  fprintf(net, "%s\n", command);

  /* read first line of response */
  if(!(l1 = fgets(line1, BUF_SIZ, net)) || readable == MACHINE) {
    /* we only want the first line */
    if(l1) net_output(stdout, host, command, line1);
    fclose(net);
    if(p) *p = ':';
    return;
  }

  /* output first line if we definitely want it */
  if(readable == BOTH) net_output(stdout, host, command, line1);

  /* read response lines */
  while(fgets(line, BUF_SIZ, net)) {
    printfirst = 0;
    net_output(stdout, host, command, line);
  }

  /* print the first line if it's human-readable and we only got one */
  if(readable == HUMAN && printfirst) net_output(stdout, host, command, line1);

  /* all done, dc socket */
  fclose(net);

  freeaddrinfo(addr);

  if(p) *p = ':';
}

int main(int argc, char **argv) {
  int c;
  int i, j;

  if(argc <= 2) {
    usage();
    return 1;
  }

  argv0 = argv[0];

  opterr = 1;

  /* parse some options */
  while((c = getopt(argc, argv, "aAbc:C:hn:N:ms")) != -1) {
    switch(c) {
    case 'a':
      show_host = 1;
      break;
    case 'A':
      show_cmd = 1;
      break;
    case 'b':
      readable = BOTH;
      break;
    case 'c':
      read_list(&command, &commands, optarg);
      break;
    case 'C':
      add_list(&command, &commands, optarg);
      break;
    case 'h':
      usage();
      return 1;
      break;
    case 'm':
      readable = MACHINE;
      break;
    case 'n':
      read_list(&node, &nodes, optarg);
      break;
    case 'N':
      add_list(&node, &nodes, optarg);
    case 's':
      simultaneity = !simultaneity;
      break;
    default:
      return 1;
      break;
    }
  }

  /* add the node name */
  if(optind < argc) add_list(&node, &nodes, argv[optind++]);

  /* add the commands */
  for(i = optind; i < argc; i++) {
    add_list(&command, &commands, argv[i]);
  }

  /* now decide in which order to run this show */
  switch(simultaneity) {
  case HOST_FIRST:/* all commands for the host, then next host, etc.*/
    for(i = 0; i < nodes; i++) {
      for(j = 0; j < commands; j++) {
        run_query(node[i], command[j]);
      }
    }
    break;
  case CMD_FIRST:/* all hosts for the command, then next command, etc. */
    for(j = 0; j < commands; j++) {
      for(i = 0; i < nodes; i++) {
        run_query(node[i], command[j]);
      }
    }
    break;
  }

  return 0;
}
