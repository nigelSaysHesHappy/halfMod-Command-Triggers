# halfMod-Command-Triggers
This halfMod extension utilizes Command Blocks to allow halfMod and datapacks to communicate between eachother. `commandBlockOutput` must be enabled for this to work. Only opers can see it anyway!  

When you create a new hook with the extension, a new command block will be placed in the spawn chunks at y = 0. The x and z are calculated in a spiral pattern to take up as little space as possible. When a command block is no longer needed it is replaced with bedrock and that slot can now be used by the next created command.  

The idea is to put a command into the block that will only trigger when the datapack makes it meet the criteria.  

This is not a *perfect* system. Sometimes the command block will be able to activate more than once before the callback is able to reset the criteria. So be sure to take that into account when using this.  

# Usage

In your plugin's `onPluginStart` event, you will need to link and call the `initConsoleTriggers` function, otherwise the plugin will not be able to call the other functions:
```cpp
static void (*initConsoleTriggers)(hmHandle&);
static int (*genericConsoleTrigger)(hmHandle&,std::string,int (*)(hmHandle&,const std::string&,std::smatch),std::string);
static int (*advancedConsoleTrigger)(hmHandle&,std::string,std::string,int (*)(hmHandle&,const std::string&,std::smatch),std::string);

extern "C" int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("CmdTrigger Test",
                      "nigel",
                      "Probe!",
                      VERSION,
                      "http://aliens.made.you.justca.me/as/their/test/subject");
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
    return 1; // Extension not found.
}
```
By passing the plugin's handle to the extension, we are allowing it "plugin privilege" to create hooks and events. These are tied to the plugin, not the extension. The extension will also overwrite the plugin's `onPluginStop` function, but if the plugin had defined one, it will store it internally and ensure it is still called on unload.  

This allows the extension to safely remove any triggers the plugin created in its lifetime without trying to call invalid pointers later.  

# Callback Return Values

Callbacks can return a combination of two values OR'd together or 0, I like to define them like this:
```cpp
#define CT_CONTINUE 0
#define CT_HANDLED  1
#define CT_COMPLETE 2
```
If `CT_CONTINUE` is returned, nothing special happens. This is normal behavior, the trigger can be reused and any other registered hooks can be triggered from the command block's output.  

If `CT_HANDLED` is returned, the trigger will be reused, but no other hooks will be able to be triggered by the command block's output. This value should almost always be OR'd into your return value unless you have a particular reason not to.  

And finally, `CT_COMPLETE` will make the extension remove the trigger and reset the block back to bedrock. Only return this bit if you are finished with the trigger.  

Returning `CT_HANDLED|CT_COMPLETE` will remove the trigger and stop execution on the command block's output.  

# Generic Triggers

There are two types of triggers you can use.  

The first and easiest is the generic trigger:
```cpp
int genericConsoleTrigger(hmHandle &handle, std::string name, int (*callback)(hmHandle&,const std::string&,std::smatch), std::string command);
```
Pass the plugin's handle, to grant "plugin privileges" to the extension, name your trigger, provide the callback function, and provide the command to put into the command block.  

Example:
```cpp
int cb1(hmHandle &handle, const std::string &name, std::smatch args)
{
    hmSendRaw("say cb1 reporting for duty! " + args[2].str());
    return CT_CONTINUE;
}
extern "C" int onWorldInit(hmHandle &handle, std::smatch args)
{
    genericConsoleTrigger(handle,"cb1",&cb1,"data get entity @a[limit=1,tag=poop] Dimension");
    return 0;
}
```
You can test this trigger by running `/tag @s add poop`.  

The smatch list will be as follows:
+ 0 = Full output.
+ 1 = Name of the trigger.
+ 2 = Command block's output.  

Generic triggers cannot run commands executed as another entity as that will change the name of the executor and the hook will not match the output.

# Advanced Triggers

The other type of trigger you can use is the advanced trigger:
```cpp
int advancedConsoleTrigger(hmHandle &handle, std::string name, std::string pattern, int (*callback)(hmHandle&,const std::string&,std::smatch), std::string command);
```
The advanced trigger has no restrictions, but you have to define the regex pattern yourself.  

This allows much more flexibility, but makes it slightly harder to create these, especially on the fly.  

Example:
```cpp
int cb4(hmHandle &handle, const std::string &name, std::smatch args)
{
    hmSendRaw("say cb4 clocking out for the day! " + args[2].str());
    return CT_COMPLETE|CT_HANDLED;
}
extern "C" int onWorldInit(hmHandle &handle, std::smatch args)
{
    advancedConsoleTrigger(handle,"cb4","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[(.+): Added tag 'hmPearlObj' to Thrown Ender Pearl\\]$",&cb4,"execute as @a[tag=hmPearl,scores={hmPearl=1..}] run tag @e[type=minecraft:ender_pearl,limit=1,sort=nearest,tag=!handled,tag=!hmPearlObj] add hmPearlObj");
    return 0;
}
```
This example works just like the telepearl plugin, it uses this same technique internally to detect when a player throws an ender pearl. (Don't try this while the telepearl plugin is loaded.)  

For more examples and usage, check the included `cmdtest.cpp`. Create a dummy objective `tps_time` and set fakeplayer `#TPS_TIME`'s `tps_time` score to a number of seconds and when you set the `tps_calc` tag on an entity, the server will run a debug profile for that many seconds.

# Installation

Instation is very simple, no dependencies. Just copy the repo into your halfMod install and run these commands:
```sh
cd src/extensions
./compile.sh --install cmdtriggers.cpp
```
If you want to try out the example plugin:
```sh
cd src/plugins
./compile.sh --install disabled/cmdtest.cpp
```
