#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define SERVER "ftp://lebavui.io.vn/"

char question_file[256];

struct Memory
{
    char *data;
    size_t size;
};

size_t write_memory(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;

    struct Memory *mem = (struct Memory *)userp;

    mem->data = realloc(mem->data, mem->size + realsize + 1);

    memcpy(&(mem->data[mem->size]), contents, realsize);

    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

void reverse_string(char *s)
{
    int l = 0;
    int r = strlen(s) - 1;

    while (l < r)
    {
        char t = s[l];
        s[l] = s[r];
        s[r] = t;

        l++;
        r--;
    }
}

int main()
{
    CURL *curl;

    CURLcode res;

    char username[100];
    char password[100];

    printf("Username: ");
    scanf("%s", username);

    printf("Password: ");
    scanf("%s", password);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();

    if (!curl)
        return 1;

    // Lay danh sach file
    struct Memory list;

    list.data = malloc(1);
    list.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, SERVER);
    curl_easy_setopt(curl, CURLOPT_USERNAME, username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &list);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        printf("Login failed\n");
        return 1;
    }

    printf("Danh sach file:\n%s\n", list.data);

    char *p = strstr(list.data, "question_");

    if (!p)
    {
        printf("Khong tim thay file question\n");
        return 1;
    }

    sscanf(p, "%s", question_file);

    printf("Question file: %s\n", question_file);

    // Download question
    FILE *fp = fopen(question_file, "wb");

    char url[512];

    sprintf(url, "%s%s", SERVER, question_file);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    res = curl_easy_perform(curl);

    fclose(fp);

    if (res != CURLE_OK)
    {
        printf("Download failed\n");
        return 1;
    }

    printf("Downloaded\n");

    // Doc noi dung
    fp = fopen(question_file, "r");

    char content[1000] = "";

    char line[256];

    while (fgets(line, sizeof(line), fp) != NULL){
    strcat(content, line);
    }

    fclose(fp);

    reverse_string(content);
    // Tao answer
    char answer_file[256];

    sprintf(answer_file,"answer_%s",question_file + 9);

    fp = fopen(answer_file, "w");

    fprintf(fp, "%s", content);

    fclose(fp);

    printf("Created %s\n", answer_file);

    // Upload answer

    fp = fopen(answer_file, "rb");

    sprintf(url, "%s%s", SERVER, answer_file);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, fp);

    res = curl_easy_perform(curl);

    fclose(fp);

    if (res == CURLE_OK)
        printf("Upload success\n");
    else
        printf("Upload failed\n");

    curl_easy_cleanup(curl);

    curl_global_cleanup();

    return 0;
}
