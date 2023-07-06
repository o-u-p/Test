#include <cstdlib>
#include <thread>
#include <uv.h>

// Structure to hold thread-specific data
typedef struct {
    uv_loop_t* loop;
    uv_udp_t* udp_handle;
} ThreadData;

// Function to handle received UDP packets
void on_udp_receive(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
	size_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
	fprintf(stderr, "%lu Read %lu packet\n", id, nread);
    // Handle received packet
}

// Function to allocate a buffer for received UDP packets
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	*buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

// Thread entry point
void thread_entry(void* arg) {
    ThreadData* thread_data = (ThreadData*)arg;

    // Create a UDP handle
    uv_udp_t* udp_handle = (uv_udp_t*)malloc(sizeof(uv_udp_t));
    uv_udp_init(thread_data->loop, udp_handle);

    // Bind the UDP handle to a specific address and port
    struct sockaddr_in recv_addr;
    uv_ip4_addr("0.0.0.0", 12345, &recv_addr);
    uv_udp_bind(udp_handle, (const struct sockaddr*)&recv_addr, UV_UDP_REUSEADDR);

    // Start receiving UDP packets
    uv_udp_recv_start(udp_handle, alloc_buffer, on_udp_receive);
	size_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
	fprintf(stderr, "create thread:%lu\n", id);

    // Run the event loop for this thread
    uv_run(thread_data->loop, UV_RUN_DEFAULT);
}

int main() {
    // Create thread data structures
    const int num_threads = 4;
    ThreadData thread_data[num_threads];
	uv_thread_t threads[num_threads];

    // Create and initialize the event loops and UDP handles for each thread
    for (int i = 0; i < num_threads; ++i) {
        uv_loop_t* loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uv_loop_init(loop);

        thread_data[i].loop = loop;
        thread_data[i].udp_handle = NULL;

        // Create and start the thread
        std::thread stdThread(thread_entry, &thread_data[i]);
		stdThread.detach();
		fprintf(stderr, "detach thread:%d\n", i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(100000));

	return 0;
}

