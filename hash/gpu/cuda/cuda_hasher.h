//
// Created by Haifa Bogdan Adnan on 18/09/2018.
//

#ifndef IXIMINER_CUDA_HASHER_H
#define IXIMINER_CUDA_HASHER_H

#if defined(WITH_CUDA)

struct cuda_kernel_arguments {
    void *memory_chunk_0;
    void *memory_chunk_1;
    void *memory_chunk_2;
    void *memory_chunk_3;
    void *memory_chunk_4;
    void *memory_chunk_5;

    uint32_t *refs;
    uint32_t *idxs;
    uint32_t *segments;

	uint32_t *preseed_memory[2];
	uint32_t *seed_memory[2];
	uint32_t *out_memory[2];
	uint32_t *hash_memory[2];

    uint32_t *host_seed_memory[2];
};

struct argon2profile_info {
    argon2profile_info() {
        threads = 0;
        threads_per_chunk = 0;
        profile = argon2profile_default;
    }
    uint32_t threads;
    uint32_t threads_per_chunk;
    argon2profile *profile;
};

struct cuda_device_info {
	cuda_device_info() {
		device_index = 0;
		device_string = "";
		free_mem_size = 0;
		max_allocable_mem_size = 0;

		error = cudaSuccess;
		error_message = "";
	}

    int device_index;
	int cuda_index;

    string device_string;
    uint64_t free_mem_size;
    uint64_t max_allocable_mem_size;

    argon2profile_info profile_info;
	cuda_kernel_arguments arguments;

    mutex device_lock;

    cudaError_t error;
    string error_message;
};

struct cuda_gpumgmt_thread_data {
	void lock() {
#ifndef PARALLEL_CUDA
		device->device_lock.lock();
#endif
	}

	void unlock() {
#ifndef PARALLEL_CUDA
		device->device_lock.unlock();
#endif
	}

	int thread_id;
	cuda_device_info *device;
	void *device_data;

	int threads;
	int threads_idx;
};

class cuda_hasher : public hasher {
public:
	cuda_hasher();
	~cuda_hasher();

	virtual bool initialize();
	virtual bool configure(arguments &args);
	virtual void cleanup();

private:
    cuda_device_info *__get_device_info(int device_index);
    bool __setup_device_info(cuda_device_info *device, double intensity);
    vector<cuda_device_info*> __query_cuda_devices(cudaError_t &error, string &error_message);

    void __run(cuda_device_info *device, int thread_id);

    vector<cuda_device_info*> __devices;

    bool __running;
    vector<thread*> __runners;
};

// CUDA kernel exports
extern void cuda_allocate(cuda_device_info *device, double chunks, size_t chunk_size);
extern void cuda_free(cuda_device_info *device);
extern bool cuda_kernel_prehasher(void *memory, int threads, argon2profile *profile, void *user_data);
extern void *cuda_kernel_filler(void *memory, int threads, argon2profile *profile, void *user_data);
extern bool cuda_kernel_posthasher(void *memory, int threads, argon2profile *profile, void *user_data);
// end CUDA kernel exports

#endif //WITH_CUDA

#endif //IXIMINER_CUDA_HASHER_H