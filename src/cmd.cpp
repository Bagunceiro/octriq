#include <Arduino.h>
#include <WiFiServer.h>
#include <vector>
#include <LITTLEFS.h>

#include "cmd.h"
#include "seqvm.h"

namespace pa
{
  char *optarg = (char *)0;
  int optind = 1;
  int posinarg = 0;

  void resetgetopt()
  {
    optarg = (char *)0;
    optind = 1;
    posinarg = 0;
  }

  int getopt(int argc, char *const argv[], const char *optstring)
  {
    static int posinarg = 0;
    int result = -1;

  doitagain:
    if (optind < argc)
    {
      char *arg = argv[optind];

      if (strcmp(arg, "-") == 0 || strcmp(arg, "--") == 0)
      {
        optind++;
        posinarg = 0;
        return -1;
      }

      if ((posinarg) || (arg[0] == '-'))
      {
        posinarg++;
        char opt = arg[posinarg];
        if (opt)
        {
          result = '?'; // Unless we find it later
          for (int j = 0; char c = optstring[j]; j++)
          {
            if (c != ':') // just ignore colons
            {
              if (opt == c)
              {
                result = opt; // matched

                bool wantsparam = (optstring[j + 1] == ':');
                if (wantsparam)
                {
                  optarg = &arg[posinarg + 1];
                  if (optarg[0] == '\00')
                  {
                    optind++;
                    if (optind < argc)
                    {
                      optarg = argv[optind];
                      optind++;
                      posinarg = 0;
                    }
                  }
                  else
                  {
                    optind++;
                    posinarg = 0;
                  }
                }
                return result;
              }
            }
          }
          return result;
        }
        else // Move on to the next arg
        {
          optind++;
          posinarg = 0;
          goto doitagain;
        }
      }
    }
    return result;
  }

}

TaskHandle_t cmdTask;

WiFiServer wifiServer(1685);
WiFiClient client;

void addArg(String &arg, std::vector<String> &argv)
{
  if (arg.length() > 0)
  {
    argv.push_back(arg);
  }
}

int parse(const char *line, std::vector<String> &argv)
{
  String arg;
  bool quoting = false;
  bool quotechar = false;

  int len = strlen(line);
  for (int i = 0; i < len; i++)
  {
    char c = line[i];

    if (c == '"' && !quotechar)
    {
      quoting = !quoting;
      continue;
    }
    else if (!quoting && !quotechar)
    {
      if (c == '\\')
      {
        quotechar = true;
        continue;
      }
      else if (isspace(c))
      {
        addArg(arg, argv);
        arg = "";
        continue;
      }
    }
    quotechar = false;
    arg += c;
  }
  addArg(arg, argv);

  return 0;
}

const char *prompt = "> ";

extern int listvms(Print &out);

void list(int argc, char *argv[])
{
  if (argc != 1)
  {
    client.printf("%s\n", argv[0]);
  }
  else
  {
    listvms(client);
  }
}

void kill(int argc, char *argv[])
{
  // extern int killvm(int);
  if (argc != 2)
  {
    client.printf("%s JOBNO ...\n", argv[0]);
  }
  else
  {
    for (int i = 1; i < argc; i++)
    {
      int jobno = atoi(argv[i]);
      if (jobno > 0)
      {
        int res = VM::kill(jobno);
        if (res >= 0)
        {
          client.printf("VM %s killed\n", argv[i]);
        }
        else
        {
          client.printf("VM %s not found\n", argv[i]);
        }
      }
      else
      {
        client.printf("VM number %s out of range\n", argv[i]);
      }
    }
  }
}

char* absfilename(char* n)
{
    char* fname = new char[strlen(n)+2];
    fname[0] = '\00';
    if (n[0] != '/') strcpy(fname, "/");
    strcat(fname, n);
    return fname;
}

void clr(int argc, char *argv[])
{
  VM::clr();
}

void traceon(int argc, char* argv[])
{
  // extern void vmsetTrace(int, bool);
  if (argc == 2)
  {
    VM::setTrace(atoi(argv[1]), true);
  }
  else
  {
    client.printf("%s VMNUMBER", argv[0]);
  }
}

void traceoff(int argc, char* argv[])
{
  // extern void vmsetTrace(int, bool);
  if (argc == 2)
  {
    VM::setTrace(atoi(argv[1]), false);
  }
  else
  {
    client.printf("%s VMNUMBER", argv[0]);
  }
}

void run(int argc, char *argv[])
{
  if (argc != 2)
  {
    Serial.printf("%s BINFILE", argv[0]);
  }
  else
  {
    char* fname = absfilename(argv[1]);

    int jobno = VM::runBinary(fname);
    delete fname;
    if (jobno >= 0)
    {
      client.printf("Job #%d\n", jobno);
    }
    else
    {
      client.printf("Failed\n");
    }
  }
}

void treeRec(File dir)
{
  if (dir)
  {
    dir.size();
    if (dir.isDirectory())
    {
      client.printf("%s :\n", dir.name());
      while (File f = dir.openNextFile())
      {
        treeRec(f);
        f.close();
      }
    }
    else
      client.printf(" %6.d %s\n", dir.size(), dir.name());

    dir.close();
  }
}

void rm(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    char* fname = absfilename(argv[i]);
    if (!((LITTLEFS.remove(fname) || LITTLEFS.rmdir(fname))))
    {
      client.printf("Could not remove %s\n", argv[i]);
    }
    delete fname;
  }
}

void tree(int argc, char *argv[])
{
  File dir = LITTLEFS.open("/");
  treeRec(dir);
  dir.close();
}

void exit(int argc, char *argv[])
{
  client.stop();
}

#define cmdEntry(C, D, S) \
  {                       \
#C, C, #D, #S         \
  }

void help(int argc, char *argv[]);

std::vector<cmdDescriptor> cmdTable = {
    // You can also add entries using "addToCmdTable(...)"
    cmdEntry(exit, , ),
    cmdEntry(help, , ),
    cmdEntry(run, , ),
    cmdEntry(list, , ),
    cmdEntry(kill, , ),
    cmdEntry(clr, , ),
    cmdEntry(traceon, , ),
    cmdEntry(traceoff, , ),
    cmdEntry(rm, , ),
    cmdEntry(tree, , )
};

void help(int argc, char *argv[])
{
  for (int i = 0; i < cmdTable.size(); i++)
  {
    client.print(cmdTable[i].cmd);
    client.print('\t');
    client.println(cmdTable[i].description);
  }
}

bool execute(std::vector<String> &args)
{
  bool result = true;
  pa::resetgetopt();

  if (args.size() >= 1)
  {
    const char *cmd = args[0].c_str();
    int argc = args.size();
    char *argv[argc];
    for (int i = 0; i < argc; i++)
    {
      argv[i] = (char *)malloc(args[i].length() + 1);
      strcpy(argv[i], args[i].c_str());
    }
    bool foundCmd = false;

    for (int i = 0; i < cmdTable.size(); i++)
    {
      if (strcmp(cmd, cmdTable[i].cmd) == 0)
      {
        foundCmd = true;
        cmdTable[i].func(argc, argv);
        break;
      }
    }
    if (!foundCmd)
    {
      client.printf("Command '%s' not found\n", cmd);
      result = false;
    }

    for (int i = 0; i < argc; i++)
    {
      free(argv[i]);
    }
  }

  return result;
}

String getCommand()
{
  String result;

  while (true)
  {
    if (client.available() > 0)
    {
      int c = (client.read());
      if (c == '\n')
        break;
      else if (c == '\r')
        ;
      else
      {
        if (isprint(c))
          result += (char)c;
        else
        {
        }
      }
    }
    else if (!client.connected())
    {
      client.stop();
      result = "";
      break;
    }
    delay(50);
  }
  return result;
}

void cmd(void *)
{
  while (true)
  {
    delay(5);
    client = wifiServer.available();
    bool conn = false;
    while (client)
    {
      if (!conn)
      {
        Serial.println("Client connected");
        conn = true;
      }
      std::vector<String> args;

      client.print(prompt);
      String l = getCommand();
      parse(l.c_str(), args);
      execute(args);
    }
    if (conn)
      Serial.println("Client disconnected");
  }
}

void startCmdTask()
{
  wifiServer.begin();
  xTaskCreate(
      cmd,     // Function to implement the task
      "cmd",   // Name of the task
      8192,    // Stack size in words
      NULL,    // Task input parameter
      0,       // Priority of the task
      &cmdTask // Task handle.
  );
}

bool addToCmdTable(cmdFunc func,
                   const char *name,
                   const char *description,
                   const char *helpText)
{
  cmdDescriptor desc = {name, func, description, helpText};
  cmdTable.push_back(desc);
  return true;
}
