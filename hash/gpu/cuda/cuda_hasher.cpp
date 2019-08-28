//
// Created by Haifa Bogdan Adnan on 03/08/2018.
//

#include "../../../common/common.h"
#include "../../../app/arguments.h"

#include "../../hasher.h"
#include "../../argon2/argon2.h"

#if defined(WITH_CUDA)

#include <cuda_runtime.h>
#include <driver_types.h>

#include "cuda_hasher.h"
#include "../../../common/dllexport.h"

cuda_hasher::cuda_hasher() {
	_type = "GPU";
	_subtype = "CUDA";
	_short_subtype = "NVD";
	_priority = 2;
	_intensity = 0;
	__running = false;
	_description = "";
}


cuda_hasher::~cuda_hasher() {
	this->cleanup();
}

bool cuda_hasher::initialize() {
	cudaError_t error = cudaSuccess;
	string error_message;

	__devices = __query_cuda_devices(error, error_message);

	if(error != cudaSuccess) {
		_description = "No compatible GPU detected: " + error_message;
		return false;
	}

	if (__devices.empty()) {
		_description = "No compatible GPU detected.";
		return false;
	}

	return true;
}

bool cuda_hasher::configure(arguments &args) {
	int index = args.get_cards_count();
	double intensity = 0;

	for(vector<double>::iterator it = args.gpu_intensity().begin(); it != args.gpu_intensity().end(); it++) {
		intensity += *it;
	}
	intensity /= args.gpu_intensity().size();

	vector<string> filter = _get_gpu_filters(args);

	int total_threads = 0;

	if (intensity == 0) {
		_intensity = 0;
		_description = "Status: DISABLED - by user.";
		return false;
	}

	bool cards_selected = false;

	for(vector<cuda_device_info *>::iterator d = __devices.begin(); d != __devices.end(); d++, index++) {
		stringstream ss;
		ss << "["<< (index + 1) << "] " << (*d)->device_string;
		string device_description = ss.str();
		(*d)->device_index = index;
        (*d)->profile_info.profile = args.get_argon2_profile();

		if(filter.size() > 0) {
			bool found = false;
			for(vector<string>::iterator fit = filter.begin(); fit != filter.end(); fit++) {
				if(device_description.find(*fit) != string::npos) {
					found = true;
					break;
				}
			}
			if(!found) {
				(*d)->profile_info.threads = 0;
				ss << " - DISABLED" << endl;
				_description += ss.str();
				continue;
			}
			else {
				cards_selected = true;
			}
		}
		else {
			cards_selected = true;
		}

		ss << endl;

		double device_intensity = 0;
		if(args.gpu_intensity().size() == 1 || (*d)->device_index >= args.gpu_intensity().size())
			device_intensity = args.gpu_intensity()[0];
		else
			device_intensity = args.gpu_intensity()[(*d)->device_index];

		_description += ss.str();

		if(!(__setup_device_info((*d), device_intensity))) {
			_description += (*d)->error_message;
			_description += "\n";
			continue;
		};

		device_info device;

		char bus_id[100];
		if(cudaDeviceGetPCIBusId(bus_id, 100, (*d)->cuda_index) == cudaSuccess) {
			device.bus_id = bus_id;
			int domain_separator = device.bus_id.find(":");
			if(domain_separator != string::npos) {
				device.bus_id.erase(0, domain_separator + 1);
			}
		}

		device.name = (*d)->device_string;
		device.intensity = device_intensity;
		_store_device_info((*d)->device_index, device);

		total_threads += (*d)->profile_info.threads;
	}

	args.set_cards_count(index);

	if(!cards_selected) {
		_intensity = 0;
		_description += "Status: DISABLED - no card enabled because of filtering.";
		return false;
	}

	if (total_threads == 0) {
		_intensity = 0;
		_description += "Status: DISABLED - not enough resources.";
		return false;
	}

	_intensity = intensity;

	__running = true;
	_update_running_status(__running);
	for(vector<cuda_device_info *>::iterator d = __devices.begin(); d != __devices.end(); d++) {
		if((*d)->profile_info.threads != 0) {
            __runners.push_back(new thread([&](cuda_device_info *device, int index) {
                this->__run(device, index);
            }, (*d), 0));
            __runners.push_back(new thread([&](cuda_device_info *device, int index) {
                this->__run(device, index);
            }, (*d), 1));
		}
	}

	_description += "Status: ENABLED - with " + to_string(total_threads) + " threads.";

	return true;
}

void cuda_hasher::cleanup() {
	__running = false;
	for(vector<thread*>::iterator it = __runners.begin();it != __runners.end();++it) {
		(*it)->join();
		delete *it;
	}
	__runners.clear();

	for(vector<cuda_device_info *>::iterator d = __devices.begin(); d != __devices.end(); d++) {
		cuda_free(*d);
	}
}

cuda_device_info *cuda_hasher::__get_device_info(int device_index) {
	cuda_device_info *device_info = new cuda_device_info();
	device_info->error = cudaSuccess;
	device_info->cuda_index = device_index;

	device_info->error = cudaSetDevice(device_index);
	if(device_info->error != cudaSuccess) {
		device_info->error_message = "Error setting current device.";
		return device_info;
	}

    cudaDeviceProp devProp;
	device_info->error = cudaGetDeviceProperties(&devProp, device_index);
	if(device_info->error != cudaSuccess) {
		device_info->error_message = "Error setting current device.";
		return device_info;
	}

    device_info->device_string = devProp.name;

    size_t freemem, totalmem;
    device_info->error = cudaMemGetInfo(&freemem, &totalmem);
	if(device_info->error != cudaSuccess) {
		device_info->error_message = "Error setting current device.";
		return device_info;
	}

    device_info->free_mem_size = freemem;
    device_info->max_allocable_mem_size = freemem / 4;

    double mem_in_gb = totalmem / 1073741824.0;
    stringstream ss;
    ss << setprecision(2) << mem_in_gb;
    device_info->device_string += (" (" + ss.str() + "GB)");

    return device_info;
}

bool cuda_hasher::__setup_device_info(cuda_device_info *device, double intensity) {
    device->profile_info.threads_per_chunk = (uint32_t)(device->max_allocable_mem_size / device->profile_info.profile->memsize);
    size_t chunk_size = device->profile_info.threads_per_chunk * argon2profile_default->memsize;

    if(chunk_size == 0) {
        device->error = cudaErrorInitializationError;
        device->error_message = "Not enough memory on GPU.";
        return false;
    }

    uint64_t usable_memory = device->free_mem_size;
    double chunks = (double)usable_memory / (double)chunk_size;

    uint32_t max_threads = (uint32_t)(device->profile_info.threads_per_chunk * chunks);

    if(max_threads == 0) {
        device->error = cudaErrorInitializationError;
        device->error_message = "Not enough memory on GPU.";
        return false;
    }

    device->profile_info.threads = (uint32_t)(max_threads * intensity / 100.0);
	device->profile_info.threads = (device->profile_info.threads / 2) * 2; // make it divisible by 2 to allow for parallel kernel execution
	if(max_threads > 0 && device->profile_info.threads == 0 && intensity > 0)
        device->profile_info.threads = 2;

    chunks = (double)device->profile_info.threads / (double)device->profile_info.threads_per_chunk;

	cuda_allocate(device, chunks, chunk_size);

	if(device->error != cudaSuccess)
		return false;

    return true;
}

vector<cuda_device_info *> cuda_hasher::__query_cuda_devices(cudaError_t &error, string &error_message) {
	vector<cuda_device_info *> devices;
	int devCount = 0;
	error = cudaGetDeviceCount(&devCount);

	if(error != cudaSuccess) {
		error_message = "Error querying CUDA device count.";
		return devices;
	}

	if(devCount == 0)
		return devices;

	for (int i = 0; i < devCount; ++i)
	{
		cuda_device_info *dev = __get_device_info(i);
		if(dev == NULL)
			continue;
		if(dev->error != cudaSuccess) {
			error = dev->error;
			error_message = dev->error_message;
			continue;
		}
		devices.push_back(dev);
	}
	return devices;
}

void cuda_hasher::__run(cuda_device_info *device, int thread_id) {
	cudaSetDevice(device->cuda_index);

	cuda_gpumgmt_thread_data thread_data;
	thread_data.device = device;
	thread_data.thread_id = thread_id;
	cudaStream_t stream;
	device->error = cudaStreamCreate(&stream);
	if(device->error != cudaSuccess) {
	    LOG("Error running kernel: (" + to_string(device->error) + ") cannot create cuda stream.");
        __running = false;
    	_update_running_status(__running);
    	return;
    }

	thread_data.device_data = stream;

#ifdef PARALLEL_CUDA
    if(thread_id == 0) {
        thread_data.threads_idx = 0;
        thread_data.threads = device->profile_info.threads / 2;
    }
    else {
        thread_data.threads_idx = device->profile_info.threads / 2;
        thread_data.threads = device->profile_info.threads - thread_data.threads_idx;
    }
#else
    thread_data.threads_idx = 0;
    thread_data.threads = device->profile_info.threads;
#endif

	void *memory = device->arguments.host_seed_memory[thread_id];

	argon2profile *profile = device->profile_info.profile;

	argon2 hash_factory(cuda_kernel_prehasher, cuda_kernel_filler, cuda_kernel_posthasher, memory, &thread_data);
	hash_factory.set_lane_length(2);
	hash_factory.set_seed_memory_offset(2 * profile->thr_cost * ARGON2_BLOCK_SIZE);
	hash_factory.set_threads(thread_data.threads);

	while(__running) {
		if(device->profile_info.threads == 0 || _should_pause()) {
			this_thread::sleep_for(chrono::milliseconds(100));
			continue;
		}

		hash_data input = _get_input();

		if(!input.base.empty()) {
			vector<hash_data> hashes;
			int hash_count = hash_factory.generate_hashes(*profile, input, hashes);

			if (device->error != cudaSuccess) {
				LOG("Error running kernel: (" + to_string(device->error) + ")" + device->error_message);
				__running = false;
				exit(0);
			}
			_store_hash(hash_count, hashes, device->device_index);
		}
	}

	_update_running_status(__running);
}

REGISTER_HASHER(cuda_hasher);

#endif //WITH_CUDA
