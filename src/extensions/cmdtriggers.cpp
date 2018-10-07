#include "halfmod.h"

#define CT_CONTINUE 0
#define CT_HANDLED  1
#define CT_COMPLETE 2
#define CT_RESERVED 1672

#define VERSION "v0.0.9"

extern "C" int onExtensionLoad(hmExtension &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.extensionInfo("Command Block Triggers",
                         "nigel",
                         "Hook events from vanilla command output",
                         VERSION,
                         "http://datapack.output.justca.me/in/the/console");
    return 0;
}

struct ConsoleTrigger
{
    bool active = false;
    std::string name;
    std::string plugin;
    int coords[2] = { 0, 0 };
    int (*callback)(hmHandle&,const std::string&,rens::smatch);
};

std::vector<ConsoleTrigger> consoleTriggers;

int onCMDTrigPluginEnd(hmHandle &handle)
{
    std::string plugin = handle.getPath();
    for (auto it = consoleTriggers.begin(), ite = consoleTriggers.end();it != ite;++it)
    {
        if ((it->active) && (it->plugin == plugin))
        {
            hmSendRaw("setblock ~" + std::to_string(it->coords[0]) + " 0 ~" + std::to_string(it->coords[1]) + " minecraft:bedrock replace");
            it->active = false;
            it->name.clear();
            it->plugin.clear();
            it->callback = nullptr;
        }
    }
    hmEvent event;
    if (handle.findEvent(CT_RESERVED,event))
        (*event.func)(handle);
    return 0;
}

int triggerCallback(hmHandle &handle, hmHook hook, rens::smatch args)
{
    int ret = 0;
    std::string name = hook.name.substr(3,std::string::npos);
    for (auto it = consoleTriggers.begin(), ite = consoleTriggers.end();it != ite;++it)
    {
        if ((it->active) && (it->name == name))
        {
            ret = (*it->callback)(handle,it->name,args);
            if (ret & CT_COMPLETE)
            {
                hmSendRaw("setblock ~" + std::to_string(it->coords[0]) + " 0 ~" + std::to_string(it->coords[1]) + " minecraft:bedrock replace");
                handle.unhookPattern(hook.name);
                //consoleTriggers.erase(it);
                it->active = false;
                it->name.clear();
                it->plugin.clear();
                it->callback = nullptr;
            }
            break;
        }
    }
    return (ret & CT_HANDLED);
}

int createNewTrigger(std::string name, std::string plugin, int (*callback)(hmHandle&,const std::string&,rens::smatch), int &x, int &y)
{
    x = y = 0;
    int d = 1, m = 1;
    bool placed = false;
    for (auto it = consoleTriggers.begin(), ite = consoleTriggers.end();it != ite;++it)
    {
        if (it->active)
        {
            if (name == it->name)
            {
                hmOutDebug("Error: A Console Trigger with the name: '" + name + "' already exists.");
                return 1;
            }
            if (2 * x * d < m)
                x += d;
            else if (2 * y * d < m)
                y += d;
            else
            {
                d *= -1;
                m++;
                x += d;
            }
        }
        else
        {
            placed = true;
            it->active = true;
            it->name = name;
            it->plugin = plugin;
            it->coords[0] = x;
            it->coords[1] = y;
            it->callback = callback;
            break;
        }
    }
    if (!placed)
    {
        ConsoleTrigger trigger;
        trigger.active = true;
        trigger.name = name;
        trigger.plugin = plugin;
        trigger.coords[0] = x;
        trigger.coords[1] = y;
        trigger.callback = callback;
        consoleTriggers.push_back(trigger);
    }
    return 0;
}

extern "C" void initConsoleTriggers(hmHandle &handle)
{
    hmEvent event;
    if (handle.findEvent(HM_ONPLUGINEND,event))
        event.func = &onCMDTrigPluginEnd;
    else
        handle.hookEvent(HM_ONPLUGINEND,&onCMDTrigPluginEnd);
    handle.hookEvent(CT_RESERVED,HM_ONPLUGINEND_FUNC,false);
}

extern "C" int genericConsoleTrigger(hmHandle &handle, std::string name, int (*callback)(hmHandle&,const std::string&,rens::smatch), std::string command)
{
    static rens::regex noExec ("^execute .*\\bas\\b.*\\brun\\b.*");
    if (rens::regex_match(command,noExec))
    {
        hmOutDebug("Error: Generic Console Triggers cannot contain commands that are executed as another entity!");
        return 1;
    }
    int x, z;
    if (createNewTrigger(name,handle.getPath(),callback,x,z))
        return 1;
    hmSendRaw("setblock ~" + std::to_string(x) + " 0 ~" + std::to_string(z) + " minecraft:repeating_command_block[facing=down]{CustomName:\"\\\\\"" + name + "\\\\\"\",auto:1b,Command:\"" + command + "\"} replace");
    handle.hookPattern("ct_" + name,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[(" + name + "): (.*)\\]$",&triggerCallback);
    return 0;
}

extern "C" int advancedConsoleTrigger(hmHandle &handle, std::string name, std::string pattern, int (*callback)(hmHandle&,const std::string&,rens::smatch), std::string command)
{
    int x, z;
    if (createNewTrigger(name,handle.getPath(),callback,x,z))
        return 1;
    hmSendRaw("setblock ~" + std::to_string(x) + " 0 ~" + std::to_string(z) + " minecraft:repeating_command_block[facing=down]{CustomName:\"\\\\\""+ name + "\\\\\"\",auto:1b,Command:\"" + command + "\"} replace");
    handle.hookPattern("ct_" + name,pattern,&triggerCallback);
    return 0;
}



















