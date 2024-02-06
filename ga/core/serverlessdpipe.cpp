#include "serverlessdpipe.h"
#include <unordered_map>

using namespace sw::redis;
using namespace std;
static Redis* redis = nullptr;

unordered_map<const char*, serverless_dpipe_t*> dpipemap;


serverless_dpipe_t*	serverless_dpipe_create(int id, const char *name, int nframe, int maxframesize) {
	if (redis == nullptr) redis = new Redis("tcp://127.0.0.1:6379");
	if (dpipemap.find(name) != dpipemap.end())  return dpipemap[name];

	serverless_dpipe_t *dpipe = new serverless_dpipe_t;
	dpipe->name = name;
	dpipe->nframe = nframe;
    dpipe->channel_id = id;
    dpipemap[name] = dpipe;
	return dpipe;
}

serverless_dpipe_t*	serverless_dpipe_lookup(const char *name) {
	if (dpipemap.find(name) == dpipemap.end()) return nullptr;
    ga_error("%s \n", dpipemap[name]);
	return dpipemap[name];
}

int serverless_dpipe_destroy(serverless_dpipe_t* dpipe) {
	redis->del(dpipe->name);
	return 1;
}

serverless_dpipe_buffer_t *	serverless_dpipe_get(serverless_dpipe_t *dpipe) {
	if (redis->llen(dpipe->name) >= dpipe->nframe) redis->rpop(dpipe->name);
	serverless_dpipe_buffer_t* buf = new serverless_dpipe_buffer_t;
	return buf;
}

void serverless_dpipe_put(serverless_dpipe_t* dpipe, serverless_dpipe_buffer_t *buffer) {
	free(buffer);
}

void serialize(serverless_dpipe_buffer_t& data, std::string& buffer) {
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

void deserialize(serverless_dpipe_buffer_t& data, std::string& buffer) {
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

serverless_dpipe_buffer_t* serverless_dpipe_load(serverless_dpipe_t* dpipe, const struct timespec *abstime) {
    std::vector<std::string> result;
    redis->lrange(dpipe->name, 0, -1, std::back_inserter(result));
    redis->del(dpipe->name);

    serverless_dpipe_buffer_t* head = nullptr;
    if (result.size() > 0) {
        head = new serverless_dpipe_buffer_t;
        serverless_dpipe_buffer_t* cur = head;

        for (auto& res: result) {
            deserialize(*cur, res);
            cur->next = new serverless_dpipe_buffer_t;
            cur = cur->next;
        }
    }

    return head;
}

void serverless_dpipe_store(serverless_dpipe_t *dpipe, serverless_dpipe_buffer_t *buffer) {
	serverless_dpipe_buffer_t* cur = buffer;
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