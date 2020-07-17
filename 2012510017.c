#include <stdio.h>
#include <string.h>
#include <json/json.h>

//basit bool tanımlaması
#define bool int
#define false 0
#define true !false

#define MB 1024

//dosyaların hazır olup olmadığı kontrol eder
//olmayan dosyanın indexini çıkarma yada, olmayan dosyada arama yapmamak için
bool datafile = false;
bool indexfile = false;

void open_command();
void create_index_command();
void search_command();
void close_command();

int main(void)
{

    char *cm = malloc(sizeof(char) * 64);
    memset(cm, 0, 64);

    while (true)
    {
        printf("enter command:");
        scanf("%s", cm);
        if (strcmp(cm, "open") == 0)
        {
            open_command();
        }
        else if (strcmp(cm, "create_index") == 0)
        {
            create_index_command();
        }
        else if (strcmp(cm, "search") == 0)
        {
            search_command();
        }
        else if (strcmp(cm, "close") == 0)
        {
            close_command();
            break;
        }
    }

    return 0;
}

//index turunu tutabilmek icin deger
typedef struct _indexRecord_
{
    void *key; //key degerini memoryde degisken bi eger olarak olarak tutacagım
    int p;     //recordun dosyada bulunan yeri
} indexRecord;

indexRecord *indexes; //

int recordCount;
int keyLength;

int sorting_comp(const void *, const void *);

char *jsonFileName;  //json dosya adını tutan deger
char *dataFileName;  //data dosya adını tutan deger
char *indexFileName; //index dosya adını tutan deger
int recordLength;    //data dosyasındaki her recordun uzunlugu
char *keyEncoding;   //key degerinin dosyaya yazım tarzı
int keyStart;        //key degerinin recordaki baslangıc indexi
int keyEnd;          //key degerinin recordaki bitis indexi
int orderType;       //key degerinin index dosyasındaki sıralama tarzı 0-asc 1-desc

void open_command()
{
    //command gelen degeri al ve kullan
    printf("enter filename: ");
    jsonFileName = malloc(sizeof(char) * 64);
    scanf("%s", jsonFileName);

    FILE *jsonf = fopen(jsonFileName, "r");
    if (jsonf == NULL)
    {
        printf("There isnt file such as %s\n", jsonFileName);
        return;
    }

    char *fileContent = malloc(sizeof(char) * MB);
    memset(fileContent, 0, MB);

    fread(fileContent, MB, 1, jsonf);
    fclose(jsonf);

    json_object *root = json_tokener_parse(fileContent);
    if (root == NULL)
    {
        printf("Something went wrong parsing json\n");
        return;
    }
    json_object *temp;
    char *tstr;
    int tlen;
    //--
    json_object_object_get_ex(root, "dataFileName", &temp);
    tstr = json_object_get_string(temp);
    tlen = strlen(tstr) + 1;
    dataFileName = malloc(sizeof(char) * tlen);
    memset(dataFileName, 0, tlen);
    strcpy(dataFileName, tstr);
    //--
    json_object_object_get_ex(root, "indexFileName", &temp);
    tstr = json_object_get_string(temp);
    tlen = strlen(tstr) + 1;
    indexFileName = malloc(sizeof(char) * tlen);
    memset(indexFileName, 0, tlen);
    strcpy(indexFileName, tstr);
    //--
    json_object_object_get_ex(root, "recordLength", &temp);
    recordLength = json_object_get_int(temp);
    //--
    json_object_object_get_ex(root, "keyEncoding", &temp);
    tstr = json_object_get_string(temp);
    tlen = strlen(tstr) + 1;
    keyEncoding = malloc(sizeof(char) * tlen);
    memset(keyEncoding, 0, tlen);
    strcpy(keyEncoding, tstr);
    //--
    json_object_object_get_ex(root, "keyStart", &temp);
    keyStart = json_object_get_int(temp);
    //--
    json_object_object_get_ex(root, "keyEnd", &temp);
    keyEnd = json_object_get_int(temp);
    //--
    json_object_object_get_ex(root, "order", &temp);
    tstr = json_object_get_string(temp);
    if (strcmp(tstr, "ASC") == 0)
    {
        orderType = 0;
    }
    else
    {
        orderType = 1;
    }
    //--

    keyLength = keyEnd - keyStart + 1;
    //index dosyası varsa oku ve indexes olustur
    FILE *inf = fopen(indexFileName, "rb");
    if (inf == NULL)
    {
        //index ddosyası olmadıgı için fonksiyondan çıkılacak
        return;
    }

    fseek(inf, 0, SEEK_END);
    int ti = (int)ftell(inf);
    fseek(inf, 0, SEEK_SET);
    recordCount = (ti) / (keyLength + sizeof(int));
    indexes = malloc(sizeof(indexRecord) * recordCount);
    int i;
    for (i = 0; i < recordCount; i++)
    {
        indexes[i].key = malloc(keyLength);
        fread(indexes[i].key, keyLength, 1, inf);
        fread(&indexes[i].p, sizeof(int), 1, inf);
    }
    fclose(inf);

    //sıralanmıs keyleri int veya string olarak gosteren fonk
    //for(i=0;i<recordCount;i++){
    //printf("%d \n",*((int*)indexes[i].key));
    //}
    //for(i=0;i<recordCount;i++){
    //printf("%s \n",(char*)indexes[i].key);
    //}
}

void create_index_command()
{

    FILE *df = fopen(dataFileName, "rb");
    fseek(df, 0, SEEK_END);
    int dflength = (int)ftell(df);

    //FILE*inf = fopen(indexFileName,"w+");

    recordCount = dflength / recordLength;

    indexes = malloc(sizeof(indexRecord) * recordCount);

    int i;
    for (i = 0; i < recordCount; i++)
    {

        indexes[i].key = malloc(keyLength);
        memset(indexes[i].key, 0, keyLength);

        int pos = i * recordLength + keyStart;
        fseek(df, pos, SEEK_SET);
        fread(indexes[i].key, keyLength, 1, df);

        indexes[i].p = pos;
    }
    fclose(df);

    //sorting time
    qsort(indexes, recordCount, sizeof(indexRecord), sorting_comp);

    //sıralanmıs keyleri int veya string olarak gosteren fonk
    //for(i=0;i<recordCount;i++){
    //printf("%d \n",*((int*)indexes[i].key));
    //}
    //for(i=0;i<recordCount;i++){
    //printf("%s \n",(char*)indexes[i].key);
    //}

    //sıra index arrayini dosyaya yazmaya geldi
    FILE *inf = fopen(indexFileName, "w+b");
    for (i = 0; i < recordCount; i++)
    {
        fwrite(indexes[i].key, keyLength, 1, inf);
        fwrite(&indexes[i].p, sizeof(int), 1, inf);
    }
    fclose(inf);
}

int binary_search(int i1, int i2, void *key)
{

    if (i2 < i1)
    { //recursive fonksiyonu eger bulamazsa -1 dondurecek
        return -1;
    }

    int mv = (int)((i1 + i2) / 2);

    if (strcmp(keyEncoding, "BIN") == 0)
    {
        //eger binary olarak yapılmıs ise int olarak karsılastırlacak
        int val = *((int *)indexes[mv].key);
        if (val == *((int *)key))
        {
            return mv; //aranan deger buluncu ve index donecek
        }
    }
    else
    {
        char *val = (char *)indexes[mv].key;
        if (strncmp(val, (char *)key, keyLength) == 0)
        {
            return mv; //eger karakter olarak deger bulunduysa index donecek
        }
    }
    //eger key halen bulunmadıysa
    bool flag = false;
    if (strcmp(keyEncoding, "BIN") == 0)
    {
        //int miş gibi karşılaştırma yap
        flag = *((int *)indexes[mv].key) > *((int *)key);
    }
    else
    {
        //string gibi karşılastır
        flag = strncmp((char *)indexes[mv].key, (char *)key, keyLength) > 0 ? true : false;
    }
    if (orderType == 1)
    {
        flag = !flag; //sıralamayı tersine çevir
    }

    if (!flag)
    {
        return binary_search(mv + 1, i2, key);
    }
    else
    {
        return binary_search(i1, mv - 1, key);
    }
}

void search_command()
{
    printf("enter key: ");
    int result_index = 0;
    if (strcmp(keyEncoding, "BIN") == 0)
    {
        //key olarak bi int okunacak
        int key;
        scanf("%d", &key);
        result_index = binary_search(0, recordCount - 1, &key);
    }
    else
    {
        //key olarak bi string okunacak
        char *key = malloc(sizeof(char) * keyLength);
        scanf("%s", key);
        result_index = binary_search(0, recordCount - 1, key);
    }

    if (result_index == -1)
    {
        printf("Not found given key\n");
        return;
    }

    printf("Index :%d File Poiter:%d\nRaw data from file will be printed char by char next line\n", result_index, indexes[result_index].p);

    char *rawData = malloc(recordLength);
    FILE *fd = fopen(dataFileName, "rb");
    fseek(fd, indexes[result_index].p, SEEK_SET);
    fread(rawData, recordLength, 1, fd);
    fclose(fd);

    int i;
    for (i = 0; i < recordLength; i++)
    {
        printf("%c", rawData[i]);
    }
    printf("\n");
    free(rawData);
}

void close_command()
{
    //butun memoryden alınan degerleri freele
    int i;
    for (i = 0; i < recordCount; i++)
    {
        free(indexes[i].key); //key degerini malloc ile aldım ve simdi freeleyerek geri bırakıyorum
    }
    free(indexes);

    free(jsonFileName);
    free(dataFileName);
    free(indexFileName);
    free(keyEncoding);
}

int sorting_comp(const void *d1, const void *d2)
{
    indexRecord *i1 = (indexRecord *)d1;
    indexRecord *i2 = (indexRecord *)d2;
    bool flag = false;
    if (strcmp(keyEncoding, "BIN") == 0)
    {
        //int miş gibi karşılaştırma yap
        flag = *((int *)i1->key) > *((int *)i2->key);
    }
    else
    {
        //string gibi karşılastır
        flag = strncmp((char *)i1->key, (char *)i2->key, keyLength) > 0 ? true : false;
    }
    if (orderType == 1)
    {
        flag = !flag; //sıralamayı tersine çevir
    }
    return flag;
}
