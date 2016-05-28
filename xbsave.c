#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef int bool;

typedef struct
{
    u8 map;
    u8 submap;
    u8 level; //Level of lead party member
    bool syssave;
    bool ngp;
    u16 st_year; //time the game was saved
    u8 st_month;
    u8 st_day;
    u8 st_hour;
    u8 st_minute;
    u8 st_second;
    u32 pt_hour;
    u8 pt_minute;
    u8 pt_second;
    u8 party[7];
    u8 guests[3];
} saveData;


saveData parseSave(u8 *saveFile)
{
    saveData save;
    const double conversion = 3600. / 0x1000;
    u32 hextime, seconds, i;

    hextime = ((saveFile[0x11eb0] << 24) | (saveFile[0x11eb1] << 16) | (saveFile[0x11eb2] << 8) | (saveFile[0x11eb3]));
    seconds = (hextime - (hextime % 0x1000)) * conversion; //get hours
    seconds += (hextime % 0x1000) * (3600. / 0xEFB); //get remaining seconds

    save.pt_hour = seconds / 3600;
    save.pt_minute = seconds % 3600 / 60;
    save.pt_second = seconds % 60;

    save.map = saveFile[0x2f];
    save.submap = saveFile[0x31];
    save.level = saveFile[0x85];
    save.syssave = saveFile[0x86];
    save.ngp = saveFile[0x87];
    save.st_year = (saveFile[0x24] << 8) | saveFile[0x25];
    save.st_month = saveFile[0x27];
    save.st_day = saveFile[0x29];
    save.st_hour = saveFile[0x28];
    save.st_minute = saveFile[0x22];
    save.st_second = saveFile[0x23];

    for (i = 0; i < 7; i++)
        save.party[i] = saveFile[0x37 + (4 * i)];

    for (i = 0; i < 3; i++)
        save.guests[i] = saveFile[0x5B + (4 * i)];

    return save;
}

void getData(saveData *save, char *newName, char mapName[][3][25])
{
    sprintf(newName + strlen(newName), "%dh %02dm %02ds Lv %i %s%s%s", save->pt_hour,
        save->pt_minute, save->pt_second, save->level, save->ngp ? "NG+ " : "",
        save->syssave ? "System Save - " : "", mapName[save->map][save->submap]);
}

void getParty(saveData *save, char *newName, char charName[][10], u32 precision)
{
    u32 i;

    sprintf(newName + strlen(newName), "; Party - %.*s", precision, charName[save->party[0]]);
    for (i = 1; i < 7; i++)
        if (save->party[i] != 0)
            sprintf(newName + strlen(newName), ", %.*s", precision, charName[save->party[i]]);

    if (save->guests[1] != 0)
        sprintf(newName + strlen(newName), "; Guests - %.*s", precision, charName[save->guests[0]]);
    for (i = 1; i < 3; i++)
        if (save->guests[i] != 0)
            sprintf(newName + strlen(newName), ", %.*s", precision, charName[save->guests[i]]);
}

#define HEIGHT 116
#define WIDTH 164
#define BLOCK_WIDTH 4
#define BLOCK_HEIGHT 4
#define WIDTH_IN_BLOCKS WIDTH / BLOCK_WIDTH
#define IMAGE_OFFSET 0xe0

u32 bmpPixelToTpl(u32 in)
{
    u32 x = in % WIDTH;
    u32 y = in / WIDTH;
    u32 blockX = x / BLOCK_WIDTH;
    u32 blockY = y / BLOCK_HEIGHT;
    u32 inBlockX = x % BLOCK_WIDTH;
    u32 inBlockY = y % BLOCK_HEIGHT;
    u32 inBlockOffset = inBlockY * BLOCK_HEIGHT + inBlockX;
    u32 blockNum = blockX + (blockY * WIDTH_IN_BLOCKS);
    u32 blockOffset = blockNum * BLOCK_HEIGHT * BLOCK_WIDTH;
    return blockOffset + inBlockOffset;
}

void convPic(char newName[], u8 saveFile[])
{
    FILE *fp;
    char imgname[180];
    u32 i;
    u8 out[0x94A0];
    u8 header[70] =
    {
        0x42, 0x4D, 0xE8, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x46, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0xA4, 0x00,
        0x00, 0x00, 0x8C, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x10, 0x00,
        0x03, 0x00, 0x00, 0x00, 0xA2, 0x94, 0x00, 0x00, 0x12, 0x0B,
        0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07,
        0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    sprintf(imgname, "%s.bmp", newName);

    for (i = 0; i < HEIGHT * WIDTH; i++)
    {
        u32 inOffset = bmpPixelToTpl(i) * 2 + IMAGE_OFFSET;
        u32 outOffset = i * 2;

        out[outOffset] = saveFile[inOffset + 1];
        out[outOffset + 1] = saveFile[inOffset];
    }

    fp = fopen(imgname, "wb");
    fwrite(&header, 1, 70, fp);
    fwrite(&out, 1, 0x94A0, fp);
    fclose(fp);
}

void printSave(saveData *save, char charName[][10], char mapName[][3][25])
{
    char party[34] = "", reserve[34] = "";

    if (save->party[0]) sprintf(party, "%s", charName[save->party[0]]);
    if (save->party[1]) sprintf(party + strlen(party), ", %s", charName[save->party[1]]);
    if (save->party[2]) sprintf(party + strlen(party), ", %s", charName[save->party[2]]);
    if (save->party[3]) sprintf(reserve, "%s", charName[save->party[3]]);
    if (save->party[4]) sprintf(reserve + strlen(reserve), ", %s", charName[save->party[4]]);
    if (save->party[5]) sprintf(reserve + strlen(reserve), ", %s", charName[save->party[5]]);
    if (save->party[6]) sprintf(reserve + strlen(reserve), ", %s", charName[save->party[6]]);

    printf("-----------------------------------------------------------\n"
        "-   Lv %02d %-16sPlay Time: %02u:%02u:%02u             -\n"
        "-   %-22sSave Time: %02d/%02d/%4u %02d:%02d:%02d  -\n"
        "-   %-22s%-32s-\n"
        "-   %-22s%-32s-\n"
        "-----------------------------------------------------------\n",
        save->level, charName[save->party[0]], save->pt_hour, save->pt_minute, save->pt_second,
        mapName[save->map][save->submap], save->st_month, save->st_day, save->st_year, save->st_hour, save->st_minute, save->st_second,
        save->syssave ? "System Save" : "", party,
        save->ngp ? "New Game +" : "", reserve);
}

void usage(const char *name)
{
    fprintf(stderr, "Usage: %s savefile [-rpiv] [-a abbreviation length]\n"
        "Options:\n"
        "    -r: Rename save file. Default unless only -i is set\n"
        "    -i: Extract the save image\n"
        "    -p: Add party info to file name\n"
        "    -v: Display the name of each file as it is renamed\n"
        "    -a name length: abbreviates party members' names to x chars\n"
        , name);
}

s32 main(s32 argc, char *argv[])
{
    FILE *fp;
    saveData save;
    char newName[180] = { 0 };
    char mapName[25][3][25];
    char charName[50][10];
    u8 saveFile[0x11eb4];
    u32 i, j;
    u32 doRename = 0, party = 0, image = 0, verbose = 0, dummy = 0, precision = 7;
    s32 opt;

    for (i = 0; i < 25; i++)
        for (j = 0; j < 3; j++)
            sprintf(mapName[i][j], "Area %i, Sublevel %i", i, j);
    sprintf(mapName[0][0], "Cleared Data");
    sprintf(mapName[1][1], "Colony 9");
    sprintf(mapName[1][2], "New Colony 9");
    sprintf(mapName[2][1], "Tephra Cave");
    sprintf(mapName[3][1], "Bionis\' Leg");
    sprintf(mapName[4][1], "Colony 6");
    sprintf(mapName[4][2], "Ether Mine");
    sprintf(mapName[5][1], "Satorl Marsh");
    sprintf(mapName[6][1], "Makna Forest");
    sprintf(mapName[7][1], "Frontier Village");
    sprintf(mapName[8][1], "Bionis\' Shoulder");
    sprintf(mapName[9][1], "High Entia Tomb");
    sprintf(mapName[10][1], "Eryth Sea");
    sprintf(mapName[11][1], "Alcamoth");
    sprintf(mapName[12][1], "Prison Island");
    sprintf(mapName[12][2], "Fallen Prison Island");
    sprintf(mapName[13][1], "Valak Mountain");
    sprintf(mapName[14][1], "Sword Valley");
    sprintf(mapName[15][1], "Galahad Fortress");
    sprintf(mapName[16][1], "Fallen Arm");
    sprintf(mapName[16][2], "Fallen Arm");
    sprintf(mapName[17][1], "Mechonis Field");
    sprintf(mapName[19][1], "Agniratha");
    sprintf(mapName[20][1], "Central Factory");
    sprintf(mapName[21][1], "Bionis\' Interior");
    sprintf(mapName[22][1], "Memory Space");
    sprintf(mapName[23][1], "Mechonis Core");
    sprintf(mapName[24][1], "Junks");

    for (i = 0; i < 50; i++)
        sprintf(charName[i], "Char %i", i);
    sprintf(charName[1], "Shulk");
    sprintf(charName[2], "Ryen");
    sprintf(charName[3], "Fiora");
    sprintf(charName[4], "Dunban");
    sprintf(charName[5], "Sharla");
    sprintf(charName[6], "Riki");
    sprintf(charName[7], "Melia");
    sprintf(charName[8], "Fiora");
    sprintf(charName[9], "Dickson");
    sprintf(charName[10], "Mumkhar");
    sprintf(charName[11], "Alvis");
    sprintf(charName[17], "Vanea");
    sprintf(charName[25], "Juju");
    sprintf(charName[26], "Otharon");
    sprintf(charName[27], "Dickson");
    sprintf(charName[28], "Dickson");
    sprintf(charName[42], "Alvis");

    while ((opt = getopt(argc, argv, "a:rpiv")) != -1)
    {
        switch (opt)
        {
        case 'a':
            precision = atoi(optarg);
            break;
        case 'r':
            doRename = 1;
            break;
        case 'p':
            party = 1;
            break;
        case 'i':
            image = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        default:
            usage(argv[0]);
            return 0;
            break;
        }
    }
    if (optind != argc - 1)
    {
        usage(argv[0]);
        return 0;
    }

    if (!doRename && !image)
        doRename = 1;
    if (party)
        doRename = 1;

    fp = fopen(argv[optind], "rb");
    if (fp == NULL)
    {
        printf("Could not open %s\n", argv[1]);
        return 0;
    }

    fseek(fp, 0L, SEEK_END);

    if (ftell(fp) != 0x28000)
    {
        printf("Not a Xenoblade save file\n");
        return 0;
    }

    fseek(fp, 0, SEEK_SET);
    fread(&saveFile, 0x11eb4, 1, fp);
    fclose(fp);

    if (memcmp(saveFile, "USRD", 4) != 0)
    {
        if (memcmp(saveFile, "DMMY", 4) == 0)
        {
            sprintf(newName, "Dummy save file");
            if (verbose)
                printf("Dummy save file\n");
            dummy = 1;
        }
        else
        {
            printf("Not a Xenoblade save file\n");
            return 0;
        }
    }

    if (!dummy)
    {
        save = parseSave(saveFile);

        if (doRename)
            getData(&save, newName, mapName);

        if (party)
            getParty(&save, newName, charName, precision);

        if (image)
            convPic(doRename ? newName : argv[optind], saveFile);

        if (verbose)
            printSave(&save, charName, mapName);
    }

    sprintf(newName + strlen(newName), ".xbsave");

    if (verbose && doRename) printf("Renaming to %s\n\n", newName);

    if ((doRename || dummy) && rename(argv[optind], newName))
        printf("Could not rename to %s\n\n", newName);

    return 0;
}