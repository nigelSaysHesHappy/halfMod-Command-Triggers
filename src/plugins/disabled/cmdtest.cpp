#include "halfmod.h"
#include "str_tok.h"

#define CT_CONTINUE 0
#define CT_HANDLED  1
#define CT_COMPLETE 2

#define VERSION "v0.0.7"

static void (*initConsoleTriggers)(hmHandle&);
static int (*genericConsoleTrigger)(hmHandle&,std::string,int (*)(hmHandle&,const std::string&,std::smatch),std::string);
static int (*advancedConsoleTrigger)(hmHandle&,std::string,std::string,int (*)(hmHandle&,const std::string&,std::smatch),std::string);

int doTheLoop(hmHandle &handle, const hmPlayer &client, std::string args[], int argc);

extern "C" int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("CmdTrigger Test",
                      "nigel",
                      "Probe!",
                      VERSION,
                      "http://aliens.made.you.justca.me/as/their/test/subject");
    handle.regAdminCmd("hm_ttest",&doTheLoop,FLAG_ADMIN,"Setup a trigger test");
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "cmdtriggers")
        {
            *(void **) (&initConsoleTriggers) = it->getFunc("initConsoleTriggers");
            *(void **) (&genericConsoleTrigger) = it->getFunc("genericConsoleTrigger");
            *(void **) (&advancedConsoleTrigger) = it->getFunc("advancedConsoleTrigger");
            initConsoleTriggers(handle);
            return 0;
        }
    }
    return 1;
}

extern "C" int onPluginEnd(hmHandle &handle)
{
    hmSendRaw("say Unloaded!");
    return 0;
}

int cb1(hmHandle &handle, const std::string &name, std::smatch args)
{
    hmSendRaw("say cb1 reporting for duty! " + args[2].str());
    return CT_CONTINUE;
}

int cb2(hmHandle &handle, const std::string &name, std::smatch args)
{
    hmSendRaw("say cb2 at your service! " + args[2].str());
    return CT_HANDLED;
}

int cb3(hmHandle &handle, const std::string &name, std::smatch args)
{
    hmSendRaw("say all cb3 systems go! " + args[1].str() + ": " + args[2].str());
    return CT_HANDLED;
}

int cb4(hmHandle &handle, const std::string &name, std::smatch args)
{
    hmSendRaw("say cb4 clocking out for the day! " + args[2].str());
    return CT_COMPLETE|CT_HANDLED;
}

int cb5(hmHandle &handle, const std::string &name, std::smatch args)
{
    hmSendRaw("say cb5 is too drunx! " + args[2].str());
    return CT_COMPLETE;
}

int cbloop(hmHandle &handle, const std::string &name, std::smatch args)
{
    hmSendRaw("say Test #" + name + "\ntag @a remove " + std::to_string(stoi(name)-1));
    if (name == "0")
        hmSendRaw("tag @a remove loop");
    else if (name == "99")
        hmSendRaw("tag @a remove 99");
    return CT_COMPLETE;
}

int setupLoop(hmHandle &handle)
{
    int j = 0;
    if (!genericConsoleTrigger(handle,"0",&cbloop,"tag @a[tag=loop] add 0"))
        j++;
    for (int i = 1;i < 100;i++)
        if (!genericConsoleTrigger(handle,std::to_string(i),&cbloop,"tag @a[tag=" + std::to_string(i-1) + "] add " + std::to_string(i)))
            j++;
    return j;
}

int getDebugResults(hmHandle &handle, hmHook hook, std::smatch args)
{
    handle.unhookPattern(hook.name);
    std::string obj = gettok(hook.name,2,"\n"), seconds = gettok(hook.name,3,"\n"), realtime = args[1].str(), ticks = args[2].str(), tps = args[3].str();
    hmSendRaw("scoreboard players set TPS tps_time " + std::to_string(int(std::stof(tps)*100.0f)) + "\ntag @e[limit=1] add tps_done");
    hmSendRaw("say Requested " + seconds + " seconds; ran for " + realtime + " seconds and " + ticks + " ticks (" + tps + " tps)");
    return 1;
}

int startDebugTPS(hmHandle &handle, const std::string &name, std::smatch args);

int stopDebugTPS(hmHandle &handle, std::string args)
{
    // Stopped debug profiling after 8.90 seconds and 178 ticks (20.00 ticks per second)
    handle.hookPattern("debugPattern\n" + args,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Stopped debug profiling after ([^ ]+) seconds and ([^ ]+) ticks \\(([^ ]+) ticks per second\\)$",&getDebugResults);
    hmSendRaw("debug stop");
    std::smatch nada;
    startDebugTPS(handle,"stop",nada);
    return 1;
}

int startDebugTPS(hmHandle &handle, const std::string &name, std::smatch args)
{
    static bool running = false;
    if (!running)
    {
        std::regex ptrn ("^Set \\[(.+?)\\] for .+ to -?(\\d+)$");
        std::smatch ml;
        if (std::regex_match(args[2].str(),ml,ptrn))
        {
            std::string obj = ml[1].str(), seconds = ml[2].str();
            hmSendRaw("debug start\nscoreboard players reset @e[tag=tps_calc] tps_time\ntag @e[tag=tps_calc] remove tps_calc");
            handle.createTimer("debugTimer",std::stoi(seconds),&stopDebugTPS,obj + "\n" + seconds);
        }
    }
    running = !running;
    return CT_HANDLED;
}

int doTheLoop(hmHandle &handle, const hmPlayer &client, std::string args[], int argc)
{
    genericConsoleTrigger(handle,"tpsCalc",&startDebugTPS,"scoreboard players operation @e[tag=tps_calc] tps_time = #TPS_TIME tps_time");
    hmSendRaw("say Setup " + std::to_string(setupLoop(handle)) + " triggers!");
    return 0;
}

extern "C" int onWorldInit(hmHandle &handle, std::smatch args)
{
    genericConsoleTrigger(handle,"cb1",&cb1,"data get entity @a[limit=1,tag=poop] Dimension");
    genericConsoleTrigger(handle,"cb2",&cb2,"tag @a[tag=foo] remove foo");
    advancedConsoleTrigger(handle,"cb3","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[([^:]+): Set the weather to clear\\]$",&cb3,"execute as @a[tag=bar] run weather clear");
    advancedConsoleTrigger(handle,"cb4","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[(.+): Added tag 'hmPearlObj' to Thrown Ender Pearl\\]$",&cb4,"execute as @a[tag=hmPearl,scores={hmPearl=1..}] run tag @e[type=minecraft:ender_pearl,limit=1,sort=nearest,tag=!handled,tag=!hmPearlObj] add hmPearlObj");
    genericConsoleTrigger(handle,"cb5",&cb5,"give @a[tag=poo] minecraft:pumpkin");
    genericConsoleTrigger(handle,"tpsCalc",&startDebugTPS,"scoreboard players operation @e[tag=tps_calc] tps_time = #TPS_TIME tps_time");
    hmSendRaw("scoreboard objectives add tps_time dummy\nscoreboard players set #TPS_TIME tps_time 10\nsay Setup " + std::to_string(setupLoop(handle)) + " triggers!");
    return 0;
}
