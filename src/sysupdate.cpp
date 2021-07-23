#include <HTTPUpdate.h>
#include <LITTLEFS.h>

#include "cmd.h"

void reportProgress(size_t completed, size_t total)
{
    static int oldPhase = 1;
    int progress = (completed * 100) / total;

    int phase = (progress / 5) % 2; // report at 5% intervals

    if (phase != oldPhase)
    {
        client.printf("%3d%% (%d/%d)\n", progress, completed, total);
        oldPhase = phase;
    }
}

t_httpUpdate_return systemUpdate(const String &url)
{
    WiFiClient httpclient;

    httpUpdate.rebootOnUpdate(false);

    Update.onProgress(reportProgress);

    t_httpUpdate_return ret = httpUpdate.update(httpclient, url);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        client.printf("Update fail error (%d): %s\n",
                      httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        client.println("No update file available");
        break;

    case HTTP_UPDATE_OK:
        client.println("System update available - reboot to load");
        break;
    }
    return ret;
}

void sysupdate(int argc, char *argv[])
{
    if (argc != 2)
    {
        client.printf("specify source URL\n");
    }
    else
    {
        String url = argv[1];
        if (!url.startsWith("http://")) url = "http://" + url;
        systemUpdate(url);
    }
}

void getFile(const char* url, const char* target)
{
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK)
        {
            File f = LITTLEFS.open(target, "w+");
            if (f)
            {
                Stream& s = http.getStream();

                while (true)
                {
                    int c = s.read();
                    if (c < 0) break;
                    f.write(c);
                }
                f.close();
            }
            else
                client.printf("Could not open %s\n",target);
        }
        else
        {
            client.printf("Upload failed (%d)\n", httpCode);
        }
    }
    else
    {
        client.printf("GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
}

void wget(int argc, char *argv[])
{
    if ((argc < 2) || (argc > 3))
    {
        client.printf("wget url target\n");
    }
    else
    {
        String url = argv[1];
        if (!url.startsWith("http://"))
            url = "http://" + url;
        char *fname;
        if (argc == 2)
        {
            int i;
            for (i = strlen(argv[1]); i > 0; i--)
            {
                if (argv[1][i] == '/') break;
            }
            fname = strdup(&argv[1][i]);
        }
        else
        {
            fname = absfilename(argv[2]);
        }
        getFile(url.c_str(), fname);
        free(fname);
    }
}

void initSysUpdate()
{
    addToCmdTable(sysupdate, "sysupdate", "", "sysupdate URL");
    addToCmdTable(wget, "wget", "" , "wget URL [TARGET]");
}