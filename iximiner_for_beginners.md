# First steps in Ixian mining with iximiner

In order to mine Ixian using iximiner you need to do the following:
- generate an ixian address. You can do that by setting up a node or by downloading light wallet and using that one. You can fing more information at https://www.ixian.io/?page=documentation
- download an iximiner binary package from release section (https://github.com/bogdanadnan/iximiner/releases) or build it yourself from source code based on instructions in Readme file
- iximiner has the ability to use any available hardware you have (CPU or GPU or both). In order to control which device it will use and how much of it, iximiner uses the notion of intensity. In case of CPU, the intensity is the percent of cores that the miner will use. For example if you have a quadcore, and use an intensity of 50% for CPU, it will use 2 cores. For GPUs on the other hand, intensity is a much more complex stuff. It represents the percent of the total GPU memory used by the miner. Total amount of memory used divided by memory used by a single hash will give you the number of "threads" run on GPU (or individual hashes calculated in a GPU batch). Because of the way GPUs work internally, the hashrate doesn't increase proportional with intensity for GPUs. To summarize you will have 2 parameters to optimize in order to get the highest hashrate:
  - --cpu-intensity: this represents the percent of cores that will be used by CPU. Higher intensity means higher hashrate. If you run the miner with GPUs as well, and you have several cards, you might want to use a low intensity for CPU mining or even disabling it completely (by setting it to 0), because GPUs also need some CPU power to prepare the work for them. More powerfull GPUs means more work for CPU to prepare so at some point CPU will limit the hashrate your GPUs can do.
  - --gpu-intensity: this represents the intensity used on GPU. This is a very tricky one. Finding the best value manually is difficult. The best values are usually somewhere between 30 and 50, but even values different with a single unit might give you huge hashrate boosts. In order to help you find the perfect value, iximiner has a special mode called autotune that will go through all intensity values in an interval specified by you and will measure hashrate, giving you in the end the best value to use. I'll show you how to use it in a few moments.
- in order to use different devices, ariominer implements them as hashers. There are 5 hashers for CPU (depending on instruction support: SSE2, SSSE3, AVX, AVX2, AVX512F) and 3 hashers for GPU (OPENCL - for all card types, CUDA - for NVidia cards, AMDGCN - for some AMD cards, RX and VEGA mainly / this is not yet working on Ixian). Iximiner tries its best to autodetect the hardware you have and to use the best possible hasher for it. But for specific hardware/software configurations, autodetection might not work well. Especially on Windows systems, AVX2 instructions are not properly detected. You can force it to use specific hashers using the following arguments: 
  - --force-cpu-optimization <SSE2, SSSE3, AVX, AVX2, AVX512F> (I really suggest checking if your processor has AVX2 and if so use the flag to force it, AVX2 almost doubles hashrate for CPUs)
  - --force-gpu-optimization <OPENCL, CUDA, AMDGCN> (AMDGCN is not yet support on Ixian)
- having these informations, you can start mining using the follwing command line:
```
./iximiner --mode miner --pool <address of pool / https://ixian.kiramine.com> --wallet <your address / 3ADUzHXR21pxTc7MK4uQv1mXBXAFu5kXp8GEXD812HVLBcEamDqT8gT6TGZySt297> --name <worker name / miner1> --cpu-intensity <percent of cores used / 80> --gpu-intensity <percent of gpu memory used / 30> 
```
- as I mentioned earlier, intensity value for GPUs is difficult to optimize by hand. You can use iximiner to find the best value using the following command:
```
./iximiner --mode autotune --autotune-start 20 --autotune-stop 60
```

- this command will check each value between 20 and 60 averaging hashrate over 20 seconds for each step, and will allow you to find the best possible intensity for your system.

## Specific use cases and questions

### I have several cards and I want to use only some of them. 
In this case use --gpu-filter argument. Please keep in mind that this is actually a filter and accepts a string as a filter. When you start the miner it detects your cards and displays a list of them with an index in front (eg. [1] Intel - Iris Pro (1.5GB)). The filter argument will actually check that text if it has the filter in it. For example, --gpu-filter AMD will match all cards that have AMD in their name. Or --gpu-filter [1] will match all cards that have the text [1] in the name. You can specify multiple filters using comma separator. For example --gpu-filter [1],[2],[3] will use the cards having [1] or [2] or [3] in the name, so basically the first 3 cards in the system. Be careful not to use spaces between filters, just comma.

### I have a mix of cards in the system, all from the same vendor (NVidia or AMD). 
In this case the best intensity for each card is different. Same as for the filter, you can specify multiple intensity values by separating them with comma. They will be matched with the cards based on the order. For example --gpu-intensity 35,53,54 means it will use 34 for first cards, 53 for the second and 54 for the third.

### I have a mix of cards in the system from different vendors (both NVidia and AMD). 
This can be enabled by specifying multiple hashers in --force-gpu-optimization flag. For example --force-gpu-optimization CUDA,AMDGCN will use CUDA for NVIDIA cards and AMDGCN for AMD cards. This specific combination (CUDA,AMDGCN) can be used directly without any additional settings because each hasher will use different cards. Using a combination of CUDA with OPENCL or AMDGCN with OPENCL is much more tricky because OPENCL will probably autodetect the cards used by CUDA and AMDGCN as well and the miner will just crash in this case. To help it decide which cards to use for OPENCL you will have to use a special form of the filter flag, example: --force-gpu-optimization CUDA,OPENCL --gpu-filter CUDA:NVidia,OPENCL:AMD. This translates to: for CUDA use only cards having NVidia in the name and for OPENCL use only cards having AMD in the name.
### I have a card that is randomly crashing but the miner doesn't detect this and continues to mine without it until I restart. 
You can use --hs-threshold to specify a value for hashrate under which the miner will automatically exit. Using a loop in bash or bat file you can force it in this way to automatically restart if hashrate goes under your specified value. The hashrate needs to be under that value for at least 5 reports before the exit is triggered (~50 sec). It is built as such in order to allow for the system to stabilize at startup or after block change.
### I want to integrate this with specific mining monitor software, is there an API available? 
Yes there is, you need to use --enable-api-port <value greater than 1024> to enable it. Once you add this argument, you can get status reports at http://localhost:<api_port>/status link. This will return you a JSON with a lot of internal details. Btw, there is a hiveos package already built in case you want to use it, you can find it on release page (https://github.com/bogdanadnan/iximiner/releases).
### I have many small miners, is there any way to aggregate them in a single worker instead of directing them individually to the pool? - Proxy mode is not yet support on Ixian
That's the proxy mode for. Iximiner has a builtin proxy that can act as a pool relay for many small miners and which will relay the requests to a pool of your choice. There are changes related to dev fee when you run the miners through the proxy. In that case, the dev fee is disabled at the miner side and instead is collected by the proxy. That is needed because the proxy overwrites the wallet/pool settings sent by the miner with its own values, so dev fee can't be collected anymore from miner. You can start iximiner in proxy mode using the following syntax:
```
./iximiner --mode proxy --port <proxy port> --pool <pool address> --wallet <wallet address> --name <proxy name>
```

After you start it, redirect all your miners to point to the address and port of your proxy. The wallet used for the miners is irrelevant as it will be replaced by the wallet set on the proxy. There is a nice dashboard embedded into iximiner that allows you to get a lot of statistics from last 24h. You can check it out by visiting the proxy address in a browser: http://<proxy ip>:<proxy port> . Please keep in mind that proxy support is an experimental feature.
### Iximiner is trying to connect to coinfee.changeling.biz, what is this?
This site has the dev fee settings to use (dev wallet and pool to connect to during that 1 min period). I implement it as such in order to be able to change the wallet in case the current one becomes compromised or to change the pool to a specific one in the future. Please don't block the site, there is no malicious code run by iximiner. The source code is open and if you don't trust the binaries you can always compile it yourself and check the code. 
### How can I solo mine?
There is no support (yet) for solo mining. I might add it in the future as it is not a difficult task, but for the moment the need for it was not big enough.
### I only want to use CPU to mine, how can I disable GPU?
Set --gpu-intensity to 0, this will disable mining with GPU.
### I only want to use GPU to mine, how can I disable CPU?
Set --cpu-intensity to 0, this will disable mining with CPU.
  
  

