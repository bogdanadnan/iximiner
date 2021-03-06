//
// Created by Haifa Bogdan Adnan on 04/08/2018.
//

#include "../common/dllexport.h"
#include "../common/common.h"
#include <getopt.h>
#include "arguments.h"

arguments::arguments(int argc, char **argv) {
    __argv_0 = argv[0];

    __init();

    int c = 0;
    char buff[50];

    if(argc < 2) {
        __help_flag = true;
        return;
    }

    while (c != -1)
    {
        static struct option options[] =
        {
            {"help", no_argument,  NULL, 'h'},
            {"verbose", no_argument, NULL, 'v'},
            {"mode", required_argument, NULL, 'm'},
            {"pool", required_argument, NULL, 'a'},
            {"port", required_argument, NULL, 'p'},
            {"wallet", required_argument, NULL, 'w'},
            {"name", required_argument, NULL, 'n'},
            {"cpu-intensity", required_argument, NULL, 'c'},
            {"gpu-intensity", required_argument, NULL, 'g'},
            {"gpu-filter", required_argument, NULL, 'd'},
            {"gpu-index", required_argument, NULL, 'd'},
			{"force-cpu-optimization", required_argument, NULL, 'o'},
			{"force-gpu-optimization", required_argument, NULL, 'f'},
			{"update-interval", required_argument, NULL, 'u'},
            {"report-interval", required_argument, NULL, 'r'},
			{"intensity-start", required_argument, NULL, 'y'},
			{"autotune-start", required_argument, NULL, 'y'},
            {"intensity-stop", required_argument, NULL, 'z'},
			{"autotune-stop", required_argument, NULL, 'z'},
			{"intensity-step", required_argument, NULL, 'q'},
			{"autotune-step", required_argument, NULL, 'q'},
            {"autotune-step-time", required_argument, NULL, 's'},
            {"hs-threshold", required_argument, NULL, 'i'},
            {"show-pool-requests", no_argument, NULL, 'k'},
            {"enable-api-port", required_argument, NULL, 'l'},
            {0, 0, 0, 0}
        };

        int option_index = 0;

        c = getopt_long (argc, argv, "hvm:a:p:w:n:c:g:d:o:f:u:r:y:z:q:s:i:j:kl:",
                         options, &option_index);

        switch (c)
        {
            case -1:
            case 0:
                break;
            case 1:
                sprintf(buff, "%s: invalid arguments",
                                  argv[0]);
                __error_message = buff;
                __error_flag = true;
                c = -1;
                break;
            case 'h':
                __help_flag = 1;
                break;
            case 'v':
                __verbose_flag = 1;
                break;
            case 'k':
                __show_pool_requests = true;
                break;
            case 'm':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    if(strcmp(optarg, "miner") == 0)
                        __miner_flag = 1;
                    else if(strcmp(optarg, "proxy") == 0)
                        __proxy_flag = 1;
                    else if(strcmp(optarg, "autotune") == 0)
                        __autotune_flag = 1;
                    else {
                        sprintf(buff, "%s: invalid arguments",
                                argv[0]);
                        __error_message = buff;
                        __error_flag = true;
                    }
                }
                break;
            case 'a':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __pool = optarg;
                }
                break;
            case 'p':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __proxy_port = atoi(optarg);
                }
                break;
            case 'w':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __wallet = optarg;
                }
                break;
            case 'n':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __name = optarg;
                    __auto_generated_name = false;
                }
                break;
            case 'c':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __cpu_intensity = atof(optarg);
                }
                break;
            case 'g':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    vector<string> gpu_intensity = __parse_multiarg(optarg);
                    for(vector<string>::iterator it = gpu_intensity.begin(); it != gpu_intensity.end(); it++) {
                        __gpu_intensity.push_back(atof(it->c_str()));
                    }
                }
                break;
            case 'd':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    string filter = optarg;
                    __gpu_filter = __parse_multiarg(filter);
                    __process_gpu_indexes();
                }
                break;
			case 'o':
				if (strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
					__help_flag = 1;
				}
				else {
					if (strcmp(optarg, "REF") == 0)
						__cpu_optimization = "REF";
#if defined(__x86_64__) || defined(_WIN64)
					else if (strcmp(optarg, "SSE2") == 0)
						__cpu_optimization = "SSE2";
					else if (strcmp(optarg, "SSSE3") == 0)
						__cpu_optimization = "SSSE3";
					else if (strcmp(optarg, "AVX") == 0)
						__cpu_optimization = "AVX";
					else if (strcmp(optarg, "AVX2") == 0)
						__cpu_optimization = "AVX2";
					else if (strcmp(optarg, "AVX512F") == 0)
						__cpu_optimization = "AVX512F";
#elif defined(__NEON__)
					else if (strcmp(optarg, "NEON") == 0)
						__cpu_optimization = "NEON";
#endif
					else {
						sprintf(buff, "%s: invalid arguments",
							argv[0]);
						__error_message = buff;
						__error_flag = true;
					}
				}
				break;
			case 'f':
				if (strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
					__help_flag = 1;
				}
				else {
				    vector<string> gpu_hashers = __parse_multiarg(optarg);
				    for(vector<string>::iterator st = gpu_hashers.begin(); st != gpu_hashers.end(); st++) {
				        string opt = *st;
                        if (opt == "OPENCL")
                            __gpu_optimization.push_back("OPENCL");
                        else if (opt == "CUDA")
                            __gpu_optimization.push_back("CUDA");
                        else if (opt == "AMDGCN")
                            __gpu_optimization.push_back("AMDGCN");
                        else {
                            sprintf(buff, "%s: invalid arguments",
                                    argv[0]);
                            __error_message = buff;
                            __error_flag = true;
                            break;
                        }
                    }
				}
			break;
			case 'u':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __update_interval = 1000000 * atoi(optarg);
                }
                break;
            case 'r':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __report_interval = 1000000 * atoi(optarg);
                }
                break;
            case 'y':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __gpu_intensity_start = atof(optarg);
                }
                break;
            case 'z':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __gpu_intensity_stop = atof(optarg);
                }
                break;
            case 'q':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __gpu_intensity_step = atof(optarg);
                }
                break;
            case 's':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __autotune_step_time = atoi(optarg);
                }
                break;
            case 'i':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __hs_threshold = atoi(optarg);
                }
                break;
            case 'l':
                if(strcmp(optarg, "-h") == 0 || strcmp(optarg, "--help") == 0) {
                    __help_flag = 1;
                }
                else {
                    __enable_api_port = atoi(optarg);
                }
                break;
            case ':':
                __error_flag = true;
                break;
            default:
                __error_flag = true;
                break;
        }
    }

	if (__miner_flag) {
        if (__gpu_intensity.size() == 0)
            __gpu_intensity.push_back(100);
	}
	else if (__autotune_flag) {
		__gpu_intensity.clear();
		__gpu_intensity.push_back(__gpu_intensity_start);
	}

	if (optind < argc)
    {
        sprintf(buff, "%s: invalid arguments",
                          argv[0]);
        __error_message = buff;
        __error_flag = true;
    }
}

bool arguments::valid(string &error) {
    error = __error_message;

    if(__error_flag)
        return false;

    if(__miner_flag == 1) {
        if (__pool.empty()) {
            error = "Pool address is mandatory.";
            return false;
        }

        if (__wallet.empty()) {
            error = "Wallet is mandatory.";
            return false;
        }

        if (__name.empty()) {
            error = "Worker name is mandatory.";
            return false;
        }

        if (__cpu_intensity < 0 || __cpu_intensity > 100) {
            error = "CPU intensity must be between 0 - disabled and 100 - full load.";
            return false;
        }

        for (vector<double>::iterator it = __gpu_intensity.begin(); it != __gpu_intensity.end(); it++) {
            if (*it < 0 || *it > 100) {
                error = "GPU intensity must be between 0 - disabled and 100 - full load.";
                return false;
            }
        }

        if (__update_interval < 2000000) {
            error = "Pool update interval must be at least 2 sec.";
            return false;
        }

        if (__report_interval < 1000000) {
            error = "Reporting interval must be at least 1 sec.";
            return false;
        }

        if(__enable_api_port != 0 && __enable_api_port < 1024) {
            error = "Iximiner API port must be at least 1024, lower port numbers are usually reserved by system and requires administrator privileges.";
            return false;
        }
    }
    else if(__autotune_flag == 1) {
        if (__gpu_intensity_start < 1 || __gpu_intensity_start > 100) {
            error = "GPU autotune start intensity must be between 1 and 100.";
            return false;
        }

        if (__gpu_intensity_stop < 1 || __gpu_intensity_stop > 100) {
            error = "GPU autotune stop intensity must be between 1 and 100.";
            return false;
        }

        if (__gpu_intensity_step < 1 || __gpu_intensity_step > 10) {
            error = "GPU autotune step intensity must be between 1 and 10.";
            return false;
        }

        if (__gpu_intensity_start > __gpu_intensity_stop) {
            error = "GPU autotune start intensity must be lower than GPU autotune stop intensity.";
            return false;
        }

        if (__autotune_step_time < 10) {
            error = "GPU autotune step time must be at least 10 seconds.";
            return false;
        }

        if(__gpu_optimization.size() > 1) {
            error = "In autotune mode you can only use one gpu hasher type (AMDGCN|CUDA|OPENCL).";
            return false;
        }
    }
    else if(__proxy_flag == 1) {
        if(__proxy_port < 1024) {
            error = "Proxy listening port must be at least 1024, lower port numbers are usually reserved by system and requires administrator privileges.";
            return false;
        }

        if (__pool.empty()) {
            error = "Pool address is mandatory.";
            return false;
        }

        if (__wallet.empty()) {
            error = "Wallet is mandatory.";
            return false;
        }

        if (__name.empty()) {
            error = "Worker name is mandatory.";
            return false;
        }

        if (__update_interval < 2000000) {
            error = "Pool update interval must be at least 2 sec.";
            return false;
        }
    }
    else  {
        error = "You need to specify an operation mode (miner/autotune/proxy).";
        return false;
    }

    return true;
}

bool arguments::is_help() {
    return __help_flag == 1;
}

bool arguments::is_verbose() {
    return __verbose_flag == 1;
}

bool arguments::is_miner() {
    return __miner_flag == 1;
}

bool arguments::is_autotune() {
    return __autotune_flag == 1;
}

bool arguments::is_proxy() {
    return __proxy_flag == 1;
}

int arguments::proxy_port() {
    return __proxy_port;
}

string arguments::pool() {
    return __pool;
}

string arguments::wallet() {
    return __wallet;
}

string arguments::name() {
    return __name;
}

double arguments::cpu_intensity() {
    return __cpu_intensity;
}

vector<double> &arguments::gpu_intensity() {
    return __gpu_intensity;
}

vector<string> arguments::gpu_filter() {
    return __gpu_filter;
}

string arguments::cpu_optimization() {
	return __cpu_optimization;
}

vector<string> arguments::gpu_optimization() {
	return __gpu_optimization;
}

int64_t arguments::update_interval() {
    return __update_interval;
}

int64_t arguments::report_interval() {
    return __report_interval;
}

double arguments::gpu_intensity_start() {
    return __gpu_intensity_start;
}

double arguments::gpu_intensity_stop() {
    return __gpu_intensity_stop;
}

double arguments::gpu_intensity_step() {
    return __gpu_intensity_step;
}

int64_t arguments::autotune_step_time() {
    return __autotune_step_time;
}

int arguments::hs_threshold() {
    return __hs_threshold;
}

bool arguments::show_pool_requests() {
    return __show_pool_requests;
}

string arguments::get_help() {
    return
            "\nIxian CPU/GPU Miner v." IxiMiner_VERSION_MAJOR "." IxiMiner_VERSION_MINOR "." IxiMiner_VERSION_REVISION "\n"
            "Copyright (C) 2019 Haifa Bogdan Adnan\n"
            "\n"
            "Usage:\n"
            "   - starting in miner mode:\n"
            "       iximiner --mode miner --pool <pool / proxy address> --wallet <wallet address> --name <worker name> --cpu-intensity <intensity> --gpu-intensity <intensity>\n"
            "   - starting in autotune mode:\n"
            "       iximiner --mode autotune --autotune-start <intensity> --autotune-stop <intensity> --autotune-step <intensity>\n"
            "   - starting in proxy mode:\n"
            "       iximiner --mode proxy --port <proxy port> --pool <pool address> --wallet <wallet address> --name <proxy name>\n"
            "\n"
            "Parameters:\n"
            "   --help: show this help text\n"
            "   --verbose: print more informative text during run\n"
            "   --mode <mode>: start in specific mode - arguments: miner / proxy\n"
            "           - miner: this instance will mine for ixian coins\n"
            "           - autotune: for finding best intensity for GPU mining\n"
            "           - proxy: this instance will act as a hub for multiple miners,\n"
            "                    useful to aggregate multiple miners into a single instance\n"
            "                    reducing the load on the pool\n"
            "   --pool <pool address>: pool/proxy address to connect to (eg. https://ixian.kiramine.com)\n"
            "   --wallet <wallet address>: wallet address\n"
            "                    this is optional if in miner mode and you are connecting to a proxy\n"
            "   --name <worker identifier>: worker identifier\n"
            "                    this is optional, will be autogenerated if is not provided\n"
            "   --port <proxy port>: proxy specific option, port on which to listen for clients\n"
            "                    this is optional, defaults to 8088\n"
            "   --enable-api-port <api port>: miner specific option, port on which to listen for api requests\n"
            "                    if enabled, you can get reports in json format at http://localhost:port/status\n"
            "                    this is optional, defaults to disabled (value 0)\n"
            "   --cpu-intensity: miner specific option, mining intensity on CPU\n"
            "                    value from 0 (disabled) to 100 (full load)\n"
            "                    this is optional, defaults to 100 (*)\n"
            "   --gpu-intensity: miner specific option, mining intensity on GPU\n"
            "                    value from 0 (disabled) to 100 (full load)\n"
            "                    this is optional, defaults to 100 (*)\n"
            "                    you can add more entries separated by comma for each GPU;\n"
            "                    in this case you need to add entries for all cards displayed,\n"
            "                    even if card is disabled by gpu-filter - use 0 for those\n"
            "   --gpu-filter: miner specific option, filter string for device selection\n"
            "                    it will select only devices that have in description the specified string\n"
            "                    this is optional, defaults to \"\"; you can add more entries separated by comma;\n"
            "                    if using multiple gpu hashers you can select specific filters for each like this:\n"
            "                    --gpu-filter CUDA:[1],CUDA:[2],OPENCL:AMD where [1], [2] and AMD are filters for cards\n"
			"   --force-cpu-optimization: miner specific option, what type of CPU optimization to use\n"
#if defined(__x86_64__) || defined(_WIN64)
			"                    values: REF, SSE2, SSSE3, AVX, AVX2, AVX512F\n"
#elif defined(__NEON__)
			"                    values: REF, NEON\n"
#else
			"                    values: REF\n"
#endif
			"                    this is optional, defaults to autodetect, change only if autodetected one crashes\n"
			"   --force-gpu-optimization: what type of GPU optimization/hasher to use; values: OPENCL, CUDA, AMDGCN\n"
			"                    this is optional, defaults to autodetect\n"
            "                    you can add more entries separated by comma\n"
            "   --hs-threshold: miner specific option, hashrate value under which miner will exit\n"
            "                    it will trigger after 5 displays in report in order to allow for miner warmup\n"
            "   --update-interval: how often should we update mining settings from pool, in seconds\n"
            "                    increasing it will lower the load on pool but will increase rejection rate\n"
            "                    this is optional, defaults to 2 sec and can't be set lower than that\n"
            "   --report-interval: how often should we display mining reports, in seconds\n"
            "                    this is optional, defaults to 10 sec\n"
            "   --show-pool-requests: miner specific option, show full requests sent to pool, debug purpose only\n"
            "   --autotune-start: autotune specific option, start intensity for autotuning (default 1)\n"
            "   --autotune-stop: autotune specific option, stop intensity for autotuning (default 100)\n"
            "   --autotune-step: autotune specific option, intensity steps for autotuning (default 1)\n"
            "   --autotune-step-time: autotune specific option, how much time should wait in a step\n"
            "                    before measuring h/s, in seconds (minimum 10, default 20)\n"
            "\n"
            "(*) Mining intensity depends on the number of CPU/GPU cores and available memory. Full load (100) is dynamically calculated by the application. You can use fractional numbers for better tunning.\n"
            ;
}

void arguments::__init() {
    __help_flag = 0;
    __verbose_flag = 0;
    __miner_flag = 0;
    __proxy_flag = 0;

    __pool = "";
    __wallet = "";
    __uid = generate_uid(8);
    __name = __uid;
    __auto_generated_name = true;
    __cpu_intensity = 100;
    __proxy_port = 8088;
    __update_interval = 2000000;
    __report_interval = 10000000;

    __gpu_intensity_start = 1;
    __gpu_intensity_stop = 100;
    __gpu_intensity_step = 1;
    __autotune_step_time = 20;

    __cpu_optimization = "";
	__gpu_optimization.clear();

    __hs_threshold = -1;

    __show_pool_requests = false;
    __enable_api_port = 0;

    __cards_count = 0;
    __error_flag = false;
}

string arguments::__argv_0 = "./";

string arguments::get_app_folder() {
    size_t last_slash = __argv_0.find_last_of("/\\");
	if (last_slash == string::npos)
		return ".";
    string app_folder = __argv_0.substr(0, last_slash);
    if(app_folder.empty()) {
        app_folder = ".";
    }
    return app_folder;
}

string arguments::get_app_name() {
    size_t last_slash = __argv_0.find_last_of("/\\");
    if (last_slash == string::npos)
        return __argv_0;

    string app_name = __argv_0.substr(last_slash + 1);
    if(app_name.empty()) {
        app_name = "iximiner";
    }
    return app_name;
}

vector<string> arguments::__parse_multiarg(const string &arg) {
    string::size_type pos, lastPos = 0, length = arg.length();
    vector<string> tokens;

    while(lastPos < length + 1)
    {
        pos = arg.find_first_of(",", lastPos);
        if(pos == std::string::npos)
        {
            pos = length;
        }

        if(pos != lastPos)
            tokens.push_back(string(arg.c_str()+lastPos,
                                        pos-lastPos ));

        lastPos = pos + 1;
    }

    return tokens;
}

void arguments::__process_gpu_indexes() {
    // if all are numbers and less than 17 than presume those are indexes and add []
    // this will work for rigs with at most 16 cards
    bool all_indexes = true;
    for(vector<string>::iterator fit = __gpu_filter.begin(); fit != __gpu_filter.end(); fit++) {
        if(!is_number(*fit) || atoi(fit->c_str()) > 16)  {
            all_indexes = false;
            break;
        }
    }

    if(all_indexes) {
        for(vector<string>::iterator fit = __gpu_filter.begin(); fit != __gpu_filter.end(); fit++) {
            *fit = "[" + *fit + "]";
        }
    }
}

bool arguments::is_autogenerated_name() {
    return __auto_generated_name;
}

int arguments::enable_api_port() {
    return __enable_api_port;
}

string arguments::uid() {
    return __uid;
}

string arguments::get_app_version() {
    return "iximiner_" IxiMiner_VERSION_MAJOR "_" IxiMiner_VERSION_MINOR "_" IxiMiner_VERSION_REVISION;
}
