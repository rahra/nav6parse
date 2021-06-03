/** Copyright 2021 Bernhard R. Fischer, 4096R/8E24F29D <bf@abenteuerland.at>
 *
 * This file is part of nav6parse.
 *
 * Nav6parse is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Nav6parse is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with smrender. If not, see <http://www.gnu.org/licenses/>.
 **/

/*! @file untuf.c
 *
 * This program is an uncompressor for the TUF file which is used to update the
 * Seatec NAV6 chartplotter.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>


/*! This is the definition of the TUF header.
 */
typedef struct tuf_header
{
   char sig[4];
   int32_t offset;
   char info[8];
   char date[16];
} tuf_header_t;


/*! This is the definition of the node header. A node is a directory or a file
 * and it can be either compressed are stored directly (as far as known yet).
 */
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


/*! This function is a zlib block decompressor. It is derived from here:
 * https://zlib.net/zlib_how.html
 */
int inf(unsigned char *in, int ilen, unsigned char *out, int olen)
{
   z_stream strm;
   int ret;

   /* allocate inflate state */
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;
   strm.avail_in = 0;
   strm.next_in = Z_NULL;
   ret = inflateInit(&strm);
   if (ret != Z_OK)
      return ret;

   strm.avail_in = ilen;
   strm.next_in = in;
   strm.avail_out = olen;
   strm.next_out = out;

   ret = inflate(&strm, Z_NO_FLUSH);

   if (ret == Z_STREAM_ERROR)
      fprintf(stderr, "Z_STREAM_ERROR\n"), exit(1);

   switch (ret)
   {
      case Z_NEED_DICT:
         ret = Z_DATA_ERROR;     /* and fall through */
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
         (void)inflateEnd(&strm);
         return ret;
   }

   if (ret == Z_STREAM_END)
      (void)inflateEnd(&strm);

   return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}


int main()
{
   tuf_node_t tn;
   tuf_header_t th;
   int pos = 0, len, ilen, fd = 0, ofd;
   char *s;
   unsigned char *in, *out;

   // read file header and check if it is TUF
   read(fd, &th, sizeof(th));

   if (strcmp(th.sig, "TUF"))
   {
      fprintf(stderr, "* file seams not to be TUF file\n");
      exit(1);
   }

   printf("fileinfo: %s, %s\n", th.info, th.date);
   if ((pos = lseek(fd, th.offset, SEEK_SET)) == -1)
      perror("lseek"), exit(1);

   // sequentially read nodes
   while (read(fd, &tn, sizeof(tn)) > 0)
   {
      len = tn.compressed ? tn.compressed_size : tn.orig_size;

      printf("0x%08x: %s, 0x%04x, 0x%02x, 0x%02x, %d, %d, %d\n", pos, tn.path, tn.a, tn.b, tn.compressed, tn.orig_size, tn.compressed_size, tn.pad);

      pos += sizeof(tn);

      for (s = tn.path; *s == '/'; s++);

      // if the len is 0, it is a directory.
      if (!len)
      {
         if (mkdir(s, 0777) == -1)
            perror("mkdir");
      }
      else
      {
         // allocate output buffer
         if ((in = out = malloc(tn.orig_size)) == NULL)
            perror("malloc"), exit(1);

         // if the data is compressed, allocate separate input buffer
         if (tn.compressed)
         {
            if ((in = malloc(tn.compressed_size)) == NULL)
               perror("malloc"), exit(1);
         }

         // create destination file
         if ((ofd = open(s, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
         {
            perror("open");
            exit(1);
         }

         // read data from TUF file
         if ((ilen = read(fd, in, len)) == -1)
            perror("read"), exit(1);

         if (ilen < len)
         {
            fprintf(stderr, "* truncated read\n");
            len = ilen;
         }

         pos += ilen;

         // uncompress it if it is compressed
         if (tn.compressed)
         {
            inf(in, ilen, out, tn.orig_size);
            free(in);
         }

         // and output data to new file
         write(ofd, out, tn.orig_size);
         free(out);
         close(ofd);

         // skip padding (TUF file is longword aligned, probably because of ARM architecture)
         lseek(0, tn.pad, SEEK_CUR);
      }
   }

   return 0;
}

