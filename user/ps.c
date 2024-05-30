#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(2, "ps: expected 1 argument, got %d\n", argc - 1);
        return -1;
    }

    if (strcmp(argv[1], "ppid") == 0) {printf("parent pid: %d\n", getppid());}
    else if (strcmp(argv[1], "count") == 0) {
        printf("%d\n", ps_list(0, 0));
        return 0;
    } else if (strcmp(argv[1], "clone") == 0) {
      int result = clone();
      if (result < 0) {
        return -1;
      }
      return 0;
    } else if (strcmp(argv[1], "pids") == 0 || strcmp(argv[1], "list") == 0) {
        int limit = 4;
        int* pids = malloc(limit * sizeof(int));
        int count = ps_list(limit, pids);
        while (count > limit) {
            free(pids);
            while (limit < count)
                limit *= 2;
            pids = malloc(limit * sizeof(int));
            count = ps_list(limit, pids);
        }
        if (strcmp(argv[1], "pids") == 0){
            for (int i = 0; i < count; ++i)
                printf("%d\n", pids[i]);
            return 0;
        }

        struct process_info info;
        printf("pid\tstate\t\tppid\tnmspc\tgpid\tgppid\to_files\tname\n");
        for (int i = 0; i < count; ++i) {
            if (ps_info(pids[i], &info) != 0) {
                return -1;
            }
            const char* state;
            switch (info.state) {
                case UNUSED:
                    state = "UNUSED  ";
                    break;
                case USED:
                    state = "USED    ";
                    break;
                case SLEEPING:
                    state = "SLEEPING";
                    break;
                case RUNNABLE:
                    state = "RUNNABLE";
                    break;
                case RUNNING:
                    state = "RUNNING ";
                    break;
                default:
                    state = "ZOMBIE  ";
                    break;
            }




            printf("%d\t%s\t%d\t%d\t%d\t%d\t%s\n",
                   pids[i], state, info.ppid, info.nmspace_id, info.getpid, info.getppid, info.name);
        }
        return 0;
    } else  {
        fprintf(2, "ps: unknown argument: %s\n", argv[1]);
        return -1;
    }
    return 0;
}