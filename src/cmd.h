typedef void (*cmdFunc)(int, char *[]);

struct cmdDescriptor
{
  const char *cmd;
  cmdFunc func;
  // void (*func)(int, char *[]);
  const char *description;
  const char *synopsis;
};

extern WiFiClient client;

extern void startCmdTask();
extern bool addToCmdTable(void (*func)(int, char *[]),
                          const char *name,
                          const char *descripton = "",
                          const char *helpText = "");