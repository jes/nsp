/* nspd - Network Statistics Protocol Daemon

   James Stanley */

#include <sys/param.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void usage(void) {
  fprintf(stderr,
          "nspd - Network Statistics Protocol Daemon\n"
          "Usage: nspd [-d directory] [-h]\n"
          "\n"
          "Options:\n"
          "  -d directory   Run commands in directory.\n"
          "  -h             Display this help.\n"
          "Note that this program should be run from inetd.\n"
          );
}

int main(int argc, char **argv) {
  char path[2 + 1024 + 1] = "./";/* "./", command, nul */
  char *cmd = path + 2;/* the command portion of the path */
  char *p;
  int c;

  /* getopt can print our errors for us */
  opterr = 1;
  
  /* parse some options */
  while((c = getopt(argc, argv, "d:h")) != -1) {
    switch(c) {
    case 'd':/* set directory for commands */
      chdir(optarg);
      break;
    case 'h':/* print help */
      usage();
      return 1;
      break;
    default:/* uh-oh! what do we do here? */
      fprintf(stderr, "fail\n");
      fprintf(stderr, "Error parsing options.\n");
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
      printf("Command name too long.\n");
    }
    return 1;
  }

  /* ensure it's allowed */
  if(strstr(cmd, "..")
     || (*cmd == '.')
     || strchr(cmd, '/')
     || strchr(cmd, '\\')) {
    printf("forbid\n");
    printf("You are not allowed to run queries like that.\n");
    return 1;
  }

  /* execute */
  execl(path, cmd, NULL);

  /* failure */
  printf("unknown\n");
  perror("execl");

  return 1;
}
