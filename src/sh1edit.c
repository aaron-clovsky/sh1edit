/*******************************************************************************
 * Extract/Decrypt or Encrypt/Insert BODYPROG.BIN into SH1 CD-ROM
 * Copyright (C) 2026 Aaron Clovsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

/*******************************************************************************
Headers
*******************************************************************************/
#include <sector.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
Types
*******************************************************************************/
typedef enum
{
    ARG_MODE_EXT  = 0,
    ARG_MODE_INS  = 1,
    ARG_MODE_EXTD = 2,
    ARG_MODE_INSE = 3
} arg_mode;

struct
{
    FILE *   in_cd;
    FILE *   in_file;
    FILE *   out;
    uint32_t start_sector;
    uint32_t size;
    arg_mode mode;
} args;

/*******************************************************************************
Utilities
*******************************************************************************/
/* Parse long int from string */
int parse_long(const char * arg, long * output, char ** endptr)
{
    char * s;

    errno = 0;

    *output = strtol(arg, &s, 10);

    if (errno != 0 || (const char *)s == arg)
    {
        return 0;
    }

    if (endptr)
    {
        *endptr = s;
    }

    return 1;
}

/* Print error and exit */
void perror_exit(const char * msg)
{
    perror(msg);

    exit(1);
}

/* Print help and exit */
void help_exit(const char * arg)
{
    const char * name;

    if ((!(name = strrchr(arg, '/'))))
    {
        name = strrchr(arg, '\\');
    }

    name = name ? &name[1] : arg;

    printf("Usage: %s <mode> ...\n", name);
    printf(" %s ext <input .bin> <sector> <size> <output file>\n", name);
    printf(" %s extd <input .bin> <sector> <size> <output file>\n", name);
    printf(" %s ins <input .bin> <sector> <input file> <output .bin>\n", name);
    printf(" %s inse <input .bin> <sector> <input file> <output .bin>\n", name);

    exit(2);
}

void init(int argc, const char ** argv)
{
    long value;

    /* Check args */
    if (argc != 6)
    {
        help_exit(argv[0]);
    }

    if (strcmp(argv[1], "ext") == 0)
    {
        args.mode = ARG_MODE_EXT;
    }
    else if (strcmp(argv[1], "extd") == 0)
    {
        args.mode = ARG_MODE_EXTD;
    }
    else if (strcmp(argv[1], "ins") == 0)
    {
        args.mode = ARG_MODE_INS;
    }
    else if (strcmp(argv[1], "inse") == 0)
    {
        args.mode = ARG_MODE_INSE;
    }
    else
    {
        fprintf(stderr, "Error: First argument must be 'ext' or 'ins'\n");

        exit(1);
    }

    /* Open input CD image */
    if ((!(args.in_cd = fopen(argv[2], "rb"))))
    {
        perror_exit("Error opening input image");
    }

    /* Check that the input CD image size is divisible by 2352 */
    {
        long size;

        if (fseek(args.in_cd, 0, SEEK_END) == -1)
        {
            perror_exit("Error determining size of input image");
        }

        if ((size = ftell(args.in_cd)) == -1)
        {
            perror_exit("Error determining size of input image");
        }

        if (size % 2352 != 0)
        {
            fprintf(stderr, "Error: Input image size not divisible by 2352\n");

            exit(1);
        }

        rewind(args.in_cd);
    }

    /* Parse start sector */
    if (!parse_long(argv[3], &value, NULL))
    {
        perror_exit("Error parsing start sector");
    }

    /* 300,000 is ~672MB, a reasonable-ish upper bound for a CD-ROM image */
    if (value < 0 || value > 300000)
    {
        fprintf(stderr, "Error: Invalid start sector: %ld\n", value);

        exit(1);
    }

    args.start_sector = (uint32_t)value;

    /* Open output file */
    if ((!(args.out = fopen(argv[5], "wb"))))
    {
        perror_exit("Error opening output file");
    }

    if (args.mode == ARG_MODE_EXT || args.mode == ARG_MODE_EXTD)
    {
        /* Parse size */
        if (!parse_long(argv[4], &value, NULL))
        {
            perror_exit("Error parsing size");
        }

        if (value < 0 || value > 705600000) /* ~672MB */
        {
            fprintf(stderr, "Error: Invalid size: %ld\n", value);

            exit(1);
        }

        args.size = (uint32_t)value;
    }
    else /* args.mode == ARG_MODE_INS || args.mode == ARG_MODE_INSE */
    {
        /* Open input file */
        if ((!(args.in_file = fopen(argv[4], "rb"))))
        {
            perror_exit("Error opening input file");
        }

        /* Check that the input file size is divisible by 256 */
        {
            long size;

            if (fseek(args.in_file, 0, SEEK_END) == -1)
            {
                perror_exit("Error determining size of input file");
            }

            if ((size = ftell(args.in_file)) == -1)
            {
                perror_exit("Error determining size of input file");
            }

            if (size < 0 || size > 705600000) /* ~672MB */
            {
                fprintf(stderr, "Error: Invalid file size: %ld\n", size);

                exit(1);
            }

            if (size % 256 != 0)
            {
                fprintf(stderr,
                        "Error: Input file size not divisible by 256\n");

                exit(1);
            }

            args.size = (uint32_t)size;

            rewind(args.in_file);
        }
    }
}

/*******************************************************************************
main()
*******************************************************************************/
int main(int argc, const char ** argv)
{
    sector_error error;
    char         sector[2352];
    sector_mode  mode;
    uint32_t *   data;
    unsigned     size;
    uint32_t     seed;
    unsigned     total;
    unsigned     sector_num;
    int          finished;
    unsigned     i;

    sector_num = 0;
    finished   = 0;
    seed       = 0;
    total      = 0;

    /* Process args */
    init(argc, argv);

    if (args.mode == ARG_MODE_EXT || args.mode == ARG_MODE_EXTD)
    {
        if (fseek(args.in_cd, 2352 * args.start_sector, SEEK_SET) == -1)
        {
            perror_exit("Error seeking to requested sector in image");
        }

        while (fread(&sector[0], 2352, 1, args.in_cd) == 1)
        {
            error = sector_analyze(&sector[0], (const void **)&data, &mode);

            if (error)
            {
                if (error == SECTOR_ERROR_MODE_2_F1_AMBIGUOUS ||
                    error == SECTOR_ERROR_MODE_2_F2_AMBIGUOUS)
                {
                    printf("Warning: sector_analyze_sector(%u): %s\n",
                           sector_num,
                           sector_error_string(error));
                }
                else
                {
                    fprintf(stderr,
                            "Error: sector_analyze_sector(%u): %s\n",
                            sector_num,
                            sector_error_string(error));

                    exit(1);
                }
            }

            switch (mode)
            {
                case SECTOR_MODE_0:        size = 2336; break;
                case SECTOR_MODE_1:        size = 2048; break;
                case SECTOR_MODE_2:        size = 2336; break;
                case SECTOR_MODE_2_FORM_1: size = 2048; break;
                case SECTOR_MODE_2_FORM_2: size = 2324; break;
                default:
                {
                    fprintf(stderr,
                            "Error: Sector %u: Unsupported Mode: %s\n",
                            sector_num,
                            sector_mode_string(mode));

                    exit(1);
                }
            }

            if (args.mode == ARG_MODE_EXTD)
            {
                for (i = 0; i < size >> 2; i++)
                {
                    /* From Fs_DecryptOverlay() in /src/main/fileinfo.c
                       https://github.com/Vatuu/silent-hill-decomp */
                    seed = (seed + 0x01309125) * 0x03A452F7;

                    data[i] ^= seed;

                    total += 4;

                    if (total == args.size)
                    {
                        size     = (i + 1) << 2;
                        finished = 1;
                        break;
                    }
                }
            }
            else
            {
                if (total + size > args.size)
                {
                    size     = args.size - total;
                    finished = 1;
                }

                total += size;
            }

            if (fwrite(data, size, 1, args.out) != 1)
            {
                perror_exit("Error writing output file");
            }

            if (finished)
            {
                break;
            }

            sector_num++;
        }
    }
    else /* args.mode == ARG_MODE_INS || args.mode == ARG_MODE_INSE */
    {
        char backup[2352];

        while (fread(&sector[0], 2352, 1, args.in_cd) == 1)
        {
            memcpy(&backup[0], &sector[0], 2352);

            if (sector_num < args.start_sector || finished)
            {
                if (fwrite(&sector[0], 2352, 1, args.out) != 1)
                {
                    perror_exit("Error writing output image");
                }

                sector_num++;

                continue;
            }

            error = sector_analyze(&sector[0], (const void **)&data, &mode);

            if (error)
            {
                if (error == SECTOR_ERROR_MODE_2_F1_AMBIGUOUS ||
                    error == SECTOR_ERROR_MODE_2_F2_AMBIGUOUS)
                {
                    printf("Warning: sector_analyze_sector(%u): %s\n",
                           sector_num,
                           sector_error_string(error));
                }
                else
                {
                    fprintf(stderr,
                            "Error: sector_analyze_sector(%u): %s\n",
                            sector_num,
                            sector_error_string(error));

                    exit(1);
                }
            }

            switch (mode)
            {
                case SECTOR_MODE_0:        size = 2336; break;
                case SECTOR_MODE_1:        size = 2048; break;
                case SECTOR_MODE_2:        size = 2336; break;
                case SECTOR_MODE_2_FORM_1: size = 2048; break;
                case SECTOR_MODE_2_FORM_2: size = 2324; break;
                default:
                {
                    fprintf(stderr,
                            "Error: Sector %u: Unsupported Mode: %s\n",
                            sector_num,
                            sector_mode_string(mode));

                    exit(1);
                }
            }

            {
                unsigned result;

                result = fread(data, 1, size, args.in_file);

                if (result != size)
                {
                    if (ferror(args.in_file))
                    {
                        perror_exit("Error reading input file");
                    }

                    if (result + total != args.size)
                    {
                        fprintf(stderr,
                                "Error: Unexpected end of input file\n");
                        exit(1);
                    }
                    else
                    {
                        size = result;
                    }
                }
            }

            if (args.mode == ARG_MODE_INSE)
            {
                for (i = 0; i < size >> 2; i++)
                {
                    /* From Fs_DecryptOverlay() in /src/main/fileinfo.c
                       https://github.com/Vatuu/silent-hill-decomp */
                    seed = (seed + 0x01309125) * 0x03A452F7;

                    data[i] ^= seed;

                    total += 4;

                    if (total == args.size)
                    {
                        size     = (i + 1) << 2;
                        finished = 1;
                        break;
                    }
                }
            }

            if (mode == SECTOR_MODE_1)
            {
                sector_mode_1 * mode_1;

                mode_1 = (sector_mode_1 *)&sector[0];

                mode_1->edc = sector_calc_edc(sector, mode);

                sector_calc_ecc(sector, mode, &mode_1->ecc[0]);
            }
            else if (mode == SECTOR_MODE_2_FORM_1)
            {
                sector_mode_2_form_1 * mode_2_f1;

                mode_2_f1 = (sector_mode_2_form_1 *)&sector[0];

                mode_2_f1->edc = sector_calc_edc(sector, mode);

                sector_calc_ecc(sector, mode, &mode_2_f1->ecc[0]);
            }

            if (fwrite(&sector[0], 2352, 1, args.out) != 1)
            {
                perror_exit("Error writing output image");
            }

            sector_num++;
        }
    }

    if (!finished)
    {
        fprintf(stderr, "Error: Unexpected end of image file\n");

        exit(1);
    }

    if (ferror(args.in_cd))
    {
        perror_exit("Error reading input file");
    }

    return 0;
}
