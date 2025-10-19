#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#pragma pack(push, 1)
typedef struct {             // Total: 54 bytes
  uint16_t  type;             // Magic identifier: 0x4d42
  uint32_t  size;             // File size in bytes
  uint16_t  reserved1;        // Not used
  uint16_t  reserved2;        // Not used
  uint32_t  offset;           // Offset to image data in bytes from beginning of file (54 bytes)
  uint32_t  dib_header_size;  // DIB Header size in bytes (40 bytes)
  int32_t   width_px;         // Width of the image
  int32_t   height_px;        // Height of image
  uint16_t  num_planes;       // Number of color planes
  uint16_t  bits_per_pixel;   // Bits per pixel
  uint32_t  compression;      // Compression type
  uint32_t  image_size_bytes; // Image size in bytes
  int32_t   x_resolution_ppm; // Pixels per meter
  int32_t   y_resolution_ppm; // Pixels per meter
  uint32_t  num_colors;       // Number of colors  
  uint32_t  important_colors; // Important colors
} BMPHeader;

typedef struct {
    BMPHeader header;
    unsigned char* data;
} BMPImage;

typedef struct {
    uint8_t rgbBlue;
    uint8_t rgbGreen;
    uint8_t rgbRed;
    uint8_t rgbReserved;
} RGBQUAD;
#pragma pack(pop)

unsigned char *buf;

#define TILESIZE 8
#define MAXFILENAMELEN 100

int
main(int argc, char **argv)
{
    if (argc != 2){
        perror("Usage: maiko <input.icn>\n");
        return EXIT_FAILURE;
    }
    
    FILE *fin, *fout;
    char *filename = argv[1];
    char *dimensions;
    BMPImage bmp;
    unsigned int tiles_w, tiles_h;
    int bytes_per_row, bytes_per_row_padded; // width, height is in tiles
    int image_size_bytes = 0; 
    int wp, hp; // width, height is in pixels

    // calculate size of image from file name
    // for now we always assume that the title is "untitled"
    dimensions = filename + strlen("untitled");
    sscanf(dimensions, "%x%*c%x", &tiles_w, &tiles_h);

    wp = tiles_w * TILESIZE;
    printf("wp = %d\n", wp);
    hp = tiles_h * TILESIZE;
    printf("hp = %d\n", hp);

    bytes_per_row = (wp + 7) / 8;
    printf("bytes_per_row = %d\n", bytes_per_row);
    bytes_per_row_padded = (bytes_per_row + 3) & (~3); // round up to 4
    printf("bytes_per_row_padded = %d\n", bytes_per_row_padded);

    image_size_bytes = bytes_per_row_padded * hp;

    char bmp_buf[image_size_bytes];
    memset(bmp_buf, 0, image_size_bytes);

    char outfilename[MAXFILENAMELEN];
    char *name = strrchr(filename, '.');
    printf("name = %s\n", name);
    strncpy(outfilename, filename, name - filename);
    strcat(outfilename, ".bmp");
    outfilename[strlen(outfilename) + 1] = '\0';

    // read bitmap from file into buffer
    fin = fopen(filename, "rb");
    printf("in-filename = %s\n", filename);
    fout = fopen(outfilename, "wb");
    printf("out-filename = %s\n", outfilename);

    if (fin == NULL){
        perror("fin does not exist\n");
        return EXIT_FAILURE;
    }
    if (fout == NULL){
        perror("fout does not exist\n");
        return EXIT_FAILURE;
    }

    for (unsigned int ty = 0; ty < tiles_h; ++ty){
        for (unsigned int tx = 0; tx < tiles_w; ++tx){
            char tile_data[8];
            unsigned int dst_y, bmp_row, dst_x_bits,
                byte_offset, buf_index;
            fread(tile_data, sizeof(char), 8, fin);

            for (int r = 0; r < 8; ++r){
                char byte = tile_data[r];
                dst_y = ty * TILESIZE + r;
                bmp_row = (hp - 1) - dst_y;

                dst_x_bits = tx * TILESIZE;
                byte_offset = dst_x_bits / 8;

                buf_index = bmp_row * bytes_per_row_padded + byte_offset;
                bmp_buf[buf_index] = byte;
            }
        }
    }

    RGBQUAD color_table[2] = {
        {0xFF, 0xFF, 0xFF, 0x00},
        {0x00, 0x00, 0x00, 0x00}
    };
    
    bmp.header.type = 0x4d42;
    bmp.header.offset = sizeof(BMPHeader) + sizeof(RGBQUAD) * 2;
    bmp.header.size = image_size_bytes + bmp.header.offset;
    bmp.header.reserved1 = 0;
    bmp.header.reserved2 = 0;
    bmp.header.image_size_bytes = image_size_bytes;
    bmp.header.dib_header_size = 40;
    bmp.header.width_px = wp;
    bmp.header.height_px = hp;
    bmp.header.num_planes = 1;
    bmp.header.bits_per_pixel = 1;
    bmp.header.compression = 0; // no compression
    bmp.header.x_resolution_ppm = 1;
    bmp.header.y_resolution_ppm = 1;
    bmp.header.num_colors = 2;
    bmp.header.important_colors = 2;

    fwrite(&bmp.header, sizeof(bmp.header), 1, fout);
    fwrite(&color_table, sizeof(color_table), 1, fout);
    fwrite(bmp_buf, sizeof(bmp_buf), 1, fout);
    printf("Writing %d bytes of image data\n", image_size_bytes);
    fflush(fout);

    fclose(fin);
    fclose(fout);

    return EXIT_SUCCESS;
}
