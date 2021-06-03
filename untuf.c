#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


typedef struct tuf_header
{
   char sig[4];
   int32_t offset;
   char info[8];
   char date[16];
} tuf_header_t;

typedef struct tuf_node
{
   char path[128];
   int16_t a;
   int8_t b;
   int8_t compressed;
   int32_t orig_size;
   int32_t compressed_size;
   int32_t pad;
} tuf_node_t;


int main()
{
   tuf_node_t tn;
   tuf_header_t th;
   int pos = 0, len, rlen, ilen, fd = 0, ofd;
   char *s, buf[4096];

   read(fd, &th, sizeof(th));

   if (strcmp(th.sig, "TUF"))
   {
      fprintf(stderr, "* file seams not to be TUF file\n");
      exit(1);
   }

   printf("fileinfo: %s, %s\n", th.info, th.date);
   if ((pos = lseek(fd, th.offset, SEEK_SET)) == -1)
      perror("lseek"), exit(1);

   while (read(fd, &tn, sizeof(tn)) > 0)
   {
      len = tn.compressed ? tn.compressed_size : tn.orig_size;

      printf("0x%08x: %s, 0x%04x, 0x%02x, 0x%02x, %d, %d, %d\n", pos, tn.path, tn.a, tn.b, tn.compressed, tn.orig_size, tn.compressed_size, tn.pad);

      pos += sizeof(tn);

      for (s = tn.path; *s == '/'; s++);

      if (!len)
      {
         if (mkdir(s, 0777) == -1)
            perror("mkdir");
      }
      else
      {
         if (tn.a == 0x01010000)
            strcat(s, ".zz");

         if ((ofd = open(s, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
         {
            perror("open");
            exit(1);
         }

         for (; len > 0;)
         {
            rlen = len > sizeof(buf) ? sizeof(buf) : len;
            if ((ilen = read(fd, buf, rlen)) == -1)
            {
               perror("read");
               exit(1);
            }
            pos += ilen;

            write(ofd, buf, ilen);

            if (ilen < rlen)
               fprintf(stderr, "* truncated read\n");

            len -= rlen;
         }

         close(ofd);

         lseek(0, tn.pad, SEEK_CUR);
      }
   }
}

