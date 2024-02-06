#include <sw/redis++/redis++.h>
# include <iostream>
#include <unordered_map>

using namespace sw::redis;
using namespace std;
static Redis* redis = new Redis("tcp://127.0.0.1:6379");

/**
 * Data structure to store a video frame in RGBA or YUV420 format.
 */
typedef struct vsource_frame_s {
	int channel;		/**< The channel id for the video frame */
	long long imgpts;	/**< Captured time stamp
				 * (or presentatin timestamp).
				 * This is actually a sequence number of
				 * captured video frame.  */
	// AVPixelFormat pixelformat;/**< pixel format, currently support
	// 			 * RGBA, BGRA, or YUV420P
	// 			 * Note: current use values defined in ffmpeg */
	int linesize[4];	/**< strides
				 * for each video plane (YUV420P only). */
	int realwidth;		/**< Actual width of the video frame */
	int realheight;		/**< Actual height of the video frame */
	int realstride;		/**< stride for RGBA and BGRA video frame */
	int realsize;		/**< Total size of the video frame data */
	struct timeval timestamp;	/**< Captured timestamp */
	// internal data - should not change after initialized
	int maxstride;		/**< */
	int imgbufsize;		/**< Allocated video frame buffer size */
	unsigned char *imgbuf;	/**< Pointer to the video frame buffer */
}	vsource_frame_t;


typedef struct dpipe_buffer_s {
	vsource_frame_t* pointer;
	struct dpipe_buffer_s* next;
} dpipe_buffer_t;

typedef struct dpipe_s {
	const char *name;
	int nframe;
} dpipe_t;

unordered_map<const char*, dpipe_s*> dpipemap;

dpipe_t* dpipe_create(int id, const char *name, int nframe, int maxframesize) {
	if (redis == nullptr) redis = new Redis("tcp://127.0.0.1:6379");
	if (dpipemap.find(name) != dpipemap.end())  return dpipemap[name];

	dpipe_t *dpipe = new dpipe_t;
	dpipe->name = name;
	dpipe->nframe = nframe;
	return dpipe;
}

dpipe_t* dpipe_lookup(const char* name) {
	if (dpipemap.find(name) == dpipemap.end()) return nullptr;
	return dpipemap[name];
}

int dpipe_destroy(dpipe_t* dpipe) {
	redis->del(dpipe->name);
	return 1;
}

dpipe_buffer_t* dpipe_get(dpipe_t* dpipe) {
	if (redis->llen(dpipe->name) >= dpipe->nframe) redis->rpop(dpipe->name);
	dpipe_buffer_t* buf = new dpipe_buffer_t;
	return buf;
}

void dpipe_put(dpipe_t* dpipe, dpipe_buffer_t *buffer) {
	free(buffer);
}

void serialize(dpipe_buffer_t& data, std::string& buffer) {
    vsource_frame_t* frame = data.pointer;
    if (frame == nullptr) return;

    auto channelptr = reinterpret_cast<char*>(&frame->channel);
    auto imgptsptr = reinterpret_cast<char*>(&frame->imgpts);
    auto linesizeptr = reinterpret_cast<char*>(&frame->linesize);
    auto realwidthptr = reinterpret_cast<char*>(&frame->realwidth);
    auto realheightptr = reinterpret_cast<char*>(&frame->realheight);
    auto realstrideptr = reinterpret_cast<char*>(&frame->realstride);
    auto realsizeptr = reinterpret_cast<char*>(&frame->realsize);
    auto timestampptr = reinterpret_cast<char*>(&frame->timestamp);
    auto maxstrideptr = reinterpret_cast<char*>(&frame->maxstride);
    auto imgbufsizeptr = reinterpret_cast<char*>(&frame->imgbufsize);
    auto imgbufptr = frame->imgbuf;

    std::copy(channelptr, channelptr + sizeof(frame->channel), std::back_inserter(buffer));
    std::copy(imgptsptr, imgptsptr + sizeof(frame->imgpts), std::back_inserter(buffer));
    std::copy(linesizeptr, linesizeptr + sizeof(frame->linesize), std::back_inserter(buffer));
    std::copy(realwidthptr, realwidthptr + sizeof(frame->realwidth), std::back_inserter(buffer));
    std::copy(realheightptr, realheightptr + sizeof(frame->realheight), std::back_inserter(buffer));
    std::copy(realstrideptr, realstrideptr + sizeof(frame->realstride), std::back_inserter(buffer));
    std::copy(realsizeptr, realsizeptr + sizeof(frame->realsize), std::back_inserter(buffer));
    std::copy(timestampptr, timestampptr + sizeof(frame->timestamp), std::back_inserter(buffer));
    std::copy(maxstrideptr, maxstrideptr + sizeof(frame->maxstride), std::back_inserter(buffer));
    std::copy(imgbufsizeptr, imgbufsizeptr + sizeof(frame->imgbufsize), std::back_inserter(buffer));
    std::copy(imgbufptr, imgbufptr + frame->imgbufsize, std::back_inserter(buffer));
}

void deserialize(dpipe_buffer_t& data, std::string& buffer) {
    vsource_frame_t* frame = new vsource_frame_t;
    if (frame == nullptr) return;

    auto channelptr = buffer.begin();
    auto imgptsptr = channelptr + sizeof(frame->channel);
    auto linesizeptr = imgptsptr + sizeof(frame->imgpts);
    auto realwidthptr = linesizeptr + sizeof(frame->linesize);
    auto realheightptr = realwidthptr + sizeof(frame->realwidth);
    auto realstrideptr = realheightptr + sizeof(frame->realheight);
    auto realsizeptr = realstrideptr + sizeof(frame->realstride);
    auto timestampptr = realsizeptr + sizeof(frame->realsize);
    auto maxstrideptr = timestampptr + sizeof(frame->timestamp);
    auto imgbufsizeptr = maxstrideptr + sizeof(frame->maxstride);
    auto imgbufptr = imgbufsizeptr + sizeof(frame->imgbufsize);

    std::copy(channelptr, imgptsptr, reinterpret_cast<char*>(&frame->channel));
    std::copy(imgptsptr, linesizeptr, reinterpret_cast<char*>(&frame->imgpts));
    std::copy(linesizeptr, realwidthptr,  reinterpret_cast<char*>(&frame->linesize));
    std::copy(realwidthptr, realheightptr,  reinterpret_cast<char*>(&frame->realwidth));
    std::copy(realheightptr, realstrideptr,  reinterpret_cast<char*>(&frame->realheight));
    std::copy(realstrideptr, realsizeptr,  reinterpret_cast<char*>(&frame->realstride));
    std::copy(realsizeptr, timestampptr,  reinterpret_cast<char*>(&frame->realsize));
    std::copy(timestampptr, maxstrideptr,  reinterpret_cast<char*>(&frame->timestamp));
    std::copy(maxstrideptr, imgbufsizeptr,  reinterpret_cast<char*>(&frame->maxstride));
    std::copy(imgbufsizeptr, imgbufptr,  reinterpret_cast<char*>(&frame->imgbufsize));

    frame->imgbuf = new unsigned char[frame->imgbufsize];
    std::copy(imgbufptr, imgbufptr + frame->imgbufsize, frame->imgbuf);
    data.pointer = frame;
}

dpipe_buffer_t* dpipe_load(dpipe_t* dpipe) {
    std::vector<std::string> result;
    redis->lrange(dpipe->name, 0, -1, std::back_inserter(result));
    redis->del(dpipe->name);

    dpipe_buffer_t* head = nullptr;
    if (result.size() > 0) {
        head = new dpipe_buffer_t;
        dpipe_buffer_t* cur = head;

        for (auto& res: result) {
            deserialize(*cur, res);
            cur->next = new dpipe_buffer_t;
            cur = cur->next;
        }
    }

    return head;
}

void dpipe_store(dpipe_t *dpipe, dpipe_buffer_t *buffer) {
	dpipe_buffer_t* cur = buffer;
    int i = 0;
    std::string raw;
	while (cur) {
		if (cur->pointer != nullptr) {
            serialize(*cur, raw);
            redis->lpush(dpipe->name, raw);
            raw.clear();
        };
		cur = cur->next;
	}
}

void printFrame(const vsource_frame_t* frame) {
    if (frame == nullptr) return;
    std::cout << "Channel: " << frame->channel << std::endl;
    std::cout << "Image Timestamp: " << frame->imgpts << std::endl;
    // You can uncomment the following lines if pixelformat is used
    // std::cout << "Pixel Format: " << frame->pixelformat << std::endl;
    std::cout << "Linesize: [" << frame->linesize[0] << ", " << frame->linesize[1] << ", " << frame->linesize[2] << ", " << frame->linesize[3] << "]" << std::endl;
    std::cout << "Real Width: " << frame->realwidth << std::endl;
    std::cout << "Real Height: " << frame->realheight << std::endl;
    std::cout << "Real Stride: " << frame->realstride << std::endl;
    std::cout << "Real Size: " << frame->realsize << std::endl;
    std::cout << "Timestamp: " << frame->timestamp.tv_sec << " seconds, " << frame->timestamp.tv_usec << " microseconds" << std::endl;
    std::cout << "Max Stride: " << frame->maxstride << std::endl;
    std::cout << "Image Buffer Size: " << frame->imgbufsize << std::endl;

    // Print the image buffer content (first 10 elements for illustration)
    std::cout << "Image Buffer Content: ";
    for (int i = 0; i < std::min(10, frame->imgbufsize); ++i) {
        std::cout << static_cast<int>(frame->imgbuf[i]) << " ";
    }
    std::cout << std::endl;
}

int main() {
    auto dpipe = dpipe_create(0, "video-0", 20, 10);
    auto buf1 = dpipe_get(dpipe);
    auto buf2 = dpipe_get(dpipe);

    vsource_frame_t frame1;
    frame1.channel = 1;
    frame1.imgpts = 2;
    frame1.linesize[0] = 1;
    frame1.linesize[1] = 2;
    frame1.linesize[2] = 3;
    frame1.linesize[3] = 4;
    frame1.realwidth = 640;
    frame1.realheight = 480;
    frame1.realstride = 4 * frame1.realwidth; // Assuming RGBA format
    frame1.realsize = frame1.realstride * frame1.realheight;

    // Allocate memory for imgbuf and set some sample data
    frame1.imgbufsize = frame1.realsize;
    frame1.imgbuf = new unsigned char[frame1.imgbufsize];
    for (int i = 0; i < frame1.imgbufsize; ++i) {
        frame1.imgbuf[i] = static_cast<unsigned char>(i % 256);
    }

    // Initialize timestamp if needed
    frame1.timestamp.tv_sec = 0;
    frame1.timestamp.tv_usec = 0;
    // Initialize other members as needed
    frame1.maxstride = 0;

    vsource_frame_t frame2;
    frame2.channel = 2;
    frame2.imgpts = 5;
    frame2.linesize[0] = 4;
    frame2.linesize[1] = 8;
    frame2.linesize[2] = 12;
    frame2.linesize[3] = 16;
    frame2.realwidth = 1280;
    frame2.realheight = 720;
    frame2.realstride = 5120; // Assuming RGBA format
    frame2.realsize = 1280 * 720 * 4; // Assuming RGBA format

    // Allocate memory for imgbuf and set some sample data
    frame2.imgbufsize = frame2.realsize;
    frame2.imgbuf = new unsigned char[frame2.imgbufsize];
    for (int i = 0; i < frame2.imgbufsize; ++i) {
        frame2.imgbuf[i] = static_cast<unsigned char>((i + 128) % 256);
    }

    // Initialize timestamp as needed
    frame2.timestamp = {0, 0};
    // Initialize other members as needed
    frame2.maxstride = 0;

    buf1->pointer = &frame1;
    buf2->pointer = &frame2;

    buf1->next = buf2;

    dpipe_store(dpipe, buf1);
    auto deserialized_buffer = dpipe_load(dpipe);

    printFrame(deserialized_buffer->pointer);

    delete[] frame1.imgbuf;
    delete[] frame2.imgbuf;

    return 0;
}


// struct A {
//     int channel;
//     int imgbufsize;
//     char* imgbuf;
// };

// // Serialization function
// void serialize(const struct A* data, std::string& buffer, size_t bufferSize) {
//     // Check buffer size
//     if (bufferSize < sizeof(struct A)) {
//         printf("Error: Insufficient buffer size for serialization\n");
//         return;
//     }

//     // Copy the individual members into the buffer
//     std::copy(reinterpret_cast<const char*>(&data->channel),
//               reinterpret_cast<const char*>(&data->channel) + sizeof(data->channel),
//               std::back_inserter(buffer));

//     std::copy(reinterpret_cast<const char*>(&data->imgbufsize),
//             reinterpret_cast<const char*>(&data->imgbufsize) + sizeof(data->imgbufsize),
//             std::back_inserter(buffer));

//     std::copy(data->imgbuf, data->imgbuf + data->imgbufsize, std::back_inserter(buffer));

//     std::cout << buffer << std::endl;
// }

// // Deserialization function
// void deserialize(struct A* data, const std::string& buffer, size_t bufferSize) {
//     // Check buffer size
//     if (bufferSize < sizeof(struct A)) {
//         printf("Error: Insufficient buffer size for deserialization\n");
//         return;
//     }

//     // Read individual members from the buffer
//     std::copy(buffer.begin(), buffer.begin() + sizeof(data->channel),
//               reinterpret_cast<char*>(&data->channel));

//     std::copy(buffer.begin() + sizeof(data->channel), buffer.begin() + sizeof(data->channel) + sizeof(data->imgbufsize),
//               reinterpret_cast<char*>(&data->imgbufsize));

//     char* test = new char[data->imgbufsize];
//     std::copy(buffer.begin() + sizeof(data->channel) + sizeof(data->imgbufsize),
//               buffer.begin() + sizeof(data->channel) + sizeof(data->imgbufsize) + data->imgbufsize,
//               test);
//     *(test + data->imgbufsize) = '\0';
//     data->imgbuf = test;
// }

// int main() {
//     // Example struct
//     A originalData;
//     originalData.channel = 3;
//     originalData.imgbuf = "223";
//     originalData.imgbufsize = 3;

//     A serializedStruct;
//     std::string buffer;
//     buffer.reserve(sizeof(A));
//     // Serialize the struct
//     serialize(&originalData, buffer, sizeof(buffer));
    

//     // Store the serialized data in Redis
//     redis->set("key", buffer);

//     // Deserialized structure
//     struct A deserializedData;

//     // Retrieve the serialized data from Redis
//     std::string retrievedData = redis->get("key").value_or("dog");
//     // cout << retrievedData << endl;

//     // Deserialize the data back into struct A
//     deserialize(&deserializedData, retrievedData, sizeof(buffer));

//     // // Output the results
//     printf("Original Data: %d %s %d\n", originalData.channel, originalData.imgbuf, originalData.imgbufsize);
//     printf("Deserialized Data: %d %s %d\n", deserializedData.channel, deserializedData.imgbuf, deserializedData.imgbufsize);

//     return 0;
// }