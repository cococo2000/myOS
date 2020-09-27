#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512
#define OS_SIZE_LOC 2
#define BOOT_LOADER_SIG_OFFSET 0x1fe
#define BOOT_LOADER_SIG_1 0x55
#define BOOT_LOADER_SIG_2 0xaa
#define BOOT_MEM_LOC 0x7c00
#define OS_MEM_LOC 0x1000

/* structure to store command line options */
static struct
{
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf64_Phdr *phdr, FILE *fp, int ph,
                      Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE *fp,
                          FILE *img, int *nbytes, int *first);
static void write_os_size(int nbytes, FILE *img);
static void write_user_thread_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE *fp,
                          FILE *img, int *nbytes, int *first);
int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph, nbytes = 0, first = 0;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    img = fopen(IMAGE_FILE,"wb");   //创建空白的image文件用于后续写入
    /* for each input file */
    while (nfiles-- > 0) {

        /* open input file */
        fp = fopen(*files, "rb");
        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
        }
        fclose(fp);
        files++;
    }
    write_os_size(nbytes, img);
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr *ehdr, FILE *fp)
{
    fread(ehdr,sizeof(Elf64_Ehdr),1,fp);
}

static void read_phdr(Elf64_Phdr *phdr, FILE *fp, int ph,
                      Elf64_Ehdr ehdr)
{
    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
    fread(phdr, sizeof(Elf64_Phdr), 1, fp);
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE *fp,
                          FILE *img, int *nbytes, int *first)
{
    char buffer[512] = "";                          //存放要写入image的内容
    printf("phdr:\noffset = 0x%lx\nfilesz = 0x%lx\nmemsz = 0x%lx\n",phdr.p_offset,phdr.p_filesz,phdr.p_memsz);
    fseek(fp, phdr.p_offset, SEEK_SET);
    fread(buffer, phdr.p_filesz, 1, fp);
    for(int i = phdr.p_filesz; i < 0x200; i++){
        buffer[i] = 0;
    }
    printf("%s\n",buffer);
    fseek(img, 0x200 * (*first), SEEK_SET);
    fwrite(buffer, 0x200, 1, img);
    *nbytes += 512;
    (*first)++;
}

static void write_os_size(int nbytes, FILE *img)
{
    fseek(img, BOOT_LOADER_SIG_OFFSET - OS_SIZE_LOC, SEEK_SET);
    char others[4] = {0x01, 0x00, BOOT_LOADER_SIG_1, BOOT_LOADER_SIG_2}; //将kernel的扇区数写到image中
    int kernel_size = (nbytes + 511)/ 512;    //kernel占据的扇区数
    others[0] = (char)kernel_size;
    others[1] = (char)(kernel_size / 256);
    fwrite(others, 4, 1, img);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
