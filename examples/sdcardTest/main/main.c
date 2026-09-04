#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "sd_card_test.h"

static bool read_line(char *line, size_t line_size)
{
  static bool skip_lf = false;
  size_t length = 0;

  while (true)
  {
    char character;
    ssize_t result = read(STDIN_FILENO, &character, 1);

    if (result == 1)
    {
      if (skip_lf)
      {
        skip_lf = false;
        if (character == '\n')
        {
          continue;
        }
      }

      if (character == '\r' || character == '\n')
      {
        skip_lf = character == '\r';
        write(STDOUT_FILENO, "\n", 1);
        fsync(STDOUT_FILENO);
        line[length] = '\0';
        return true;
      }

      write(STDOUT_FILENO, &character, 1);
      fsync(STDOUT_FILENO);

      if (length + 1 < line_size)
      {
        line[length++] = character;
      }
      continue;
    }

    if (result == 0 || (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)))
    {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    return false;
  }
}

static void sd_card_menu(void)
{
  char line[128];

  while (true)
  {
    printf("\n=== SDcard === (%s)\n"
           "1  Initialize SDcard\n"
           "2  Format SDcard\n"
           "3  Write Testfile\n"
           "4  Append Testfile\n"
           "5  Read TestFile\n"
           "9  Exit\n",
           sd_card_test_is_present() ? "SDcard detected" : "No SDcard detected");
    write(STDOUT_FILENO, "SDcard choice => ", 17);
    fsync(STDOUT_FILENO);

    if (!read_line(line, sizeof(line)))
    {
      return;
    }

    char choice = 0;
    for (char *p = line; *p; p++)
    {
      if (isdigit((unsigned char)*p))
      {
        choice = *p;
        break;
      }
    }

    switch (choice)
    {
      case '1':
        if (sd_card_test_init() == ESP_OK)
        {
          puts("SDcard initialized");
        }
        break;
      case '2':
        if (sd_card_test_format() == ESP_OK)
        {
          puts("SDcard formatted");
        }
        break;
      case '3':
      {
        const char *prompt = "Enter text: ";
        write(STDOUT_FILENO, prompt, strlen(prompt));
        fsync(STDOUT_FILENO);
        if (read_line(line, sizeof(line)) && sd_card_test_write(line) == ESP_OK)
        {
          puts("Testfile written");
        }
        break;
      }
      case '4':
      {
        const char *prompt = "Enter text: ";
        write(STDOUT_FILENO, prompt, strlen(prompt));
        fsync(STDOUT_FILENO);
        if (read_line(line, sizeof(line)) && sd_card_test_append(line) == ESP_OK)
        {
          puts("Testfile appended");
        }
        break;
      }
      case '5':
        sd_card_test_read();
        break;
      case '9':
        return;
      default:
        puts("Use 1, 2, 3, 4, 5, or 9");
        break;
    }
  }
}

void app_main(void)
{
  setvbuf(stdout, NULL, _IONBF, 0);
  sd_card_menu();
  puts("SDcard test finished");
}
