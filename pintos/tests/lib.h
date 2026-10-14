#ifndef TESTS_LIB_H
#define TESTS_LIB_H

#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <syscall.h>

extern const char *test_name;
extern bool quiet;

void msg (const char *, ...) PRINTF_FORMAT (1, 2);
void fail (const char *, ...) PRINTF_FORMAT (1, 2) NO_RETURN;

/* 성공 여부를 판단할 식 SUCCESS와, printf 형식 인자를 포함할 수
   있는 메시지를 받음. 메시지를 기록한 다음 식을 검사함. 결과가
   0(실패)이면 메시지를 실패로 출력함.

   쓰기가 조금 까다로움:

   - SUCCESS가 메시지에 영향을 주는 부수 효과를 가지면 안 됨.
   그러면 원래 메시지와 실패 메시지가 달라짐.

   - 메시지 자체도 부수 효과가 있으면 안 됨. 실패하면 두 번
   출력되고, quiet가 설정돼 있으면 성공 시 한 번도 출력되지
   않기 때문임. */
#define CHECK(SUCCESS, ...)                     \
        do                                      \
          {                                     \
            msg (__VA_ARGS__);                  \
            if (!(SUCCESS))                     \
              fail (__VA_ARGS__);               \
          }                                     \
        while (0)

void shuffle (void *, size_t cnt, size_t size);

void exec_children (const char *child_name, pid_t pids[], size_t child_cnt);
void wait_children (pid_t pids[], size_t child_cnt);

void check_file_handle (int fd, const char *file_name,
                        const void *buf_, size_t filesize);
void check_file (const char *file_name, const void *buf, size_t filesize);

void compare_bytes (const void *read_data, const void *expected_data,
                    size_t size, size_t ofs, const char *file_name);

#endif /* test/lib.h */
