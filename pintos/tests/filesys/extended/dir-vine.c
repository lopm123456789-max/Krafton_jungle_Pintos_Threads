/* 아주 깊은 "덩굴" 모양 디렉터리 /dir0/dir1/dir2/...를 만들고,
   디스크가 꽉 찰 때까지 각 디렉터리에 일반 파일을 하나씩 만듦.

   그다음 대부분을 지움. 이유는 두 가지임. 첫째, "tar"는 파일
   이름을 100자로 제한함 (어렵지 않게 256자로 늘릴 수는 있음).
   둘째, 꽉 찬 디스크에는 tar 아카이브를 둘 자리가 없음. */

#include <string.h>
#include <stdio.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  int i;

  msg ("creating many levels of files and directories...");
  quiet = true;
  CHECK (mkdir ("start"), "mkdir \"start\"");
  CHECK (chdir ("start"), "chdir \"start\"");
  for (i = 0; ; i++) 
    {
      char name[3][READDIR_MAX_LEN + 1];
      char file_name[16], dir_name[16];
      char contents[128];
      int fd;

      /* 파일 생성. */
      snprintf (file_name, sizeof file_name, "file%d", i);
      if (!create (file_name, 0))
        break;
      CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
      snprintf (contents, sizeof contents, "contents %d\n", i);
      if (write (fd, contents, strlen (contents)) != (int) strlen (contents)) 
        {
          CHECK (remove (file_name), "remove \"%s\"", file_name);
          close (fd);
          break;
        }
      close (fd);
      
      /* 디렉터리 생성. */
      snprintf (dir_name, sizeof dir_name, "dir%d", i);
      if (!mkdir (dir_name)) 
        {
          CHECK (remove (file_name), "remove \"%s\"", file_name);
          break; 
        }

      /* 파일과 디렉터리 확인. */
      CHECK ((fd = open (".")) > 1, "open \".\"");
      CHECK (readdir (fd, name[0]), "readdir \".\"");
      CHECK (readdir (fd, name[1]), "readdir \".\"");
      CHECK (!readdir (fd, name[2]), "readdir \".\" (should fail)");
      CHECK ((!strcmp (name[0], dir_name) && !strcmp (name[1], file_name))
             || (!strcmp (name[1], dir_name) && !strcmp (name[0], file_name)),
             "names should be \"%s\" and \"%s\", "
             "actually \"%s\" and \"%s\"",
             file_name, dir_name, name[0], name[1]);
      close (fd);

      /* 디렉터리 안으로 들어감. */
      CHECK (chdir (dir_name), "chdir \"%s\"", dir_name);
    }
  CHECK (i > 200, "created files and directories only to level %d", i);
  quiet = false;

  msg ("removing all but top 10 levels of files and directories...");
  quiet = true;
  while (i-- > 10) 
    {
      char file_name[16], dir_name[16];

      snprintf (file_name, sizeof file_name, "file%d", i);
      snprintf (dir_name, sizeof dir_name, "dir%d", i);
      CHECK (chdir (".."), "chdir \"..\"");
      CHECK (remove (dir_name), "remove \"%s\"", dir_name);
      CHECK (remove (file_name), "remove \"%s\"", file_name);
    }
  quiet = false;
}
