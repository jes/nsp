/* nspd - Network Statistics Protocol Daemon

   James Stanley */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#ifndef VERSION
#define VERSION "(version unknown)"
#endif

void version(void) {
  printf("nspd %s\n", VERSION);
}

void usage(void) {
  printf(
  "nspd %s - network statistics protocol Daemon\n"
  "Usage: nspd [-d directory] [-hV]\n"
  "\n"
  "Options:\n"
  "  -d directory   Run commands in directory\n"
  "  -h             Display this help\n"
  "  -V             Show version\n"
  "Note that this program should be run from inetd.\n"
  , VERSION);
}

int main(int argc, char **argv) {
  char path[2 + 1024 + 1] = "./";/* "./", command, nul */
  char *cmd = path + 2;/* the command portion of the path */
  char *p;
  int c;
  struct stat stat_buf;

  /* getopt can print our errors for us */
  opterr = 1;

  /* parse some options */
  while((c = getopt(argc, argv, "d:hV")) != -1) {
    switch(c) {
    case 'd':/* set directory for commands */
      chdir(optarg);
      break;
    case 'h':/* print help */
      usage();
      return 0;
      break;
    case 'V':/* print version */
      version();
      return 0;
      break;
    default:/* uh-oh! what do we do here? */
      fprintf(stderr, "fail\n");
      fprintf(stderr, "failure: Error parsing options.\n");
      return 1;
      break;
    }
  }

  /* read a line */
  fgets(cmd, 1024, stdin);

  /* check that it fits and remove the endline */
  if((p = strchr(cmd, '\n'))) {
    *p = '\0';
  } else {
    if(!feof(stdin)) {
      printf("fail\n");
      printf("failure: Command name too long.\n");
    }
    return 1;
  }

  /* ensure it's allowed */
  if(strstr(cmd, "..")
     || (*cmd == '.')
     || strchr(cmd, '/')
     || strchr(cmd, '\\')) {
    printf("forbid\n");
    printf("forbidden: You are not allowed to run queries like '%s'.\n", cmd);
    return 1;
  }

  /* check that it exists */
  if(stat(path, &stat_buf) == -1) {
    printf("unknown\n");
    printf("unknown: The server does not know '%s'.\n", cmd);
    return 1;
  }

  /* execute */
  execl(path, cmd, (char *)NULL);

  /* failure */
  printf("fail\n");
  printf("failure: Unable to run query '%s'.\n", cmd);

  return 1;
}
